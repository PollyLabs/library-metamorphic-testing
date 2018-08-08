#ifndef TEST_EMITTER_HPP
#define TEST_EMITTER_HPP

#include <ctime>
#include <fstream>

#include "api_fuzzer.hpp"
#include "set_meta_tester.hpp"

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
    Modes mode;
    unsigned int max_dims;
    unsigned int max_params;
    unsigned int max_set_count;
    std::string input_sets;

    Arguments(): seed(42), max_dims(5), max_params(5), max_set_count(3), input_sets("") {}
};

void writeLine(std::stringstream&, std::string);
void prepareHeader(std::stringstream&, std::vector<std::string>&, Arguments&);
void mainPreSetup(std::stringstream&, std::vector<std::string>&);
void mainPostSetup(std::stringstream&);

#endif
