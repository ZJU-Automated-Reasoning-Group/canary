#ifndef QUEUE_H
#define QUEUE_H

#include "Support/SystemHeaders.h"

namespace llvm {

class Queue {
public:
  std::unordered_map<int, int> queueSizeToIndex;
  std::vector<Type *> queueElementTypes;
  std::vector<Function *> queuePushes;
  std::vector<Function *> queuePops;
  std::vector<Type *> queueTypes;
};

} // namespace llvm

#endif
