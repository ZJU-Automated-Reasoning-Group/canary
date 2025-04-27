
#include "Dataflow/Mono/DataFlow.h"


DataFlowAnalysis::DataFlowAnalysis() {
  return;
}

DataFlowResult *DataFlowAnalysis::getFullSets(Function *f) {

  auto df = new DataFlowResult{};
  for (auto &inst : instructions(*f)) {
    auto &inSetOfInst = df->IN(&inst);
    auto &outSetOfInst = df->OUT(&inst);
    for (auto &inst2 : instructions(*f)) {
      inSetOfInst.insert(&inst2);
      outSetOfInst.insert(&inst2);
    }
  }

  return df;
}

DataFlowResult *DataFlowAnalysis::runReachableAnalysis(
    Function *f,
    std::function<bool(Instruction *i)> filter) {

  /*
   * Allocate the engine
   */
  auto dfa = DataFlowEngine{};

  /*
   * Define the data-flow equations
   */
  auto computeGEN = [filter](Instruction *i, DataFlowResult *df) {
    /*
     * Check if the instruction should be considered.
     */
    if (!filter(i)) {
      return;
    }

    /*
     * Add the instruction to the GEN set.
     */
    auto &gen = df->GEN(i);
    gen.insert(i);

    return;
  };
  auto computeKILL = [](Instruction *, DataFlowResult *) { return; };
  auto computeOUT = [](Instruction *inst,
                       Instruction *succ,
                       std::set<Value *> &OUT,
                       DataFlowResult *df) {
    auto &inS = df->IN(succ);
    OUT.insert(inS.begin(), inS.end());
    return;
  };
  auto computeIN =
      [](Instruction *inst, std::set<Value *> &IN, DataFlowResult *df) {
        auto &genI = df->GEN(inst);
        auto &outI = df->OUT(inst);

        /*
         * IN[i] = GEN[i] U OUT[i]
         */
        IN.insert(genI.begin(), genI.end());
        IN.insert(outI.begin(), outI.end());

        return;
      };

  /*
   * Run the data flow analysis needed to identify the instructions that could
   * be executed from a given point.
   */
  auto df =
      dfa.applyBackward(f, computeGEN, computeKILL, computeIN, computeOUT);

  return df;
}

DataFlowResult *DataFlowAnalysis::runReachableAnalysis(Function *f) {

  /*
   * Create the function that doesn't filter out instructions.
   */
  auto noFilter = [](Instruction *i) -> bool { return true; };

  /*
   * Run the analysis
   */
  auto dfr = this->runReachableAnalysis(f, noFilter);

  return dfr;
}

