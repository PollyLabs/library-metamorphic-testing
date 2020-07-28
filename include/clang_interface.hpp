#ifndef _CLANG_INTERFACE_HPP
#define _CLANG_INTERFACE_HPP

#include "api_fuzzer.hpp"

namespace fuzzer {
namespace clang {

ApiFuzzerNew* getFuzzer();
void setSeed(size_t);

void addPrimitiveTypes(ApiFuzzerNew*);
void addBaseFuncs(ApiFuzzerNew*);
void addLibType(std::string);
void addLibType(std::string, bool, bool);
void addLibEnumType(std::string);
void addLibEnumVal(std::string, std::string);
void addLibTemplateType(std::string, size_t);
void addLibDeclaredObj(std::string, std::string);
void addLibFunc(std::string, std::string, std::string, std::vector<std::string>,
    bool, bool, bool);
std::pair<std::string, std::string>
    generateObjectInstructions(std::string, std::string);
std::string generateMetaTestInstructions(std::vector<std::string>&,
    const std::string&, const std::string&, const std::string&, size_t);
void resetApiObjs(std::set<std::pair<std::string, std::string>>);

int generateRand(int, int);
double generateRand(double, double);
std::string generateRandStr(uint8_t, uint8_t);
std::string cleanTypeName(std::string);

} // namespace clang
} // namespace fuzzer

#endif // _CLANG_INTERFACE_HPP
