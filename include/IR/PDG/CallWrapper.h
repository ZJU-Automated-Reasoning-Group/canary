#ifndef CALLWRAPPER_H_
#define CALLWRAPPER_H_
#include "IR/PDG/LLVMEssentials.h"
#include "IR/PDG/Tree.h"
#include "IR/PDG/PDGUtils.h"
#include "IR/PDG/FunctionWrapper.h"

namespace pdg
{
  class CallWrapper
  {
    private:
      // Define map type with full template parameters
      typedef std::map<llvm::Value *, Tree *, std::less<llvm::Value *>, std::allocator<std::pair<llvm::Value * const, Tree *>>> ValueTreeMap;
      
      llvm::CallInst* _call_inst;
      llvm::Function* _called_func;
      std::vector<llvm::Value *> _arg_list;
      ValueTreeMap _arg_actual_in_tree_map;
      ValueTreeMap _arg_actual_out_tree_map;
      Tree * _ret_val_actual_in_tree;
      Tree * _ret_val_actual_out_tree;
      bool _has_param_trees = false;

    public:
      CallWrapper(llvm::CallInst& ci)
      {
        _call_inst = &ci;
        _called_func = pdgutils::getCalledFunc(ci);
        for (auto arg_iter = ci.arg_begin(); arg_iter != ci.arg_end(); arg_iter++)
        {
          _arg_list.push_back(*arg_iter);
        }
      }
      void buildActualTreeForArgs(FunctionWrapper &callee_fw);
      void buildActualTreesForRetVal(FunctionWrapper &callee_fw);
      llvm::CallInst *getCallInst() { return _call_inst; }
      llvm::Function *getCalledFunc() { return _called_func; }
      std::vector<llvm::Value *> &getArgList() { return _arg_list; }
      Tree *getArgActualInTree(llvm::Value &actual_arg);
      Tree *getArgActualOutTree(llvm::Value &actual_arg);
      Tree *getRetActualInTree() { return _ret_val_actual_in_tree; }
      Tree *getRetActualOutTree() { return _ret_val_actual_out_tree; }
      bool hasNullRetVal() { return (_ret_val_actual_in_tree == nullptr); }
      bool hasParamTrees() { return _has_param_trees; }
      void setHasParamTrees() { _has_param_trees = true; }
  };
}

#endif