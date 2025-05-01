/**
 * @file ControlDependencyGraph.cpp
 * @brief Implementation of the control dependency analysis for the PDG
 *
 * This file implements the ControlDependencyGraph pass, which analyzes control
 * dependencies between program elements. Control dependencies occur when the
 * execution of one instruction determines whether another instruction executes.
 *
 * Key features:
 * - Analysis of branch instructions and their targets
 * - Construction of post-dominator trees for control flow analysis
 * - Function-level control dependency analysis
 * - Integration with the overall PDG framework
 * - Support for different types of control dependencies
 *
 * The control dependency analysis is a fundamental component of the PDG system,
 * complementing data dependency analysis to provide a complete view of
 * program dependencies.
 */

#include "IR/PDG/ControlDependencyGraph.h"

char pdg::ControlDependencyGraph::ID = 0;

using namespace llvm;
bool pdg::ControlDependencyGraph::runOnFunction(Function &F)
{
  _PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
  addControlDepFromEntryNodeToInsts(F);
  addControlDepFromDominatedBlockToDominator(F);
  return false;
}

void pdg::ControlDependencyGraph::addControlDepFromNodeToBB(Node &n, BasicBlock &BB, EdgeType edge_type)
{
  ProgramGraph &g = ProgramGraph::getInstance();
  for (auto &inst : BB)
  {
    Node* inst_node = g.getNode(inst);
    // TODO: a special case when gep is used as a operand in load. Fix later
    if (inst_node != nullptr)
      n.addNeighbor(*inst_node, edge_type);
    // assert(inst_node != nullptr && "cannot find node for inst\n");
  }
}

void pdg::ControlDependencyGraph::addControlDepFromEntryNodeToInsts(Function &F)
{
  ProgramGraph &g = ProgramGraph::getInstance();
  FunctionWrapper* func_w = g.getFuncWrapperMap()[&F];
  if (!func_w)
    return;
  for (auto &BB : F)
  {
    addControlDepFromNodeToBB(*func_w->getEntryNode(), BB, EdgeType::CONTROLDEP_ENTRY);
  }
}

void pdg::ControlDependencyGraph::addControlDepFromDominatedBlockToDominator(Function &F)
{
  ProgramGraph &g = ProgramGraph::getInstance();
  for (auto &BB : F)
  {
    for (auto succ_iter = succ_begin(&BB); succ_iter != succ_end(&BB); succ_iter++)
    {
      BasicBlock *succ_bb = *succ_iter;
      if (&BB == &*succ_bb || !_PDT->dominates(&*succ_bb, &BB))
      {
        // get terminator and connect with the dependent block
        Instruction *terminator = BB.getTerminator();
        if (BranchInst *bi = dyn_cast<BranchInst>(terminator))
        {
          if (!bi->isConditional() || !bi->getCondition())
            break;
          // Node *cond_node = g.getNode(*bi->getCondition());
          // if (!cond_node)
          //   break;
          Node *branch_node = g.getNode(*bi);
          if (branch_node == nullptr)
            break;
          BasicBlock *nearestCommonDominator = _PDT->findNearestCommonDominator(&BB, succ_bb);
          if (nearestCommonDominator == &BB)
            addControlDepFromNodeToBB(*branch_node, *succ_bb, EdgeType::CONTROLDEP_BR);

          for (auto *cur = _PDT->getNode(&*succ_bb); cur != _PDT->getNode(nearestCommonDominator); cur = cur->getIDom())
          {
            addControlDepFromNodeToBB(*branch_node, *cur->getBlock(), EdgeType::CONTROLDEP_BR);
          }
        }
      }
    }
  }
}

void pdg::ControlDependencyGraph::getAnalysisUsage(AnalysisUsage &AU) const
{
  AU.addRequired<PostDominatorTreeWrapperPass>();
  AU.setPreservesAll();
}

static RegisterPass<pdg::ControlDependencyGraph>
    CDG("cdg", "Control Dependency Graph Construction", false, true);
