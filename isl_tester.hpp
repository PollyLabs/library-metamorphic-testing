#ifndef ISL_TESTER_HPP
#define ISL_TESTER_HPP

#include <iostream>
#include <fstream>
#include <cstring>
#include <cassert>
#include <random>

#include "isl-noexceptions.h"

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
    std::string input_sets;

    Arguments(): seed(42), max_dims(5), max_params(5), max_set_count(3), input_sets("") {}
};

Arguments parse_args(int, char **);
std::vector<std::string> gather_sets(std::string);
isl::set retrieve_set(isl::ctx, std::vector<std::string>);

}

namespace set_meta_tester {
    void run_simple(isl::set, isl_tester::Arguments &);
}

namespace set_fuzzer {
    isl::set fuzz_set(isl::ctx, const unsigned int,
        const unsigned int, const unsigned int);
}

namespace set_tester {
    int run_tests(isl::set, isl::set);
}

#endif
