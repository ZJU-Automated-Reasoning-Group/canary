#pragma once

#include <cassert>
#include <utility>

namespace util {

// Empty optional tag
struct nullopt_t {
    explicit constexpr nullopt_t(int) {}
};
constexpr nullopt_t nullopt{0};

// A simple replacement for std::optional
template <typename T>
class Optional {
private:
    bool has_value_;
    alignas(T) char storage_[sizeof(T)];

    T* ptr() { return reinterpret_cast<T*>(storage_); }
    const T* ptr() const { return reinterpret_cast<const T*>(storage_); }

public:
    // Default constructor
    Optional() : has_value_(false) {}

    // Nullopt constructor
    Optional(nullopt_t) : has_value_(false) {}

    // Copy constructor
    Optional(const Optional& other) : has_value_(other.has_value_) {
        if (has_value_) {
            new(storage_) T(*other.ptr());
        }
    }

    // Move constructor
    Optional(Optional&& other) noexcept : has_value_(other.has_value_) {
        if (has_value_) {
            new(storage_) T(std::move(*other.ptr()));
        }
    }

    // Value constructor
    Optional(const T& value) : has_value_(true) {
        new(storage_) T(value);
    }

    // Move value constructor
    Optional(T&& value) : has_value_(true) {
        new(storage_) T(std::move(value));
    }

    // Destructor
    ~Optional() {
        if (has_value_) {
            ptr()->~T();
        }
    }

    // Copy assignment
    Optional& operator=(const Optional& other) {
        if (this != &other) {
            if (has_value_) {
                if (other.has_value_) {
                    *ptr() = *other.ptr();
                } else {
                    ptr()->~T();
                    has_value_ = false;
                }
            } else if (other.has_value_) {
                new(storage_) T(*other.ptr());
                has_value_ = true;
            }
        }
        return *this;
    }

    // Move assignment
    Optional& operator=(Optional&& other) noexcept {
        if (this != &other) {
            if (has_value_) {
                if (other.has_value_) {
                    *ptr() = std::move(*other.ptr());
                } else {
                    ptr()->~T();
                    has_value_ = false;
                }
            } else if (other.has_value_) {
                new(storage_) T(std::move(*other.ptr()));
                has_value_ = true;
            }
        }
        return *this;
    }

    // Nullopt assignment
    Optional& operator=(nullopt_t) {
        if (has_value_) {
            ptr()->~T();
            has_value_ = false;
        }
        return *this;
    }

    // Value assignment
    Optional& operator=(const T& value) {
        if (has_value_) {
            *ptr() = value;
        } else {
            new(storage_) T(value);
            has_value_ = true;
        }
        return *this;
    }

    // Move value assignment
    Optional& operator=(T&& value) {
        if (has_value_) {
            *ptr() = std::move(value);
        } else {
            new(storage_) T(std::move(value));
            has_value_ = true;
        }
        return *this;
    }

    // Accessors
    bool has_value() const { return has_value_; }
    explicit operator bool() const { return has_value_; }

    T& value() & {
        assert(has_value_);
        return *ptr();
    }

    const T& value() const & {
        assert(has_value_);
        return *ptr();
    }

    T&& value() && {
        assert(has_value_);
        return std::move(*ptr());
    }

    const T&& value() const && {
        assert(has_value_);
        return std::move(*ptr());
    }

    // Dereference operators
    T& operator*() & {
        assert(has_value_);
        return *ptr();
    }

    const T& operator*() const & {
        assert(has_value_);
        return *ptr();
    }

    T* operator->() {
        assert(has_value_);
        return ptr();
    }

    const T* operator->() const {
        assert(has_value_);
        return ptr();
    }
};

// Helper function to create an Optional from a value
template <typename T>
Optional<typename std::decay<T>::type> make_optional(T&& value) {
    return Optional<typename std::decay<T>::type>(std::forward<T>(value));
}

} // namespace util 