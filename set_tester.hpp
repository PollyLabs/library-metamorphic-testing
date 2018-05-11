#ifndef SET_TESTER_HPP
#define SET_TESTER_HPP

#include <random>

#include "isl-noexceptions.h"

namespace set_tester {

void run_unary_tests(isl::set);
int run_tests(isl::set, isl::set, isl::set);

}

#endif

