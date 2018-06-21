#ifndef SET_META_TESTER_HPP
#define SET_META_TESTER_HPP

#include "isl-noexceptions.h"
#include "yaml-cpp/yaml.h"
#include "isl_tester.hpp"

#include <cassert>
#include <cstring>
#include <sstream>
#include <fstream>
#include <map>
#include <queue>
#include <vector>
#include <iostream>
#include <random>

namespace isl_tester {
    struct Arguments;
}

namespace set_meta_tester {

void write_line(std::stringstream &, std::string);
void write_args(std::stringstream &, isl_tester::Arguments, std::string);
void prepare_header(std::stringstream &);
void main_pre_setup(std::stringstream &);
void gen_var_declarations(std::stringstream &, isl::set);
void gen_coalesce_split_test(std::stringstream &);
void main_post_setup(std::stringstream &);
std::queue<std::string> gen_meta_relation(unsigned int);
std::string get_meta_relation(std::queue<std::string>);
std::string get_relation(const YAML::Node);
std::string get_generator(const YAML::Node, const std::string);
void replace_meta_inputs(std::string &, const std::string, const YAML::Node);
std::string gen_meta_func(const std::string, std::string, const YAML::Node);
std::string gen_meta_expr(std::stringstream &, const unsigned int, std::queue<std::string>, const YAML::Node, std::set<std::string>);
void run_simple(isl::set, isl_tester::Arguments &);

}

#endif
