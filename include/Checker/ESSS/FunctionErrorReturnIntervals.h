#pragma once

#include "Checker/ESSS/Interval.h"
#include "Checker/ESSS/Common.h"
#include <map>
#include <optional>


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

    [[nodiscard]] inline optional<const Interval*> maybeIntervalFor(pair<const Function*, unsigned int> function) const {
        auto intervalIt = intervals.find(function);
        if (intervalIt == intervals.end())
            return {};
        return &intervalIt->second;
    }

    [[nodiscard]] inline bool empty() const {
        return intervals.empty();
    }

    [[nodiscard]] inline size_t size() const {
        return intervals.size();
    }

    void mergeDestructivelyForOther(FunctionErrorReturnIntervals& other);

    void dump() const;

    [[nodiscard]] Map::const_iterator begin() const {
        return intervals.begin();
    }

    [[nodiscard]] Map::const_iterator end() const {
        return intervals.end();
    }

    [[nodiscard]] Map::iterator begin() {
        return intervals.begin();
    }

    [[nodiscard]] Map::iterator end() {
        return intervals.end();
    }

    Map::iterator erase(Map::iterator it) {
        return intervals.erase(it);
    }

private:
    Map intervals;
};
