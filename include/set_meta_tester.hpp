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

void writeLine(std::stringstream&, std::string);
void writeArgs(std::stringstream&, isl_tester::Arguments, std::string);
void prepareHeader(std::stringstream&);
void mainPreSetup(std::stringstream&);
void genSetDeclaration(std::stringstream&, std::vector<std::string>&);
void genCoalesceSplitTest(std::stringstream&);
void mainPostSetup(std::stringstream&);
std::queue<std::string> genMetaRelation(unsigned int);
std::string getMetaRelation(std::queue<std::string>);
std::string getRelation(const YAML::Node);
std::string getGenerator(const YAML::Node, const std::string);
void replaceMetaInputs(std::string&, const std::string, const YAML::Node);
std::string genMetaFunc(const std::string, std::string, const YAML::Node);
size_t genMetaExpr(std::stringstream&, const unsigned int,
    std::queue<std::string>, const YAML::Node, std::set<size_t>);
void runSimple(std::vector<std::string>, isl_tester::Arguments&);

}

#endif
