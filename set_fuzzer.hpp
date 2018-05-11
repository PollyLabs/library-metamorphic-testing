#ifndef SET_FUZZER_HPP
#define SET_FUZZER_HPP

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <list>
#include <tuple>
#include <string>
#include <limits>
#include <experimental/random>
#include <vector>
#include <queue>

#include "isl-noexceptions.h"

namespace set_fuzzer {

typedef struct {
    const unsigned int dims;
    const unsigned int params;
    const isl::ctx *ctx;
    const isl::space *space;
    const isl::local_space *ls;
} Parameters;

typedef struct {
    unsigned int seed;
    unsigned int max_dims;
    unsigned int max_params;
    unsigned int max_set_count;
} Arguments;

std::vector<isl::pw_aff> generate_dims(isl::local_space, isl::dim);
isl::pw_aff generate_expr(isl::pw_aff &, const Parameters &);
isl::pw_aff generate_expr(std::vector<isl::pw_aff>,
    const Parameters &);
void split_dims(const std::vector<isl::pw_aff> &,
    std::vector<isl::pw_aff> &,
    std::vector<isl::pw_aff> &);
isl::set generate_one_set(const Parameters&);
isl::set fuzz_set(Arguments = { 42, 20, 20, 10});

}

#endif
