#include <llvm/Demangle/Demangle.h>
#include "Checker/ESSS/FunctionErrorReturnIntervals.h"


void FunctionErrorReturnIntervals::mergeDestructivelyForOther(FunctionErrorReturnIntervals& other) {
    for (auto& [function, interval] : other.intervals) {
        if (auto it = intervals.find(function); it != intervals.end()) {
            it->second.unionInPlace(interval);
        } else {
            intervals.emplace(function, std::move(interval));
        }
    }
}

void FunctionErrorReturnIntervals::dump() const {
    auto intervalCount = intervals.size();
#if 1
    for (const auto& [pair, interval] : intervals) {
        auto function = pair.first;
        if (function->getParent()->getName().contains("/libc.so.bc"))
            --intervalCount;
    }
#endif

    LOG(LOG_INFO, "Function error return intervals (" << intervalCount << ", pre-libc-pruning " << intervals.size() << "):\n");
    for (const auto& [pair, interval] : intervals) {
        auto function = pair.first;
#if 1
        if (function->getParent()->getName().contains("/libc.so.bc"))
            continue;
#endif
        LOG(LOG_INFO, "Function: " << (function ? demangle(function->getName().str()) : "?") << " {return index " << pair.second << "}\n  ");
        interval.dump();
    }
}
