#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <fstream>
#include <regex>

#include "Checker/ESSS/Common.h"

bool trimPathSlash(string& path, int slash) {
    while (slash > 0) {
        path = path.substr(path.find('/') + 1);
        --slash;
    }

    return true;
}

string getFileName(DILocation* Loc, DISubprogram* SP) {
    string FN;
    if (Loc)
        FN = Loc->getFilename().str();
    else if (SP)
        FN = SP->getFilename().str();
    else
        return "";

    // TODO: require config
    int slashToTrim = 2;
    trimPathSlash(FN, slashToTrim);
    return FN;
}

/// Get called function name of V.
StringRef getCalledFuncName(Instruction* I) {

    Value* V = nullptr;
    if (auto CI = dyn_cast<CallInst>(I))
        V = CI->getCalledOperand();
    else if (auto II = dyn_cast<InvokeInst>(I))
        V = II->getCalledOperand();
    assert(V != nullptr);

    auto IA = dyn_cast<InlineAsm>(V);
    if (IA)
        return {IA->getAsmString()};

    User* UV = dyn_cast<User>(V);
    if (UV) {
        if (UV->getNumOperands() > 0) {
            Value* VUV = UV->getOperand(0);
            return VUV->getName();
        }
    }

    return V->getName();
}

static string removeDigits(const string& input) {
    static regex nrRegex("\\.[0-9]+");
    return regex_replace(input, nrRegex, "");
}

static string signatureStringWithoutCleanup(const Type* signatureType) {
    string output;

    string sig;
    raw_string_ostream rso(sig);
    signatureType->print(rso);
    output = rso.str();
    return output;
}

static string cleanup(string& output) {
    string::iterator end_pos = remove(output.begin(),
                                      output.end(), ' ');
    output.erase(end_pos, output.end());
    return removeDigits(output);
}

static string signatureString(const Type* signatureType) {
    auto output = signatureStringWithoutCleanup(signatureType);
    return cleanup(output);
}

size_t funcHash(const Function* F, StringRef name) {
    string output = signatureString(F->getFunctionType());
    if (!name.empty()) {
        output += name;
    }
    output = cleanup(output);
    hash<string> str_hash;
    return str_hash(output);
}

size_t funcHash(const Function* F, bool withName) {
    return funcHash(F, withName ? F->getName() : StringRef {});
}

size_t callHash(CallInst* CI) {
    Function* CF = CI->getCalledFunction();

    if (CF)
        return funcHash(CF);
    else {
        hash<string> str_hash;
        return str_hash(signatureString(CI->getFunctionType()));
    }
}

string HandleSimpleTy(const Type* Ty) {
    unsigned size = Ty->getScalarSizeInBits();
    return to_string(size);
}

static void addTypeClass(string& output, Type* type) {
    if (type->isPointerTy()) {
        auto pointerType = dyn_cast<PointerType>(type);
        output += "P";
        addTypeClass(output, pointerType->getPointerElementType());
        output += ",";
    } else if (type->isIntegerTy())
        output += to_string(CurrentLayout->getTypeSizeInBits(type)) + "I,";
    else if (type->isFunctionTy()) {
        auto functionType = dyn_cast<FunctionType>(type);
        output += to_string(functionType->getNumParams()) + "F,";
    } else if (type->isSized())
        output += to_string(CurrentLayout->getTypeSizeInBits(type)) + ",";
    else
        output += "U,";
}

string expand_struct(const StructType* STy) {
    // TODO: Handle opaque structures hashing better
    auto typeSize = STy->isOpaque() ? 0 : CurrentLayout->getStructLayout(const_cast<StructType*>(STy))->getSizeInBits();
    auto numElements = STy->getNumElements();
    auto str = to_string(typeSize) + "," + to_string(numElements);
    for (unsigned int i = 0; i < numElements; ++i) {
        addTypeClass(str, STy->getStructElementType(i));
    }
    return str;
}


size_t typeHash(const Type* Ty) {
    string ty_str;
    if (auto STy = dyn_cast<StructType>(Ty)) {
        ty_str = expand_struct(STy);
    } else {
        ty_str = HandleSimpleTy(Ty);
    }

    hash<string> str_hash;
    return str_hash(ty_str);
}

size_t typeHash(const Type* Ty, const DataLayout &DL) {
    // Just delegate to the existing implementation since DL isn't used directly
    return typeHash(Ty);
}

size_t typeHash(const Type* Ty, const DataLayout *DL) {
    // Just forward to the reference version if DL is not null
    if (DL)
        return typeHash(Ty, *DL);
    return typeHash(Ty);
}

size_t hashIdxHash(size_t Hs, int Idx) {
    hash<string> str_hash;
    return Hs + str_hash(to_string(Idx));
}

size_t typeIdxHash(const Type* Ty, int Idx) {
    return hashIdxHash(typeHash(Ty), Idx);
}

size_t typeIdxHash(const Type* Ty, const DataLayout &DL, int Idx) {
    // Just delegate to the existing implementation
    return typeIdxHash(Ty, Idx);
}

size_t typeIdxHash(const Type* Ty, const DataLayout *DL, int Idx) {
    // Just forward to the reference version if DL is not null
    if (DL)
        return typeIdxHash(Ty, *DL, Idx);
    return typeIdxHash(Ty, Idx);
}
