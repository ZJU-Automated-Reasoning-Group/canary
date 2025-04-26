#include "Alias/Dynamic/DynamicAliasAnalysis.h"
#include "Alias/Dynamic/AllocType.h"
#include "Alias/Dynamic/LogProcessor.h"

#include <cassert>
#include <llvm/ADT/SmallPtrSet.h>

using namespace llvm;

namespace dynamic {

namespace {

class AnalysisImpl : public LogProcessor<AnalysisImpl>
{
private:
    using AliasPairSet = DenseSet<AliasPair>;
    using AnalysisMap = DenseMap<DynamicPointer, AliasPairSet>;
    AnalysisMap& aliasPairMap;

    using GlobalMap = DenseMap<DynamicPointer, const void*>;
    GlobalMap globalMap;

    using PtsSet = SmallPtrSet<const void*, 4>;
    using LocalMap = DenseMap<DynamicPointer, PtsSet>;
    struct Frame
    {
        DynamicPointer func;
        LocalMap localMap;
    };
    std::vector<Frame> stackFrames;

    static bool intersects(const PtsSet&, const PtsSet&);
    void findAliasPairs();

public:
    AnalysisImpl(const char* fileName, AnalysisMap& m)
        : LogProcessor<AnalysisImpl>(fileName), aliasPairMap(m) {}

    void visitAllocRecord(const AllocRecord& allocRecord);
    void visitPointerRecord(const PointerRecord&);
    void visitEnterRecord(const EnterRecord&);
    void visitExitRecord(const ExitRecord&);
    void visitCallRecord(const CallRecord&);
};

bool AnalysisImpl::intersects(const PtsSet& lhs, const PtsSet& rhs) {
    for (auto ptr : lhs) {
        if (rhs.count(ptr))
            return true;
    }
    return false;
}

void AnalysisImpl::findAliasPairs() {
    auto func = stackFrames.back().func;
    auto& summary = aliasPairMap[func];
    auto const& localMap = stackFrames.back().localMap;

    for (auto itr = localMap.begin(), ite = localMap.end(); itr != ite; ++itr) {
        auto itr2 = itr;
        for (++itr2; itr2 != ite; ++itr2) {
            if (intersects(itr->second, itr2->second))
                summary.insert(AliasPair(itr->first, itr2->first));
        }
    }

    for (auto itr = localMap.begin(), ite = localMap.end(); itr != ite; ++itr) {
        for (auto itr2 = globalMap.begin(), ite2 = globalMap.end();
             itr2 != ite2; ++itr2) {
            if (itr->second.count(itr2->second))
                summary.insert(AliasPair(itr->first, itr2->first));
        }
    }
}

void AnalysisImpl::visitAllocRecord(const AllocRecord& allocRecord) {
    if (allocRecord.type == AllocType::Global) {
        globalMap[allocRecord.id] = allocRecord.address;
    } else {
        stackFrames.back().localMap[allocRecord.id].insert(allocRecord.address);
    }
}

void AnalysisImpl::visitPointerRecord(const PointerRecord& ptrRecord) {
    stackFrames.back().localMap[ptrRecord.id].insert(ptrRecord.address);
}

void AnalysisImpl::visitEnterRecord(const EnterRecord& enterRecord) {
    stackFrames.push_back(Frame{enterRecord.id, LocalMap()});
}

void AnalysisImpl::visitExitRecord(const ExitRecord& exitRecord) {
    if (stackFrames.back().func != exitRecord.id)
        throw std::logic_error("Function entry/exit do not match");
    findAliasPairs();
    stackFrames.pop_back();
}

void AnalysisImpl::visitCallRecord(const CallRecord& callRecord) {
    // TODO
}
}

DynamicAliasAnalysis::DynamicAliasAnalysis(const char* fileName)
    : fileName(fileName) {}

void DynamicAliasAnalysis::runAnalysis() {
    AnalysisImpl(fileName, aliasPairMap).process();
}

const DynamicAliasAnalysis::AliasPairSet* DynamicAliasAnalysis::getAliasPairs(
    DynamicPointer p) const {
    auto itr = aliasPairMap.find(p);
    if (itr == aliasPairMap.end())
        return nullptr;
    else
        return &itr->second;
}
}