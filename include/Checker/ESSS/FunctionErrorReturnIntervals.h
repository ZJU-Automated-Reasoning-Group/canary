#pragma once

#include "Interval.h"
#include "Common.h"
#include <map>

enum IntervalSource {

};


class FunctionErrorReturnIntervals {
private:
    using Map = map<pair<const Function*, unsigned int>, Interval>;

public:
    inline void insertIntervalFor(pair<const Function*, unsigned int> function, Interval&& interval) {
        intervals.emplace(function, std::move(interval));
    }

    inline void replaceIntervalFor(pair<const Function*, unsigned int> function, Interval&& interval) {
        intervals[function] = std::move(interval);
    }

    inline Interval& intervalFor(pair<const Function*, unsigned int> function, bool emptyByDefault=false) {
        auto intervalIt = intervals.find(function);
        if (intervalIt == intervals.end())
            return intervals.insert({function, Interval{emptyByDefault}}).first->second;
        return intervalIt->second;
    }

    inline llvm::Optional<const Interval*> maybeIntervalFor(pair<const Function*, unsigned int> function) const {
        auto intervalIt = intervals.find(function);
        if (intervalIt == intervals.end())
            return llvm::Optional<const Interval*>();
        return &intervalIt->second;
    }

    inline bool empty() const {
        return intervals.empty();
    }

    inline size_t size() const {
        return intervals.size();
    }

    void mergeDestructivelyForOther(FunctionErrorReturnIntervals& other);

    void dump() const;

    Map::const_iterator begin() const {
        return intervals.begin();
    }

    Map::const_iterator end() const {
        return intervals.end();
    }

    Map::iterator begin() {
        return intervals.begin();
    }

    Map::iterator end() {
        return intervals.end();
    }

    Map::iterator erase(Map::iterator it) {
        return intervals.erase(it);
    }

private:
    Map intervals;
};
