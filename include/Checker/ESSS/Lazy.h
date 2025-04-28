#include <type_traits>
#include <functional>

#pragma once

template<typename T>
class Lazy {
private:
    using Computer = std::function<T(void)>;

public:
    explicit Lazy(Computer computer) : computer(computer) {
    }

    Lazy(Lazy&) = delete;

    ~Lazy() {
        if (computed)
            storage()->~T();
    }

    T* operator->() {
        return ptr();
    }

    T* ptr() {
        compute_if_needed();
        return storage();
    }

private:
    T* storage() {
        return reinterpret_cast<T*>(&_storage);
    }

    void compute_if_needed() {
        if (computed)
            return;
        computed = true;
        new (storage()) T(std::move(computer()));
    }

    std::aligned_storage_t<sizeof(T), alignof(T)> _storage;
    Computer computer;
    bool computed = false;
};
