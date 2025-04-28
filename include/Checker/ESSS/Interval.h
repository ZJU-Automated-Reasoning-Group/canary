#pragma once

#include <vector>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Instructions.h>

enum class CheckPredicate {
    LT,
    LE,
    GT,
    GE,
    EQ,
    NE,
    NA,
};

struct Range {
    int low, high;

    Range() : low(INT32_MIN), high(INT32_MAX) {}
    Range(int low, int high) : low(low), high(high) {
        assert(low <= high);
    }

    inline bool contains(int value) const {
        return low <= value && value <= high;
    }

    bool hasSingleElement() const {
        return low == high;
    }

    void dump() const;

    std::string toString() const;

    size_t size() const;

    bool operator==(const Range& other) const {
        return low == other.low && high == other.high;
    }
    
    bool operator!=(const Range& other) const {
        return !(*this == other);
    }
};

struct IntervalHash;

class Interval {
public:
    explicit Interval(bool empty=true);

    bool contains(int value) const;
    void applyInequalityOperator(llvm::ICmpInst::Predicate predicate, int rhs);
    void removeValue(int value);
    void clear();
    inline bool empty() const { return ranges.empty(); }
    inline bool full() const {
        return ranges.size() == 1 && ranges[0].low == INT_MIN && ranges[0].high == INT_MAX;
    }
    void appendUnsafeBecauseExpectsSortMaintained(Range range);
    Interval intersection(const Interval& other) const;
    Interval union_(const Interval& other) const;
    void unionInPlace(const Interval& other);
    void intersectionInPlace(const Interval& other);
    bool isSubsetOf(const Interval& other) const;
    size_t size() const;
    void dump() const;
    void replaceWith(Interval&& other) {
        ranges = std::move(other.ranges);
    }
    int lowest() const;
    int highest() const;
    std::string toString() const;
    Interval complement() const;
    int signedness() const;

    bool operator==(const Interval& other) const;

private:
    // Invariant: this is sorted and disjoint!
    llvm::SmallVector<Range, 1> ranges;

    friend struct IntervalHash;
};

struct RangeHash {
    size_t operator()(const Range& range) const {
        return std::hash<int>()(range.low) * 104729 + std::hash<int>()(range.high);
    }
};

struct IntervalHash {
    size_t operator()(const Interval& interval) const {
        RangeHash rangeHash;
        size_t hash = 0;
        for (const auto& range : interval.ranges) {
            hash *= 5381;
            hash += rangeHash(range);
        }
        return hash;
    }
};
