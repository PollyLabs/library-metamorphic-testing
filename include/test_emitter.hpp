#ifndef TEST_EMITTER_HPP
#define TEST_EMITTER_HPP

#include <ctime>
#include <fstream>

#include "api_fuzzer.hpp"

#include "fmt/format.h"
#include "yaml-cpp/yaml.h"

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
    std::string config_file;
    Modes mode;
    bool debug;
    std::string output_file;

    Arguments(): seed(42), config_file(""), debug(false), output_file("") {};
};

void parseArgs(Arguments&, int, char**);
YAML::Node loadYAMLFileWithCheck(const std::string&);
void writeLine(std::stringstream&, std::string);
void prepareHeader(std::stringstream&, std::vector<std::string>&, Arguments&);
void mainPreSetup(std::stringstream&, std::vector<std::string>&);
void mainPostSetup(std::stringstream&);

#endif
