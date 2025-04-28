#include "Checker/ESSS/Interval.h"
#include "Checker/ESSS/Common.h"
#include <algorithm>

using namespace std;
using namespace llvm;

void Range::dump() const {
    LOG(LOG_INFO, "[" << low << ", " << high << "]");
}

string Range::toString() const {
    return "[" + to_string(low) + ", " + to_string(high) + "]";
}

size_t Range::size() const {
    return (high - low) + 1;
}

Interval::Interval(bool empty) {
    if (!empty)
        ranges.emplace_back(Range {});
}

bool Interval::contains(int value) const {
    return any_of(ranges, [value](const Range& range) {
        return range.contains(value);
    });
}

void Interval::applyInequalityOperator(llvm::ICmpInst::Predicate predicate, int rhs) {
    if (predicate == llvm::ICmpInst::Predicate::ICMP_EQ) {
        auto shouldHave = contains(rhs);
        ranges.clear();
        if (shouldHave)
            ranges.emplace_back(Range(rhs, rhs));
    } else if (predicate == llvm::ICmpInst::Predicate::ICMP_NE) {
        removeValue(rhs);
    } else {
        for (auto rangeIt = ranges.begin(); rangeIt != ranges.end();) {
            auto& range = *rangeIt;

            bool didErase = false;

            if (predicate == llvm::ICmpInst::Predicate::ICMP_SLT || predicate == llvm::ICmpInst::Predicate::ICMP_ULT) {
                if (range.high < rhs) {
                    // OK
                } else if (range.low < rhs) { // low < rhs <= high
                    range.high = rhs - 1;
                } else { // rhs <= low <= high
                    rangeIt = ranges.erase(rangeIt);
                    didErase = true;
                }
            } else if (predicate == llvm::ICmpInst::Predicate::ICMP_SLE || predicate == llvm::ICmpInst::Predicate::ICMP_ULE) {
                if (range.high <= rhs) {
                    // OK
                } else if (range.low <= rhs) { // low <= rhs <= high
                    range.high = rhs;
                } else { // rhs < low <= high
                    rangeIt = ranges.erase(rangeIt);
                    didErase = true;
                }
            } else if (predicate == llvm::ICmpInst::Predicate::ICMP_UGT || predicate == llvm::ICmpInst::Predicate::ICMP_SGT) {
                if (range.low > rhs) {
                    // OK
                } else if (range.high > rhs) { // low <= rhs < high
                    range.low = rhs + 1;
                } else { // low <= high <= rhs
                    rangeIt = ranges.erase(rangeIt);
                    didErase = true;
                }
            } else if (predicate == llvm::ICmpInst::Predicate::ICMP_SGE || predicate == llvm::ICmpInst::Predicate::ICMP_UGE) {
                if (range.low >= rhs) {
                    // OK
                } else if (range.high >= rhs) { // low <= rhs < high
                    range.low = rhs;
                } else { // low <= high < rhs
                    rangeIt = ranges.erase(rangeIt);
                    didErase = true;
                }
            } else {
                LOG(LOG_INFO, "Predicate: " << predicate << "\n");
                assert(false && "unsupported");
            }

            if (!didErase)
                ++rangeIt;
        }
    }
}

void Interval::removeValue(int value) {
    for (auto rangeIt = ranges.begin(); rangeIt != ranges.end(); ++rangeIt) {
        auto& range = *rangeIt;
        if (range.contains(value)) {
            // 3 cases: - if it is the only element, remove the range
            //          - if it is at the end, shrink the range
            //          - otherwise, split the range in 2 parts
            if (range.hasSingleElement())
                ranges.erase(rangeIt);
            else if (range.low == value)
                ++range.low;
            else if (range.high == value)
                --range.high;
            else {
                auto oldHigh = range.high;
                range.high = value - 1;
                ranges.insert(rangeIt + 1, Range{value + 1, oldHigh});
            }

            return;
        }
    }
}

void Interval::clear() {
    ranges.clear();
}

void Interval::appendUnsafeBecauseExpectsSortMaintained(Range range) {
    ranges.emplace_back(range);
}

Interval Interval::intersection(const Interval& other) const {
    size_t first = 0;
    size_t second = 0;

    Interval intersection(true);
    while (first < ranges.size() && second < other.ranges.size()) {
        auto l = max(ranges[first].low, other.ranges[second].low);
        auto r = min(ranges[first].high, other.ranges[second].high);
        if (l <= r)
            intersection.appendUnsafeBecauseExpectsSortMaintained(Range(l, r));
        if (ranges[first].high < other.ranges[second].high)
            ++first;
        else
            ++second;
    }

    return intersection;
}

Interval Interval::union_(const Interval& other) const {
    // Fast path
    if (ranges.empty())
        return other;

    size_t first = 0;
    size_t second = 0;

    Interval union_(true);
    while (first < ranges.size() || second < other.ranges.size()) {
        Range current;

        // Act as if the ranges were put into one list and sorted by low.
        // Then follow in order mergesort-ish.
        if (second == other.ranges.size() || (first < ranges.size() && ranges[first].low < other.ranges[second].low)) {
            current = ranges[first];
            ++first;
        } else {
            current = other.ranges[second];
            ++second;
        }

        auto lastEntry = union_.ranges.end() - 1;
        if (union_.empty() || (lastEntry->high != numeric_limits<decltype(current.low)>::max() /* handle overflow */ && lastEntry->high + 1 < current.low)) {
            union_.appendUnsafeBecauseExpectsSortMaintained(current);
        } else {
            lastEntry->high = max(current.high, lastEntry->high);
        }
    }

    return union_;
}

void Interval::unionInPlace(const Interval& other) {
    auto union__ = union_(other);
    ranges = std::move(union__.ranges);
}

void Interval::intersectionInPlace(const Interval& other) {
    auto intersection_ = intersection(other);
    ranges = std::move(intersection_.ranges);
}

bool Interval::isSubsetOf(const Interval& other) const {
    auto intersection_ = intersection(other);
    return intersection_ == *this;
}

size_t Interval::size() const {
    size_t total = 0;
    for (const auto& range : ranges) {
        total += range.size();
    }
    return total;
}

void Interval::dump() const {
    if (empty()) {
        LOG(LOG_INFO, "EMPTY");
    } else {
        for (size_t i = 0; i < ranges.size(); ++i) {
            ranges[i].dump();
            if (i < ranges.size() - 1) {
                LOG(LOG_INFO, " U ");
            }
        }
    }
    LOG(LOG_INFO, "\n");
}

string Interval::toString() const {
    string result;
    size_t count = ranges.size();
    for (const auto& range : ranges) {
        result += range.toString();
        if (--count > 0) {
            result += " U ";
        }
    }
    return result;
}

Interval Interval::complement() const {
    Interval result;
    if (ranges[0].low != INT_MIN) {
        result.appendUnsafeBecauseExpectsSortMaintained(Range(INT_MIN, ranges[0].low - 1));
    }
    for (unsigned int i = 0, l = ranges.size() - 1; i < l; ++i) {
        Range inBetween{ranges[i].high + 1, ranges[i + 1].low - 1};
        result.appendUnsafeBecauseExpectsSortMaintained(inBetween);
    }
    if (ranges.back().high != INT_MAX) {
        result.appendUnsafeBecauseExpectsSortMaintained(Range(ranges.back().high + 1, INT_MAX));
    }
    return result;
}

int Interval::signedness() const {
    if (ranges.size() == 1 && ranges[0].low == 0 && ranges[0].high == ranges[0].low)
        return 0;
    if (all_of(ranges, [](const Range& range) {
        return range.high <= 0;
    })) {
        return -1;
    }
    if (all_of(ranges, [](const Range& range) {
        return range.low >= 0;
    })) {
        return 1;
    }
    return 0;
}

bool Interval::operator==(const Interval& other) const {
    // Assumes that consecutive ranges without holes are represented as one range.
    if (ranges.size() != other.ranges.size())
        return false;
    for (size_t i = 0; i < ranges.size(); ++i) {
        if (ranges[i] != other.ranges[i])
            return false;
    }
    return true;
}

int Interval::lowest() const {
    if (ranges.empty()) {
        return INT_MIN;
    }
    return ranges[0].low;
}

int Interval::highest() const {
    if (ranges.empty()) {
        return INT_MAX;
    }
    return ranges.back().high;
}
