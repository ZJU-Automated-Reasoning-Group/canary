#ifndef CALL_GRAPH_H
#define CALL_GRAPH_H

#include "Analyzer.h"

class CallGraphPass : public IterativeModulePass {

	private:
        // Functions whose addresses are taken.
        FuncSet AddressTakenFuncs;
		const DataLayout *DL;
		// char * or void *
		Type *Int8PtrTy;
		// long interger type
		Type *IntPtrTy;

		static DenseMap<size_t, FuncSet>typeFuncsMap;
		static unordered_map<size_t, unordered_set<size_t>>typeConfineMap;
		static unordered_map<size_t, unordered_set<size_t>>typeTransitMap;
		static unordered_set<size_t>typeEscapeSet;

		// Use type-based analysis to find targets of indirect calls
		void findCalleesWithType(llvm::CallInst*, FuncSet&);

        bool collectFunctions(const Value* value, SmallPtrSet<const Function*, 1>& out, set<const Value*>& visited) const;

        void generateDotGraphOutput(const Function* F, int depth) const;

		void unrollLoops(Function *F);

		bool typeConfineInInitializer(User *Ini);
		bool typeConfineInStore(StoreInst *SI);
		bool typeConfineInCast(CastInst *CastI);
		void escapeType(Type *Ty, const StoreInst* SI, int Idx = -1);
		void transitType(Type *ToTy, Type *FromTy,
				int ToIdx = -1, int FromIdx = -1);

		Value *nextLayerBaseType(Value *V, Type * &BTy, int &Idx,
				const DataLayout *DL);

		void funcSetIntersection(FuncSet &FS1, FuncSet &FS2,
				FuncSet &FS); 
		bool findCalleesWithMLTA(CallInst *CI, FuncSet &FS);

        void visitMetadata(const MDNode* mdNode);
        static string computeDebugTag(const DISubroutineType* subroutineType);

	public:
		CallGraphPass(GlobalContext *Ctx_)
			: IterativeModulePass(Ctx_, "CallGraph") { }

		virtual bool doInitialization(llvm::Module *);
		virtual bool doFinalization(llvm::Module *);
		virtual void doModulePass(llvm::Module *);

        DenseSet<const MDNode*> metadata;
        DenseMap<pair<StringRef, unsigned int>, string> typeAndOffsetToDebugTags;
};

#endif
