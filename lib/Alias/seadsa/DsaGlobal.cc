#include <llvm/ADT/Optional.h>
#include <llvm/ADT/SCCIterator.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

#include "Alias/seadsa/AllocWrapInfo.hh"
#include "Alias/seadsa/BottomUp.hh"
#include "Alias/seadsa/CallGraphUtils.hh"
#include "Alias/seadsa/CallSite.hh"
#include "Alias/seadsa/Cloner.hh"
#include "Alias/seadsa/CompleteCallGraph.hh"
#include "Alias/seadsa/DsaLibFuncInfo.hh"
#include "Alias/seadsa/Global.hh"
#include "Alias/seadsa/Graph.hh"
#include "Alias/seadsa/GraphUtils.hh"
#include "Alias/seadsa/Local.hh"
#include "Alias/seadsa/TopDown.hh"
#include "Alias/seadsa/config.h"
#include "Alias/seadsa/support/Debug.h"

#include <queue>

static llvm::cl::opt<bool> normalizeUniqueScalars(
    "sea-dsa-norm-unique-scalar",
    llvm::cl::desc("SeaDsa: all callees and callers agree on unique scalars"),
    llvm::cl::init(true), llvm::cl::Hidden);

static llvm::cl::opt<bool> normalizeAllocaSites(
    "sea-dsa-norm-alloca-sites",
    llvm::cl::desc("SeaDsa: all callees and callers agree on allocation sites"),
    llvm::cl::init(true), llvm::cl::Hidden);

static llvm::cl::opt<bool> UseDsaCallGraph(
    "sea-dsa-devirt",
    llvm::cl::desc(
        "Build a complete call graph before running SeaDsa analyses"),
    llvm::cl::init(false));

using namespace llvm;

namespace seadsa {

//////
/// Context-insensitive analysis
/////
// Unify callsite arguments within the same graph
void ContextInsensitiveGlobalAnalysis::resolveArguments(
    DsaCallSite &cs, Graph &g, const DsaLibFuncInfo &dlfi) {

  // unify return
  auto callee = cs.getCallee();
  auto calleeSpec =
      dlfi.hasSpecFunc(*callee) ? dlfi.getSpecFunc(*callee) : callee;

  if (g.hasRetCell(*calleeSpec)) {
    Cell &nc = g.mkCell(*cs.getInstruction(), Cell());
    const Cell &r = g.getRetCell(*calleeSpec);
    Cell c(*r.getNode(), r.getRawOffset());
    nc.unify(c);
  }

  auto range = llvm::make_filter_range(calleeSpec->args(), [](auto &arg) {
    return arg.getType()->isPointerTy();
  });

  // unify actuals and formals
  DsaCallSite::const_actual_iterator AI = cs.actual_begin(),
                                     AE = cs.actual_end();
  for (auto FI = range.begin(), FE = range.end(); FI != FE && AI != AE;
       ++FI, ++AI) {
    const Value *arg = (*AI).get();
    const Value *farg = &*FI;
    if (g.hasCell(*farg)) {
      Cell &c = g.mkCell(*arg, Cell());
      Cell &d = g.mkCell(*farg, Cell());
      c.unify(d);
    }
  }
}

bool ContextInsensitiveGlobalAnalysis::runOnModule(Module &M) {

  LOG("dsa-global",
      errs() << "Started context-insensitive global analysis ... \n");

  // ufo::Stats::resume ("CI-DsaAnalysis");

  if (kind() == GlobalAnalysisKind::FLAT_MEMORY)
    m_graph.reset(new FlatGraph(m_dl, m_setFactory));
  else
    m_graph.reset(new Graph(m_dl, m_setFactory));

  LocalAnalysis la(m_dl, m_tliWrapper, m_allocInfo);

  // -- bottom-up inlining of all graphs
  for (auto it = scc_begin(&m_cg); !it.isAtEnd(); ++it) {
    auto &scc = *it;

    // --- all scc members share the same local graph
    for (CallGraphNode *cgn : scc) {
      Function *fn = cgn->getFunction();
      if (!fn) continue;
      if ((fn->isDeclaration() || fn->empty()) &&
          !m_dsaLibFuncInfo.hasSpecFunc(*fn))
        continue;
      // compute local graph

      auto spec = m_dsaLibFuncInfo.hasSpecFunc(*fn)
                      ? m_dsaLibFuncInfo.getSpecFunc(*fn)
                      : fn;

      GraphRef fGraph = nullptr;
      if (kind() == GlobalAnalysisKind::FLAT_MEMORY)
        fGraph.reset(new FlatGraph(m_dl, m_setFactory));
      else
        fGraph.reset(new Graph(m_dl, m_setFactory));

      la.runOnFunction(*spec, *fGraph);

      m_fns.insert(spec);
      m_graph->import(*fGraph, true);
    }

    // --- resolve callsites
    for (CallGraphNode *cgn : scc) {
      Function *fn = cgn->getFunction();

      // XXX probably not needed since if the function is external
      // XXX it will have no call records
      if (!fn) continue;

      if ((fn->isDeclaration() || fn->empty()) &&
          !m_dsaLibFuncInfo.hasSpecFunc(*fn))
        continue;

      // -- iterate over all call instructions of the current function fn
      // -- they are indexed in the CallGraphNode data structure
      for (auto &callRecord : *cgn) {
        llvm::Optional<DsaCallSite> dsaCS =
            call_graph_utils::getDsaCallSite(callRecord, &m_dsaLibFuncInfo);
        if ((!dsaCS.hasValue())) { continue; }
        assert(fn == dsaCS.getValue().getCaller());
        resolveArguments(dsaCS.getValue(), *m_graph, m_dsaLibFuncInfo);
      }
    }
    m_graph->compress();
  }
  m_graph->remove_dead();

  // ufo::Stats::stop ("CI-DsaAnalysis");

  LOG("dsa-global-graph", errs() << "### Global Dsa graph \n";
      m_graph->write(errs()); errs() << "\n");

  LOG("dsa-global",
      errs() << "Finished context-insensitive global analysis.\n");

  return false;
}

const Graph &
ContextInsensitiveGlobalAnalysis::getGraph(const Function &) const {
  assert(m_graph);
  return *m_graph;
}

Graph &ContextInsensitiveGlobalAnalysis::getGraph(const Function &) {
  assert(m_graph);
  return *m_graph;
}

bool ContextInsensitiveGlobalAnalysis::hasGraph(const Function &fn) const {
  return m_fns.count(&fn) > 0;
}

/// LLVM passes

ContextInsensitiveGlobalPass::ContextInsensitiveGlobalPass()
    : DsaGlobalPass(ID), m_ga(nullptr) {}

void ContextInsensitiveGlobalPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  // dependency for immutable AllowWrapInfo
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<AllocWrapInfo>();
  AU.addRequired<DsaLibFuncInfo>();
  if (UseDsaCallGraph) {
    AU.addRequired<CompleteCallGraph>();
  } else {
    AU.addRequired<CallGraphWrapperPass>();
  }
  AU.setPreservesAll();
}

bool ContextInsensitiveGlobalPass::runOnModule(Module &M) {
  auto &dl = M.getDataLayout();
  auto &tli = getAnalysis<TargetLibraryInfoWrapperPass>();
  auto &allocInfo = getAnalysis<AllocWrapInfo>();
  auto &dsaLibFuncInfo = getAnalysis<DsaLibFuncInfo>();
  allocInfo.initialize(M, this);
  dsaLibFuncInfo.initialize(M);

  CallGraph *cg = nullptr;
  if (UseDsaCallGraph) {
    cg = &getAnalysis<CompleteCallGraph>().getCompleteCallGraph();
  } else {
    cg = &getAnalysis<CallGraphWrapperPass>().getCallGraph();
  }
  const bool useFlatMemory = false;
  m_ga.reset(new ContextInsensitiveGlobalAnalysis(
      dl, tli, allocInfo, dsaLibFuncInfo, *cg, m_setFactory, useFlatMemory));
  return m_ga->runOnModule(M);
}

FlatMemoryGlobalPass::FlatMemoryGlobalPass()
    : DsaGlobalPass(ID), m_ga(nullptr) {}

void FlatMemoryGlobalPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addRequired<CallGraphWrapperPass>();
  // dependency for immutable AllowWrapInfo
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<AllocWrapInfo>();
  AU.addRequired<DsaLibFuncInfo>();
  AU.setPreservesAll();
}

bool FlatMemoryGlobalPass::runOnModule(Module &M) {
  auto &dl = M.getDataLayout();
  auto &tli = getAnalysis<TargetLibraryInfoWrapperPass>();
  auto &cg = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  auto &allocInfo = getAnalysis<AllocWrapInfo>();
  auto &dsaLibFuncInfo = getAnalysis<DsaLibFuncInfo>();
  allocInfo.initialize(M, this);
  dsaLibFuncInfo.initialize(M);

  const bool useFlatMemory = true;
  m_ga.reset(new ContextInsensitiveGlobalAnalysis(
      dl, tli, allocInfo, dsaLibFuncInfo, cg, m_setFactory, useFlatMemory));
  return m_ga->runOnModule(M);
}

} // end namespace seadsa

namespace seadsa {

// A simple worklist implementation
template <typename T> struct WorkList<T>::impl {
  std::queue<T> m_w;
  std::set<T> m_s;
};
template <typename T>
WorkList<T>::WorkList() : m_pimpl(new WorkList<T>::impl()) {}
template <typename T> bool WorkList<T>::empty() const {
  return m_pimpl->m_w.empty();
}
template <typename T> size_t WorkList<T>::size() const {
  return m_pimpl->m_w.size();
}
template <typename T> void WorkList<T>::enqueue(const T &e) {
  auto p = m_pimpl->m_s.insert(e);
  if (p.second) { m_pimpl->m_w.push(e); }
}
template <typename T> T WorkList<T>::dequeue() {
  T e = m_pimpl->m_w.front();
  m_pimpl->m_w.pop();
  m_pimpl->m_s.erase(e);
  return e;
}

std::unique_ptr<Graph> cloneGraph(const llvm::DataLayout &dl,
                                  Graph::SetFactory &sf, const Graph &g) {
  std::unique_ptr<Graph> new_g(new Graph(dl, sf, g.isFlat()));
  new_g->import(g, true /*copy all parameters*/);
  return new_g;
}

//////
/// Context-sensitive analysis as described in SAS'17: bottom up +
/// iterative (bottom-up/top-down) propagation on callsites.
//////

ContextSensitiveGlobalAnalysis::ContextSensitiveGlobalAnalysis(
    const llvm::DataLayout &dl, llvm::TargetLibraryInfoWrapperPass &tliWrapper,
    const AllocWrapInfo &allocInfo, const DsaLibFuncInfo &dsaLibFuncInfo,
    llvm::CallGraph &cg, SetFactory &setFactory, bool storeSummaryGraphs)
    : GlobalAnalysis(GlobalAnalysisKind::CONTEXT_SENSITIVE), m_dl(dl),
      m_tliWrapper(tliWrapper), m_allocInfo(allocInfo),
      m_dsaLibFuncInfo(dsaLibFuncInfo), m_cg(cg), m_setFactory(setFactory),
      m_store_bu_graphs(storeSummaryGraphs) {}

bool ContextSensitiveGlobalAnalysis::checkAllNodesAreMapped(
    const Function &fn, Graph &fnG, const SimulationMapper &sm) {
  std::set<const Node *> reach;
  std::set<const Node *> retReach /*unused*/;
  graph_utils::reachableNodes(fn, fnG, reach, retReach);
  for (const Node *n : reach) {
    Cell callerC = sm.get(Cell(const_cast<Node *>(n), 0));
    if (callerC.isNull()) {
      errs() << "ERROR: callee node " << *n
             << " not mapped to a caller node.\n";
      return false;
    }
  }
  return true;
}

bool ContextSensitiveGlobalAnalysis::runOnModule(Module &M) {

  LOG("dsa-global",
      errs() << "Started context-sensitive global analysis ... \n");
  // ufo::Stats::resume ("CS-DsaAnalysis");

  // Keep checks until implementation is stable
#ifdef SANITY_CHECKS
  const bool do_sanity_checks = true;
#else
  const bool do_sanity_checks = true;
#endif

  for (auto &F : M) {
    if ((F.isDeclaration() || F.empty()) && !m_dsaLibFuncInfo.hasSpecFunc(F))
      continue;
    GraphRef fGraph = std::make_shared<Graph>(m_dl, m_setFactory);
    m_graphs[&F] = fGraph;
  }

  // -- Run bottom up analysis on the whole call graph
  //    and initialize worklist
  const bool flowSensitiveOpt = false;
  BottomUpAnalysis bu(m_dl, m_tliWrapper, m_allocInfo, m_dsaLibFuncInfo, m_cg,
                      flowSensitiveOpt);
  bu.runOnModule(M, m_graphs);

  if (m_store_bu_graphs) {
    // -- store bottom-up graphs
    for (auto kv : m_graphs) {
      m_bu_graphs.insert(
          {kv.getFirst(), cloneGraph(m_dl, m_setFactory, *(kv.getSecond()))});
    }
  }

  // -- Compute simulation map so that we can identify which callsites
  // -- require extra top-down propagation. Since bottom-up pass has
  // -- been done already, we assume that the simulation relation is a
  // -- total function (i.e., each callee node is mapped to a single
  // -- node in the caller).
  CalleeCallerMapping callee_caller_map;
  for (auto it = scc_begin(&m_cg); !it.isAtEnd(); ++it) {
    auto &scc = *it;
    for (CallGraphNode *cgn : scc) {
      Function *fn = cgn->getFunction();
      if (!fn || fn->isDeclaration() || fn->empty()) { continue; }
      // -- store the simulation maps from the SCC
      for (auto &callRecord : *cgn) {

        llvm::Optional<DsaCallSite> dsaCS =
            call_graph_utils::getDsaCallSite(callRecord);
        if (!dsaCS.hasValue()) { continue; }

        assert(m_graphs.count(dsaCS.getValue().getCaller()) > 0);
        assert(m_graphs.count(dsaCS.getValue().getCallee()) > 0);

        Graph &callerG = *(m_graphs.find(dsaCS.getValue().getCaller())->second);
        Graph &calleeG = *(m_graphs.find(dsaCS.getValue().getCallee())->second);

        SimulationMapperRef sm(new SimulationMapper());
        bool res = Graph::computeCalleeCallerMapping(
            dsaCS.getValue(), calleeG, callerG, *sm, do_sanity_checks);
        if (!res) { llvm_unreachable("Simulation mapping check failed"); }
        callee_caller_map.emplace(dsaCS.getValue(), sm);

        if (do_sanity_checks) {
          // Check the simulation map is a function
          if (!sm->isFunction()) {
            errs() << "ERROR (sea-dsa): simulation map for "
                   << *dsaCS.getValue().getInstruction()
                   << " is not a function!\n";
          } else {
            // Check the simulation map is a total function: check
            // that all nodes in the callee are mapped to one node in
            // the caller graph
            checkAllNodesAreMapped(*dsaCS.getValue().getCallee(), calleeG, *sm);
          }
        }
      }
    }
  }

  CallGraphWrapper dsaCG(m_cg);
  dsaCG.buildDependencies();

  /// push in the worklist callsites for which two different
  /// callee nodes are mapped to the same caller node
  WorkList<DsaCallSite> w;
  for (auto &kv :
       llvm::make_range(callee_caller_map.begin(), callee_caller_map.end())) {
    auto const &simMapper = *(kv.second);
    assert(simMapper.isFunction());

    if (!simMapper.isInjective()) w.enqueue(kv.first); // they do need top-down
  }

  /// -- top-down/bottom-up propagation until no change

  LOG("dsa-global",
      errs() << "Initially " << w.size() << " callsite to propagate\n";);

  unsigned td_props = 0;
  unsigned bu_props = 0;
  while (!w.empty()) {
    DsaCallSite dsaCS = w.dequeue();
    auto callee = dsaCS.getCallee();
    if (!callee || callee->isDeclaration() || callee->empty()) continue;

    LOG("dsa-global", errs()
                          << "Selected callsite " << *(dsaCS.getInstruction())
                          << " from queue ... ";);
    auto caller = dsaCS.getCaller();

    assert(m_graphs.count(caller) > 0);
    assert(m_graphs.count(callee) > 0);

    Graph &callerG = *(m_graphs.find(caller)->second);
    Graph &calleeG = *(m_graphs.find(callee)->second);

    // -- find out which propagation is needed if any
    auto propKind = decidePropagation(dsaCS, calleeG, callerG);
    if (propKind == DOWN) {
      propagateTopDown(dsaCS, callerG, calleeG);
      td_props++;
      auto &calleeU = dsaCG.getUses(*callee);
      auto &calleeD = dsaCG.getDefs(*callee);
      for (auto cs : calleeU)
        w.enqueue(cs); // they might need bottom-up
      for (auto cs : calleeD)
        w.enqueue(cs); // they might need top-down
    } else if (propKind == UP) {
      propagateBottomUp(dsaCS, calleeG, callerG);
      bu_props++;
      auto &callerU = dsaCG.getUses(*caller);
      auto &callerD = dsaCG.getDefs(*caller);
      for (auto cs : callerU)
        w.enqueue(cs); // they might need bottom-up
      for (auto cs : callerD)
        w.enqueue(cs); // they might need top-down
    }
    LOG("dsa-global", errs() << "processed\n";);
  }

  LOG("dsa-global",
      errs() << "-- Number of top-down propagations=" << td_props << "\n";
      errs() << "-- Number of bottom-up propagations=" << bu_props << "\n";);

  if (do_sanity_checks) {
    if (!checkNoMorePropagation(m_cg)) {
      errs() << "ERROR (sea-dsa) sanity check failed: more top-down or "
             << "bottom-up propagation is needed\n";
    }
  }

  if (normalizeUniqueScalars) {
    CallGraphClosure<ContextSensitiveGlobalAnalysis, UniqueScalar> usa(*this,
                                                                       dsaCG);
    usa.runOnModule(M);
  }

  if (normalizeAllocaSites) {
    CallGraphClosure<ContextSensitiveGlobalAnalysis, AllocaSite> asa(*this,
                                                                     dsaCG);
    asa.runOnModule(M);
  }

  // Removing dead nodes (if any)
  for (auto &kv : m_graphs)
    kv.second->remove_dead();

  LOG(
      "dsa-global-graph", for (auto &kv
                               : m_graphs) {
        errs() << "### Global Dsa graph for " << kv.first->getName() << "\n";
        kv.second->write(errs());
        errs() << "\n";
      });

  LOG("dsa-global", errs() << "Finished context-sensitive global analysis\n");

  // ufo::Stats::stop ("CS-DsaAnalysis");

  return false;
}

// Decide which kind of propagation (if any) is needed
ContextSensitiveGlobalAnalysis::PropagationKind
ContextSensitiveGlobalAnalysis::decidePropagation(const DsaCallSite &cs,
                                                  Graph &calleeG,
                                                  Graph &callerG) {

  PropagationKind res = UP;
  SimulationMapper sm;
  if (Graph::computeCalleeCallerMapping(cs, calleeG, callerG, sm, false)) {
    if (sm.isFunction()) {
      // isInjective only checks modified nodes by default
      res = (sm.isInjective() ? NONE : DOWN);
    }
  }
  return res;
}

void ContextSensitiveGlobalAnalysis::propagateTopDown(const DsaCallSite &cs,
                                                      Graph &callerG,
                                                      Graph &calleeG) {

  const bool flowSensitiveOpt = false;
  const bool noescape = true;
  TopDownAnalysis::cloneAndResolveArguments(cs, callerG, calleeG,
                                            flowSensitiveOpt, noescape);

  LOG(
      "dsa-global", if (decidePropagation(cs, calleeG, callerG) == DOWN) {
        errs() << "Sanity check failed:"
               << " we should not need more top-down propagation\n";
      });
  // errs () << "Top-down propagation at " << *cs.getInstruction () << "\n";
  assert(decidePropagation(cs, calleeG, callerG) != DOWN);
}

void ContextSensitiveGlobalAnalysis::propagateBottomUp(const DsaCallSite &cs,
                                                       Graph &calleeG,
                                                       Graph &callerG) {

  const bool flowSensitiveOpt = false;
  BottomUpAnalysis::cloneAndResolveArguments(
      cs, calleeG, callerG, m_dsaLibFuncInfo, flowSensitiveOpt);

  LOG(
      "dsa-global", if (decidePropagation(cs, calleeG, callerG) == UP) {
        errs() << "Sanity check failed:"
               << " we should not need more bottom-up propagation\n";
      });
  // errs () << "Bottom-up propagation at " << *cs.getInstruction () << "\n";
  assert(decidePropagation(cs, calleeG, callerG) != UP);
}

// Perform some sanity checks:
// 1) each callee node can be simulated by its corresponding caller node.
// 2) no two callee nodes are mapped to the same caller node.
bool ContextSensitiveGlobalAnalysis::checkNoMorePropagation(CallGraph &cg) {
  for (auto it = scc_begin(&cg); !it.isAtEnd(); ++it) {
    auto &scc = *it;
    for (CallGraphNode *cgn : scc) {
      Function *fn = cgn->getFunction();
      if (!fn || fn->isDeclaration() || fn->empty()) continue;

      for (auto &callRecord : *cgn) {
        llvm::Optional<DsaCallSite> dsaCS =
            call_graph_utils::getDsaCallSite(callRecord);
        if (!dsaCS.hasValue()) { continue; }

        assert(m_graphs.count(dsaCS.getValue().getCaller()) > 0);
        assert(m_graphs.count(dsaCS.getValue().getCallee()) > 0);
        Graph &callerG = *(m_graphs.find(dsaCS.getValue().getCaller())->second);
        Graph &calleeG = *(m_graphs.find(dsaCS.getValue().getCallee())->second);
        PropagationKind pkind =
            decidePropagation(dsaCS.getValue(), calleeG, callerG);
        if (pkind != NONE) {
          auto pkind_str = (pkind == UP) ? "bottom-up" : "top-down";
          errs() << "ERROR (sea-dsa) sanity check failed:"
                 << *(dsaCS.getValue().getInstruction()) << " requires "
                 << pkind_str << " propagation.\n";
          return false;
        }
      }
    }
  }

  LOG("dsa",
      errs() << "Sanity check succeed: global propagation completed!\n";);
  return true;
}

const Graph &
ContextSensitiveGlobalAnalysis::getGraph(const Function &fn) const {
  return *(m_graphs.find(&fn)->second);
}

Graph &ContextSensitiveGlobalAnalysis::getGraph(const Function &fn) {
  return *(m_graphs.find(&fn)->second);
}

bool ContextSensitiveGlobalAnalysis::hasGraph(const Function &fn) const {
  return m_graphs.count(&fn) > 0;
}

const Graph &
ContextSensitiveGlobalAnalysis::getSummaryGraph(const Function &fn) const {
  return *(m_bu_graphs.find(&fn)->second);
}

Graph &ContextSensitiveGlobalAnalysis::getSummaryGraph(const Function &fn) {
  return *(m_bu_graphs.find(&fn)->second);
}

bool ContextSensitiveGlobalAnalysis::hasSummaryGraph(const Function &fn) const {
  return m_bu_graphs.count(&fn) > 0;
}

///////
/// Context-sensitive analysis: bottom-up + top-down
///////
BottomUpTopDownGlobalAnalysis::BottomUpTopDownGlobalAnalysis(
    const llvm::DataLayout &dl, llvm::TargetLibraryInfoWrapperPass &tliWrapper,
    const AllocWrapInfo &allocInfo, const DsaLibFuncInfo &dsaLibFuncInfo,
    llvm::CallGraph &cg, SetFactory &setFactory, bool storeSummaryGraphs)
    : GlobalAnalysis(GlobalAnalysisKind::BUTD_CONTEXT_SENSITIVE), m_dl(dl),
      m_tliWrapper(tliWrapper), m_allocInfo(allocInfo),
      m_dsaLibFuncInfo(dsaLibFuncInfo), m_cg(cg), m_setFactory(setFactory),
      m_store_bu_graphs(storeSummaryGraphs) {}

bool BottomUpTopDownGlobalAnalysis::runOnModule(Module &M) {
  LOG("dsa-global",
      errs() << "Started bottom-up + top-down global analysis ... \n");

  for (auto &F : M) {
    if ((F.isDeclaration() || F.empty()) && !m_dsaLibFuncInfo.hasSpecFunc(F))
      continue;

    GraphRef fGraph = std::make_shared<Graph>(m_dl, m_setFactory);
    m_graphs[&F] = fGraph;
  }

  // -- Run bottom up analysis on the whole call graph: callees before
  // -- callers.
  BottomUpAnalysis bu(m_dl, m_tliWrapper, m_allocInfo, m_dsaLibFuncInfo, m_cg);
  bu.runOnModule(M, m_graphs);

  if (m_store_bu_graphs) {
    // -- store bottom-up graphs
    for (auto kv : m_graphs) {
      m_bu_graphs.insert(
          {kv.getFirst(), cloneGraph(m_dl, m_setFactory, *(kv.getSecond()))});
    }
  }

  // -- Run top down analysis on the whole call graph: callers before
  // -- callees.
  TopDownAnalysis td(m_cg);
  td.runOnModule(M, m_graphs);

  // Removing dead nodes (if any)
  for (auto &kv : m_graphs)
    kv.second->remove_dead();

  LOG(
      "dsa-global-graph", for (auto &kv
                               : m_graphs) {
        errs() << "### Global Dsa graph for " << kv.first->getName() << "\n";
        kv.second->write(errs());
        errs() << "\n";
      });

  LOG("dsa-global",
      errs() << "Finished bottom-up + top-down global analysis\n");
  return false;
}

const Graph &BottomUpTopDownGlobalAnalysis::getGraph(const Function &fn) const {
  return *(m_graphs.find(&fn)->second);
}

Graph &BottomUpTopDownGlobalAnalysis::getGraph(const Function &fn) {
  return *(m_graphs.find(&fn)->second);
}

bool BottomUpTopDownGlobalAnalysis::hasGraph(const Function &fn) const {
  return m_graphs.count(&fn) > 0;
}

const Graph &
BottomUpTopDownGlobalAnalysis::getSummaryGraph(const Function &fn) const {
  return *(m_bu_graphs.find(&fn)->second);
}

Graph &BottomUpTopDownGlobalAnalysis::getSummaryGraph(const Function &fn) {
  return *(m_bu_graphs.find(&fn)->second);
}

bool BottomUpTopDownGlobalAnalysis::hasSummaryGraph(const Function &fn) const {
  return m_bu_graphs.count(&fn) > 0;
}

///////
/// Bottom-up analysis
///////
BottomUpGlobalAnalysis::BottomUpGlobalAnalysis(
    const llvm::DataLayout &dl, llvm::TargetLibraryInfoWrapperPass &tliWrapper,
    const AllocWrapInfo &allocInfo, const DsaLibFuncInfo &dsaLibFuncInfo,
    llvm::CallGraph &cg, SetFactory &setFactory)
    : GlobalAnalysis(GlobalAnalysisKind::BU), m_dl(dl),
      m_tliWrapper(tliWrapper), m_allocInfo(allocInfo),
      m_dsaLibFuncInfo(dsaLibFuncInfo), m_cg(cg), m_setFactory(setFactory) {}

bool BottomUpGlobalAnalysis::runOnModule(Module &M) {
  LOG("dsa-global", errs() << "Started bottom-up global analysis ... \n");

  for (auto &F : M) {
    if ((F.isDeclaration() || F.empty()) && !m_dsaLibFuncInfo.hasSpecFunc(F))
      continue;

    GraphRef fGraph = std::make_shared<Graph>(m_dl, m_setFactory);
    m_graphs[&F] = fGraph;
  }

  // -- Run bottom up analysis on the whole call graph: callees before
  // -- callers.
  BottomUpAnalysis bu(m_dl, m_tliWrapper, m_allocInfo, m_dsaLibFuncInfo, m_cg);
  bu.runOnModule(M, m_graphs);

  // Removing dead nodes (if any)
  for (auto &kv : m_graphs)
    kv.second->remove_dead();

  LOG(
      "dsa-global-graph", for (auto &kv
                               : m_graphs) {
        errs() << "### Global Dsa graph for " << kv.first->getName() << "\n";
        kv.second->write(errs());
        errs() << "\n";
      });

  LOG("dsa-global", errs() << "Finished bottom-up global analysis\n");
  return false;
}

const Graph &BottomUpGlobalAnalysis::getGraph(const Function &fn) const {
  return *(m_graphs.find(&fn)->second);
}

Graph &BottomUpGlobalAnalysis::getGraph(const Function &fn) {
  return *(m_graphs.find(&fn)->second);
}

bool BottomUpGlobalAnalysis::hasGraph(const Function &fn) const {
  return m_graphs.count(&fn) > 0;
}

/// LLVM passes

ContextSensitiveGlobalPass::ContextSensitiveGlobalPass()
    : DsaGlobalPass(ID), m_ga(nullptr) {}

void ContextSensitiveGlobalPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  if (UseDsaCallGraph) {
    AU.addRequired<CompleteCallGraph>();
  } else {
    AU.addRequired<CallGraphWrapperPass>();
  }
  // dependency for immutable AllowWrapInfo
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<AllocWrapInfo>();
  AU.addRequired<DsaLibFuncInfo>();
  AU.setPreservesAll();
}

bool ContextSensitiveGlobalPass::runOnModule(Module &M) {
  auto &dl = M.getDataLayout();
  auto &tli = getAnalysis<TargetLibraryInfoWrapperPass>();
  auto &allocInfo = getAnalysis<AllocWrapInfo>();
  auto &dsaLibFuncInfo = getAnalysis<DsaLibFuncInfo>();
  allocInfo.initialize(M, this);
  dsaLibFuncInfo.initialize(M);
  CallGraph *cg = nullptr;
  if (UseDsaCallGraph) {
    cg = &getAnalysis<CompleteCallGraph>().getCompleteCallGraph();
  } else {
    cg = &getAnalysis<CallGraphWrapperPass>().getCallGraph();
  }

  m_ga.reset(new ContextSensitiveGlobalAnalysis(
      dl, tli, allocInfo, dsaLibFuncInfo, *cg, m_setFactory));
  return m_ga->runOnModule(M);
}

BottomUpTopDownGlobalPass::BottomUpTopDownGlobalPass()
    : DsaGlobalPass(ID), m_ga(nullptr) {}

void BottomUpTopDownGlobalPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  if (UseDsaCallGraph) {
    AU.addRequired<CompleteCallGraph>();
  } else {
    AU.addRequired<CallGraphWrapperPass>();
  }
  // dependency for immutable AllowWrapInfo
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<AllocWrapInfo>();
  AU.addRequired<DsaLibFuncInfo>();
  AU.setPreservesAll();
}

bool BottomUpTopDownGlobalPass::runOnModule(Module &M) {
  auto &dl = M.getDataLayout();
  auto &tli = getAnalysis<TargetLibraryInfoWrapperPass>();
  auto &allocInfo = getAnalysis<AllocWrapInfo>();
  auto &dsaLibFuncInfo = getAnalysis<DsaLibFuncInfo>();
  allocInfo.initialize(M, this);
  dsaLibFuncInfo.initialize(M);
  CallGraph *cg = nullptr;
  if (UseDsaCallGraph) {
    cg = &getAnalysis<CompleteCallGraph>().getCompleteCallGraph();
  } else {
    cg = &getAnalysis<CallGraphWrapperPass>().getCallGraph();
  }

  m_ga.reset(new BottomUpTopDownGlobalAnalysis(
      dl, tli, allocInfo, dsaLibFuncInfo, *cg, m_setFactory));
  return m_ga->runOnModule(M);
}

BottomUpGlobalPass::BottomUpGlobalPass() : DsaGlobalPass(ID), m_ga(nullptr) {}

void BottomUpGlobalPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  if (UseDsaCallGraph) {
    AU.addRequired<CompleteCallGraph>();
  } else {
    AU.addRequired<CallGraphWrapperPass>();
  }
  // dependency for immutable AllowWrapInfo
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<AllocWrapInfo>();
  AU.addRequired<DsaLibFuncInfo>();
  AU.setPreservesAll();
}

bool BottomUpGlobalPass::runOnModule(Module &M) {
  auto &dl = M.getDataLayout();
  auto &tli = getAnalysis<TargetLibraryInfoWrapperPass>();
  auto &allocInfo = getAnalysis<AllocWrapInfo>();
  auto &dsaLibFuncInfo = getAnalysis<DsaLibFuncInfo>();
  allocInfo.initialize(M, this);
  dsaLibFuncInfo.initialize(M);
  CallGraph *cg = nullptr;
  if (UseDsaCallGraph) {
    cg = &getAnalysis<CompleteCallGraph>().getCompleteCallGraph();
  } else {
    cg = &getAnalysis<CallGraphWrapperPass>().getCallGraph();
  }

  m_ga.reset(new BottomUpGlobalAnalysis(dl, tli, allocInfo, dsaLibFuncInfo, *cg,
                                        m_setFactory));
  return m_ga->runOnModule(M);
}

} // namespace seadsa

namespace seadsa {

// propagate unique scalars across callsites
void UniqueScalar::runOnCallSite(const DsaCallSite &cs, Node &calleeN,
                                 Node &callerN) {
  unsigned changed = calleeN.mergeUniqueScalar(callerN);
  if (changed & 0x01) // calleeN changed
  {
    if (const Function *fn = cs.getCallee()) {
      for (auto cs : m_dsaCG.getUses(*fn))
        m_w.enqueue(cs);
      for (auto cs : m_dsaCG.getDefs(*fn))
        m_w.enqueue(cs);
    }
  }

  if (changed & 0x02) // callerN changed
  {
    if (const Function *fn = cs.getCaller()) {
      for (auto cs : m_dsaCG.getUses(*fn))
        m_w.enqueue(cs);
      for (auto cs : m_dsaCG.getDefs(*fn))
        m_w.enqueue(cs);
    }
  }
}

// propagate allocation sites across callsites
void AllocaSite::runOnCallSite(const DsaCallSite &cs, Node &calleeN,
                               Node &callerN) {
  unsigned changed = calleeN.mergeAllocSites(callerN);
  if (changed & 0x01) // calleeN changed
  {
    if (const Function *fn = cs.getCallee()) {
      for (auto cs : m_dsaCG.getUses(*fn))
        m_w.enqueue(cs);
      for (auto cs : m_dsaCG.getDefs(*fn))
        m_w.enqueue(cs);
    }
  }

  if (changed & 0x02) // callerN changed
  {
    if (const Function *fn = cs.getCaller()) {
      for (auto cs : m_dsaCG.getUses(*fn))
        m_w.enqueue(cs);
      for (auto cs : m_dsaCG.getDefs(*fn))
        m_w.enqueue(cs);
    }
  }
}

// Quick closure implementation over a call graph's callsites
template <class GA, class Op>
bool CallGraphClosure<GA, Op>::runOnModule(Module &M) {
  for (auto it = scc_begin(&m_dsaCG.getCallGraph()); !it.isAtEnd(); ++it) {
    auto &scc = *it;
    for (CallGraphNode *cgn : scc) {
      Function *fn = cgn->getFunction();
      if (!fn || fn->isDeclaration() || fn->empty()) continue;

      for (auto &callRecord : *cgn) {

        llvm::Optional<DsaCallSite> dsaCS =
            call_graph_utils::getDsaCallSite(callRecord);
        if (!dsaCS.hasValue()) { continue; }

        if (m_ga.hasGraph(*dsaCS.getValue().getCaller()) &&
            m_ga.hasGraph(*dsaCS.getValue().getCallee())) {
          Graph &calleeG = m_ga.getGraph(*dsaCS.getValue().getCallee());
          Graph &callerG = m_ga.getGraph(*dsaCS.getValue().getCaller());
          exec_callsite(dsaCS.getValue(), calleeG, callerG);
        }
      }
    }
  }

  while (!m_w.empty()) {
    DsaCallSite dsaCS = m_w.dequeue();
    if (dsaCS.getCaller() && m_ga.hasGraph(*dsaCS.getCaller()) &&
        dsaCS.getCallee() && m_ga.hasGraph(*dsaCS.getCallee())) {
      Graph &calleeG = m_ga.getGraph(*dsaCS.getCallee());
      Graph &callerG = m_ga.getGraph(*dsaCS.getCaller());
      exec_callsite(dsaCS, calleeG, callerG);
    }
  }
  return false;
}

template <class GA, class Op>
void CallGraphClosure<GA, Op>::exec_callsite(const DsaCallSite &cs,
                                             Graph &calleeG, Graph &callerG) {
  // globals
  for (auto &kv :
       llvm::make_range(calleeG.globals_begin(), calleeG.globals_end())) {
    Cell &c = *kv.second;
    Cell &nc = callerG.mkCell(*kv.first, Cell());
    Op op(m_dsaCG, m_w);
    op.runOnCallSite(cs, *c.getNode(), *nc.getNode());
  }

  // return
  const Function &callee = *cs.getCallee();
  if (calleeG.hasRetCell(callee) && callerG.hasCell(*cs.getInstruction())) {
    Cell &c = calleeG.getRetCell(callee);
    Cell &nc = callerG.mkCell(*cs.getInstruction(), Cell());
    Op op(m_dsaCG, m_w);
    op.runOnCallSite(cs, *c.getNode(), *nc.getNode());
  }

  // actuals and formals
  DsaCallSite::const_actual_iterator AI = cs.actual_begin(),
                                     AE = cs.actual_end();
  for (DsaCallSite::const_formal_iterator FI = cs.formal_begin(),
                                          FE = cs.formal_end();
       FI != FE && AI != AE; ++FI, ++AI) {
    const Value *arg = (*AI).get();
    const Value *fml = &*FI;
    if (callerG.hasCell(*arg) && calleeG.hasCell(*fml)) {
      Cell &c = calleeG.mkCell(*fml, Cell());
      Cell &nc = callerG.mkCell(*arg, Cell());
      Op op(m_dsaCG, m_w);
      op.runOnCallSite(cs, *c.getNode(), *nc.getNode());
    }
  }
}
} // namespace seadsa

char seadsa::FlatMemoryGlobalPass::ID = 0;

char seadsa::ContextInsensitiveGlobalPass::ID = 0;

char seadsa::ContextSensitiveGlobalPass::ID = 0;

char seadsa::BottomUpTopDownGlobalPass::ID = 0;

char seadsa::BottomUpGlobalPass::ID = 0;

static llvm::RegisterPass<seadsa::FlatMemoryGlobalPass>
    U("seadsa-flat-global", "Flat memory SeaDsa analysis");

static llvm::RegisterPass<seadsa::ContextInsensitiveGlobalPass>
    X("seadsa-ci-global",
      "Context-insensitive SeaDsa analysis (for VC generation)");

static llvm::RegisterPass<seadsa::ContextSensitiveGlobalPass>
    Y("seadsa-cs-global",
      "Context-sensitive SeaDsa analysis (for VC generation)");

static llvm::RegisterPass<seadsa::BottomUpTopDownGlobalPass>
    Z("seadsa-butd-global", "Bottom-up + top-down SeaDsa analysis");

static llvm::RegisterPass<seadsa::BottomUpGlobalPass>
    W("seadsa-bu-global", "Bottom-up SeaDsa analysis");
