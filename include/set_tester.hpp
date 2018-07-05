#ifndef SET_TESTER_HPP
#define SET_TESTER_HPP

#include <random>
#include <sys/resource.h>

#include "isl-noexceptions.h"

namespace set_tester {

void run_unary_tests(isl::set);
void run_binary_tests(isl::set, isl::set);
int run_tests(isl::set, isl::set);

}

#endif

