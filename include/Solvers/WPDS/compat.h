#ifndef WPDS_COMPAT_H
#define WPDS_COMPAT_H

// Compatibility shims for modern C++ standards
// These definitions replace the deprecated std::unary_function and std::binary_function

namespace std {
    // Provide unary_function and binary_function for all C++ standards
    // since they've been removed in C++17
    template<typename Arg, typename Result>
    struct unary_function {
        typedef Arg argument_type;
        typedef Result result_type;
    };

    // Replacement for std::binary_function
    template<typename Arg1, typename Arg2, typename Result>
    struct binary_function {
        typedef Arg1 first_argument_type;
        typedef Arg2 second_argument_type;
        typedef Result result_type;
    };
}

#endif // WPDS_COMPAT_H 