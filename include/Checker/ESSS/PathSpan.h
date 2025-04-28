#pragma once

#include <vector>
#include <cstddef>
#include <type_traits>

// 前向声明，而不是直接包含头文件
namespace llvm {
    class BasicBlock;
}

using namespace llvm;

namespace esss {

// A simplified implementation of span to replace C++17 std::span
template <typename T>
class span {
private:
    T* data_;
    size_t size_;

public:
    // Constructor from pointer and size
    span(T* data, size_t size) : data_(data), size_(size) {}

    // Constructor from container (vector, array, etc.)
    template <typename Container>
    span(Container& c) : data_(c.data()), size_(c.size()) {}

    // Empty span
    span() : data_(nullptr), size_(0) {}

    // Access
    T& operator[](size_t idx) { return data_[idx]; }
    const T& operator[](size_t idx) const { return data_[idx]; }

    // Iterators
    T* begin() { return data_; }
    T* end() { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }

    // Properties
    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    T* data() { return data_; }
    const T* data() const { return data_; }
};

} // namespace esss

class PathSpanIterator {
public:
    explicit PathSpanIterator(typename std::vector<const BasicBlock*>::const_iterator it, bool isBackwardsPath)
            : it(it), isBackwardsPath(isBackwardsPath) {}

    PathSpanIterator& operator++() { // Prefix increment
        if (isBackwardsPath)
            --it;
        else
            ++it;
        return *this;
    }

    PathSpanIterator& operator--() { // Prefix decrement
        if (isBackwardsPath)
            ++it;
        else
            --it;
        return *this;
    }

    const BasicBlock* operator*() const {
        return *it;
    }

    const BasicBlock* operator->() const {
        return *it;
    }

    bool operator==(const PathSpanIterator& other) const {
        return it == other.it;
    }

    bool operator!=(const PathSpanIterator& other) const {
        return it != other.it;
    }

private:
    typename std::vector<const BasicBlock*>::const_iterator it;
    bool isBackwardsPath;
};

class PathSpan {
public:
    explicit PathSpan(const std::vector<const BasicBlock*>& blocks, bool isBackwardsPath) 
            : blocks(blocks), isBackwardsPath(isBackwardsPath) {}

    PathSpanIterator begin() const {
        if (isBackwardsPath)
            return PathSpanIterator(blocks.end() - 1, isBackwardsPath);
        else
            return PathSpanIterator(blocks.begin(), isBackwardsPath);
    }

    PathSpanIterator end() const {
        if (isBackwardsPath)
            return PathSpanIterator(blocks.begin() - 1, isBackwardsPath);
        else
            return PathSpanIterator(blocks.end(), isBackwardsPath);
    }

    std::vector<const BasicBlock*> blocks;
    bool isBackwardsPath;
};
