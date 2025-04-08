#pragma once

#include <vector>

namespace llvm {
    class BasicBlock;
}

using namespace llvm;

// Simple span-like container for C++17
template <typename T>
class span {
public:
    using iterator = const T*;
    using const_iterator = const T*;
    
    span() : data_(nullptr), size_(0) {}
    
    span(const T* data, size_t size) : data_(data), size_(size) {}
    
    template <typename Container>
    span(const Container& c) : data_(c.data()), size_(c.size()) {}
    
    const T* data() const { return data_; }
    size_t size() const { return size_; }
    
    iterator begin() const { return data_; }
    iterator end() const { return data_ + size_; }
    
    const T& operator[](size_t idx) const { return data_[idx]; }
    
private:
    const T* data_;
    size_t size_;
};

struct PathSpanIterator {
    explicit PathSpanIterator(span<const BasicBlock* const>::iterator it, bool isBackwardsPath)
            : it(it), isBackwardsPath(isBackwardsPath) {}

    PathSpanIterator& operator++() { // Prefix increment
        if (isBackwardsPath)
            --it;
        else
            ++it;
        return *this;
    }

    PathSpanIterator operator++(int) { // Postfix increment
        PathSpanIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    PathSpanIterator& operator--() { // Prefix decrement
        if (isBackwardsPath)
            ++it;
        else
            --it;
        return *this;
    }
    
    PathSpanIterator operator--(int) { // Postfix decrement
        PathSpanIterator tmp = *this;
        --(*this);
        return tmp;
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
    span<const BasicBlock* const>::iterator it;
    bool isBackwardsPath;
};

struct PathSpan {
    explicit PathSpan(const span<const BasicBlock* const>& blocks, bool isBackwardsPath) 
        : blocks(blocks), isBackwardsPath(isBackwardsPath) {}

    [[nodiscard]] PathSpanIterator begin() const {
        if (isBackwardsPath)
            return PathSpanIterator{blocks.end() - 1, isBackwardsPath};
        else
            return PathSpanIterator{blocks.begin(), isBackwardsPath};
    }

    [[nodiscard]] PathSpanIterator end() const {
        if (isBackwardsPath)
            return PathSpanIterator{blocks.begin() - 1, isBackwardsPath};
        else
            return PathSpanIterator{blocks.end(), isBackwardsPath};
    }

    span<const BasicBlock* const> blocks;
    bool isBackwardsPath;
};
