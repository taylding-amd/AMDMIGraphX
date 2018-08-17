#ifndef MIGRAPH_GUARD_MIGRAPHLIB_GENERATE_HPP
#define MIGRAPH_GUARD_MIGRAPHLIB_GENERATE_HPP

#include <migraph/argument.hpp>
#include <migraph/literal.hpp>
#include <random>

namespace migraph {

template <class T>
struct xorshf96_generator
{
    unsigned long x = 123456789;
    unsigned long y = 362436069;
    unsigned long z = 521288629;

    constexpr T operator()()
    {
        x ^= x << 16;
        x ^= x >> 5;
        x ^= x << 1;

        unsigned long t = x;
        x               = y;
        y               = z;
        z               = t ^ x ^ y;

        return z;
    }
};

template <class T>
std::vector<T> generate_tensor_data(migraph::shape s, std::mt19937::result_type)
{
    std::vector<T> result(s.elements());
    std::generate(result.begin(), result.end(), xorshf96_generator<T>{});
    return result;
}

argument generate_argument(shape s, std::mt19937::result_type seed = 0);

literal generate_literal(shape s, std::mt19937::result_type seed = 0);

} // namespace migraph

#endif
