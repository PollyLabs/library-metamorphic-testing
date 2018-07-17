#ifndef ISL_TESTER_HPP
#define ISL_TESTER_HPP

#include <iostream>
#include <map>
#include <fstream>
#include <cstring>
#include <cassert>
#include <random>

#include "isl-noexceptions.h"
#include "api_fuzzer.hpp"

#include "fmt/format.h"

namespace isl_tester {

enum Modes {
    SET_FUZZ,
    API_FUZZ,
    SET_TEST,
    SET_META_STR,
    SET_META_API,
    SET_META_NEW,
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

Arguments parseArgs(int, char **);
std::vector<std::string> gatherSets(std::string);
isl::set retrieveSet(isl::ctx, std::vector<std::string>);
std::vector<std::string> generateSetDeclFromObj(isl::set, std::string);

}

namespace set_meta_tester {
    void runSimple(std::vector<std::string>, isl_tester::Arguments&);
}

namespace set_fuzzer {
    isl::set fuzz_set(isl::ctx, const unsigned int,
        const unsigned int, const unsigned int);
}

namespace set_tester {
    int run_tests(isl::set, isl::set);
}

#endif
