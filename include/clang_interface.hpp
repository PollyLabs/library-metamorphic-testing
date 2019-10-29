#ifndef _CLANG_INTERFACE_HPP
#define _CLANG_INTERFACE_HPP

#include "clang/AST/Type.h"

#include "api_fuzzer.hpp"

namespace fuzzer {
namespace clang {

ApiFuzzerNew* getFuzzer();
void setSeed(size_t);

void addPrimitiveTypes(ApiFuzzerNew*);
void addLibType(std::string);
void addLibDeclaredObj(std::string, std::string);
void addLibFunc(std::string, std::string, std::string, std::vector<std::string>,
    bool, bool);
std::pair<std::string, std::string>
    generateObjectInstructions(std::string, std::string);
std::string generateMetaTestInstructions(std::vector<std::string>&,
    const std::string&, const std::string&, const std::string&, size_t);
void resetApiObjs(std::set<std::pair<std::string, std::string>>);

int generateRand(int, int);

} // namespace clang
} // namespace fuzzer

#endif // _CLANG_INTERFACE_HPP
