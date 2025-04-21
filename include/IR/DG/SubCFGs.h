#ifndef DG_SUBCFGS_H_
#define DG_SUBCFGS_H_

#include "llvm/Support/GraphWriter.h"
#include "IR/DG/DGBase.h"


/*
 * Execution Graph.
 */
class SubCFGs : public DG<BasicBlock> {
public:
  /*
   * Add all basic blocks included in the module M as nodes.
   */
  SubCFGs(Module &M) {
    for (auto &F : M) {
      for (auto &B : F) {
        addNode(&B, true);
      }
    }

    connectBasicBlockNodes();
  }

  /*
   * Add all basic blocks included in the function F as nodes.
   */
  SubCFGs(Function &F) {
    for (auto &B : F) {
      addNode(&B, true);
    }

    connectBasicBlockNodes();
  }

  /*
   * Add all basic blocks included in the loop only.
   */
  SubCFGs(Loop *loop) {
    for (auto &B : loop->blocks()) {
      addNode(B, true);
    }

    connectBasicBlockNodes();
  }

  /*
   * Add all basic blocks given
   */
  SubCFGs(std::set<BasicBlock *> &bbs) {
    for (auto B : bbs) {
      addNode(B, true);
    }

    connectBasicBlockNodes();
  }

private:
  void connectBasicBlockNodes() {
    std::set<DGNode<BasicBlock> *> nodes(begin_nodes(), end_nodes());
    for (auto node : nodes) {
      auto bb = node->getT();
      for (auto succBB : successors(bb)) {
        fetchOrAddNode(succBB, false);
        addUndefinedDependenceEdge(bb, succBB);
      }
    }
  }
};

#endif // DG_SUBCFGS_H_
