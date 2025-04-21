
#ifndef DATAFLOW_DATAFLOWANALYSIS_H_
#define DATAFLOW_DATAFLOWANALYSIS_H_

#include "Support/SystemHeaders.h"
#include "Dataflow/DataFlowResult.h"

class DataFlowAnalysis {
public:
  /*
   * Methods
   */
  DataFlowAnalysis();

  DataFlowResult *runReachableAnalysis(Function *f);

  DataFlowResult *runReachableAnalysis(
      Function *f,
      std::function<bool(Instruction *i)> filter);

  DataFlowResult *getFullSets(Function *f);
};

#endif // DATAFLOW_DATAFLOWANALYSIS_H_
