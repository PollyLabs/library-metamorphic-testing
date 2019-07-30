#ifndef TEST_EMITTER_HPP
#define TEST_EMITTER_HPP

#include <ctime>
#include <fstream>

#include "fmt/format.h"
#include "yaml-cpp/yaml.h"

#include "api_fuzzer.hpp"

extern std::stringstream new_ss_i, new_ss_mi, new_ss_p;
extern const char *command, *exe_command;

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

std::string Exec(const char* cmd);
std::pair<std::string, std::string> parseErrorMsg(std::string msg);
bool parseAssertInstruction(std::set<std::string> var, const ApiInstructionInterface *inst);

void printVectorApiObjects(std::vector<const ApiObject*> var);
void printVectorApiFuncObjects(std::vector<const ApiFuncObject*> var);
void printVectorApiInstructions(std::vector<const ApiInstructionInterface*> instr);
#endif
