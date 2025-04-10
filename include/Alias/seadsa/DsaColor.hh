#pragma once

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Function.h"

#include "Alias/seadsa/CallSite.hh"

using namespace seadsa;
using namespace llvm;

namespace seadsa {

using Color = unsigned long long; // at least 64 bit, needed 48

class Graph;
class Node;

using ColorMap = DenseMap<const Node *, Color>;
using NodeSet = DenseSet<const Node *>;

void colorGraphsCallSite(const DsaCallSite &cs, const Graph &calleeG,
                const Graph &callerG, ColorMap &color_callee,
                ColorMap &color_caller);

void colorGraphsFunction(const Function &f, const Graph &fromG,
                         const Graph &toG, ColorMap &colorFrom,
                         ColorMap &colorTo);

} // end namespace seadsa
