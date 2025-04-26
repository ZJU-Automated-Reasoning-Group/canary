#pragma once

namespace llvm {
class Module;
}

namespace dynamic {

class MemoryInstrument
{
private:
public:
    MemoryInstrument() = default;

    void runOnModule(llvm::Module&);
};
}
