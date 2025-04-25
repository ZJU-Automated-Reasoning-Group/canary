#include "Support/API.h"
#include <llvm/IR/Instructions.h>
#include <set>
#include <string>

std::set<std::string> API::HeapAllocFunctions = {
    "malloc",
    "calloc",
    "memalign",
    "aligned_alloc",
    "pvalloc",
    "valloc",
    "strdup",
    "strndup",
    "kmalloc",
    "mmap",
    "mmap64",
    "get_current_dir_name",
    "_Znwj",               // operator new(unsigned int)
    "_Znwm",               // operator new(unsigned long)
    "_Znaj",               // operator new[](unsigned int)
    "_Znam",               // operator new[](unsigned long)
    "_ZnwjRKSt9nothrow_t", // operator new(unsigned int)
    "_ZnwmRKSt9nothrow_t", // operator new(unsigned long)
    "_ZnajRKSt9nothrow_t", // operator new[](unsigned int)
    "_ZnamRKSt9nothrow_t", // operator new[](unsigned long)
    "realloc",
    "reallocf",
    "getline",
    "getwline",
    "getdelim",
    "getwdelim",
};

bool API::isMemoryAllocate(Instruction *I) {
  return isHeapAllocate(I) || isStackAllocate(I);
}

bool API::isHeapAllocate(Instruction *I) {
  if (auto CI = dyn_cast<CallInst>(I)) {
    if (auto Callee = CI->getCalledFunction()) {
      return HeapAllocFunctions.count(Callee->getName().str());
    }
  }
  return false;
}

bool API::isStackAllocate(Instruction *I) { return isa<AllocaInst>(I); }