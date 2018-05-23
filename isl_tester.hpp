#ifndef ISL_TESTER_HPP
#define ISL_TESTER_HPP

#include <random>

#include "isl-noexceptions.h"
#include "set_fuzzer.hpp"
#include "set_tester.hpp"
#include "set_meta_tester.hpp"

namespace isl_tester {

enum Modes {
    SET_FUZZ,
    SET_TEST,
    SET_META,
};

struct Arguments {
    unsigned int seed;
    Modes mode;
    unsigned int max_dims;
    unsigned int max_params;
    unsigned int max_set_count;

    Arguments(): seed(42), max_dims(5), max_params(5), max_set_count(3) {}
};

}

#endif
