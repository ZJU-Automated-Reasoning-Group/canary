#include "Dataflow/Mono/DataFlowResult.h"


DataFlowResult::DataFlowResult() {
  return;
}

std::set<Value *> &DataFlowResult::GEN(Instruction *inst) {
  auto &s = this->gens[inst];

  return s;
}

std::set<Value *> &DataFlowResult::KILL(Instruction *inst) {
  auto &s = this->kills[inst];

  return s;
}

std::set<Value *> &DataFlowResult::IN(Instruction *inst) {
  auto &s = this->ins[inst];

  return s;
}

std::set<Value *> &DataFlowResult::OUT(Instruction *inst) {
  auto &s = this->outs[inst];

  return s;
}
