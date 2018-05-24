#ifndef SET_META_TESTER_HPP
#define SET_META_TESTER_HPP

#include "isl-noexceptions.h"
#include "yaml-cpp/yaml.h"

#include <cassert>
#include <cstring>
#include <sstream>
#include <fstream>
#include <map>
#include <queue>
#include <vector>
#include <iostream>
#include <random>

namespace set_meta_tester {

void write_line(std::stringstream &, std::string);
void prepare_header(std::stringstream &);
void main_pre_setup(std::stringstream &);
void gen_var_declarations(std::stringstream &, isl::set);
void main_post_setup(std::stringstream &);
std::queue<std::string> gen_meta_relation(unsigned int);
std::string get_relation(const YAML::Node);
std::string get_generator(const YAML::Node, const std::string);
std::string gen_meta_func(const YAML::Node, std::queue<std::string>);
std::pair<std::string, std::string> gen_pair_exprs(const YAML::Node, std::queue<std::string>);
void run_simple(isl::set);

}

#endif
