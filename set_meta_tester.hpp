#ifndef SET_META_TESTER_HPP
#define SET_META_TESTER_HPP

#include "isl-noexceptions.h"
#include "yaml-cpp/yaml.h"

#include <cassert>
#include <cstring>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <iostream>
#include <random>

namespace set_meta_tester {

void write_line(std::stringstream &, std::string);
void prepare_header(std::stringstream &);
void main_pre_setup(std::stringstream &);
void gen_var_declarations(std::stringstream &, isl::set);
void main_post_setup(std::stringstream &);
std::string gen_meta_func(isl::set, const YAML::Node, int);
void run_simple(isl::set);

}

#endif
