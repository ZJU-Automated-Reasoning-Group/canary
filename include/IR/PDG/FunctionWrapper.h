#ifndef FUNCTIONWRAPPER_H_
#define FUNCTIONWRAPPER_H_
#include "IR/PDG/LLVMEssentials.h"
#include "IR/PDG/Tree.h"
#include "IR/PDG/PDGUtils.h"

namespace pdg
{
  class FunctionWrapper
  {
  public:
    // Define map types with full template parameters
    typedef std::map<llvm::Argument *, Tree *, std::less<llvm::Argument *>, std::allocator<std::pair<llvm::Argument * const, Tree *>>> ArgTreeMap;
    
    FunctionWrapper(llvm::Function *func)
    {
      _func = func;
      for (auto arg_iter = _func->arg_begin(); arg_iter != _func->arg_end(); arg_iter++)
      {
        _arg_list.push_back(&*arg_iter);
      }
      _entry_node = new Node(GraphNodeType::FUNC_ENTRY);
      _entry_node->setFunc(*func);
    }
    llvm::Function *getFunc() const { return _func; }
    Node *getEntryNode() { return _entry_node; }
    void addInst(llvm::Instruction &i);
    void buildFormalTreeForArgs();
    void buildFormalTreesForRetVal();
    void setClassName(std::string _cls_name) { _class_name = _cls_name; }
    std::string getClassName() { return _class_name; }
    llvm::DIType *getArgDIType(llvm::Argument &arg);
    llvm::DILocalVariable *getArgDILocalVar(llvm::Argument &arg);
    llvm::AllocaInst *getArgAllocaInst(llvm::Argument &arg);
    Tree *getArgFormalInTree(llvm::Argument &arg);
    Tree *getArgFormalOutTree(llvm::Argument &arg);
    Tree *getRetFormalInTree() { return _ret_val_formal_in_tree; }
    Tree *getRetFormalOutTree() { return _ret_val_formal_out_tree; }
    ArgTreeMap &getArgFormalInTreeMap() { return _arg_formal_in_tree_map; }
    ArgTreeMap &getArgFormalOutTreeMap() { return _arg_formal_out_tree_map; }
    std::vector<llvm::AllocaInst *> &getAllocInsts() { return _alloca_insts; }
    std::vector<llvm::DbgDeclareInst *> &getDbgDeclareInsts() { return _dbg_declare_insts; }
    std::vector<llvm::LoadInst *> &getLoadInsts() { return _load_insts; }
    std::vector<llvm::StoreInst *> &getStoreInsts() { return _store_insts; }
    std::vector<llvm::CallInst *> &getCallInsts() { return _call_insts; }
    std::vector<llvm::ReturnInst *> &getReturnInsts() { return _return_insts; }
    std::vector<llvm::Argument *> &getArgList() { return _arg_list; }
    bool hasNullRetVal() { return (_ret_val_formal_in_tree == nullptr); }

  private:
    Node *_entry_node;
    std::string _class_name;
    llvm::Function *_func;
    std::vector<llvm::AllocaInst *> _alloca_insts;
    std::vector<llvm::DbgDeclareInst *> _dbg_declare_insts;
    std::vector<llvm::LoadInst *> _load_insts;
    std::vector<llvm::StoreInst *> _store_insts;
    std::vector<llvm::CallInst *> _call_insts;
    std::vector<llvm::ReturnInst *> _return_insts;
    std::vector<llvm::Argument *> _arg_list;
    ArgTreeMap _arg_formal_in_tree_map;
    ArgTreeMap _arg_formal_out_tree_map;
    Tree *_ret_val_formal_in_tree;
    Tree *_ret_val_formal_out_tree;

  };
} // namespace pdg

#endif