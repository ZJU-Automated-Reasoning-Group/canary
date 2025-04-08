#ifndef COMMON_H
#define COMMON_H

#include <llvm/IR/Module.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/IR/DebugInfo.h>

#include <unistd.h>
//#include <chrono>
#include <optional>

using namespace llvm;
using namespace std;

#define LOG_INFO 0
#define LOG_VERBOSE 1

#define LOG(lv, stmt)                            \
    do {                                            \
        if (VerboseLevel >= lv)                        \
        errs() << stmt;                            \
    } while(0)


#define OP llvm::errs()

extern cl::opt<unsigned> VerboseLevel;
extern const DataLayout *CurrentLayout;

//
// Common functions
//

string getFileName(DILocation *Loc,
                   DISubprogram *SP = nullptr);

StringRef getCalledFuncName(Instruction *I);

size_t funcHash(const Function* F, StringRef name);

size_t funcHash(const Function *F, bool withName = true);

size_t callHash(CallInst *CI);

size_t typeHash(const Type *Ty);
size_t typeHash(const Type *Ty, const DataLayout &DL);
size_t typeHash(const Type *Ty, const DataLayout *DL);

size_t typeIdxHash(const Type *Ty, int Idx = -1);
size_t typeIdxHash(const Type *Ty, const DataLayout &DL, int Idx = -1);
size_t typeIdxHash(const Type *Ty, const DataLayout *DL, int Idx = -1);

size_t hashIdxHash(size_t Hs, int Idx = -1);

string HandleSimpleTy(const Type *Ty);

string expand_struct(const StructType *STy);

struct SourceLocation {
    explicit SourceLocation(const Instruction *v, bool followInlines=true) : v(v) {
        const auto &dbgLoc = v->getDebugLoc();
        auto diNode = dbgLoc.get();
        if (diNode && dbgLoc.getLine() >= 1) {
            SCheckFileName = diNode->getFilename();
            SCheckLineNo = dbgLoc.getLine();
        }

        raw_string_ostream output(cachedOutputString);
        if (!isValid()) {
            // Note: printing the LLVM instruction is very slow!
            //string tmp;
            //raw_string_ostream rso(tmp);
            //v->print(rso);
            //string_view view(tmp);
            //output << "Unknown source code location, IR instruction: <" << view.substr(2, view.size() - 1) << ">";
            if (auto subProgram = v->getFunction()->getSubprogram()) {
                output << subProgram->getFilename() << ": ~" << subProgram->getLine() << "~";
            } else {
                output << "?: ?";
            }
        } else {
            output << SCheckFileName << ": " << SCheckLineNo;
            if (followInlines && (diNode = diNode->getInlinedAt())) {
                output << " (inlined at " << diNode->getFilename() << ": " << diNode->getLine() << ")";
            }
        }
        output.flush();
    }

    bool operator==(const SourceLocation &b) const {
        if (SCheckLineNo == 0) {
            return v == b.v;
        } else {
            return SCheckLineNo == b.SCheckLineNo && SCheckFileName == b.SCheckFileName;
        }
    }

    [[nodiscard]] bool isValid() const {
        return SCheckLineNo > 0;
    }

    [[nodiscard]] bool isValidProgramSource() const {
        return isValid() || v->getFunction()->getSubprogram();
    }

    void dumpToStringStream(raw_string_ostream &output) const {
        output << cachedOutputString;
    }

    void dump(unsigned int level = LOG_INFO) const {
        if (VerboseLevel >= level) {
            OP << cachedOutputString;
        }
    }

    StringRef SCheckFileName; /* Source file name of security check */
    unsigned SCheckLineNo{};  /* Line number of security check */

private:
    const Instruction *v;
    string cachedOutputString;
};

struct SourceLocationHasher {
    size_t operator()(const SourceLocation& sourceLocation) const {
        hash<unsigned> hl;
        return hl(sourceLocation.SCheckLineNo) * 6700417 + hash_value(sourceLocation.SCheckFileName);
    }
};

#endif
