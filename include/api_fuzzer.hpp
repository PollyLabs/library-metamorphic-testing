#ifndef API_FUZZER_HPP
#define API_FUZZER_HPP

#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <random>
#include <string>
#include <sstream>
#include <set>
#include <tuple>
#include <vector>

#include "api_elements.hpp"
#include "set_meta_tester.hpp"

#include "isl-noexceptions.h"
#include "fmt/format.h"
#include "yaml-cpp/yaml.h"

extern char delim_front, delim_back, delim_mid;

template<typename T> T getRandomVectorElem(std::vector<T>&, std::mt19937*);
template<typename T> T getRandomSetElem(std::set<T>&, std::mt19937*);
template<typename T> std::vector<const ApiObject*> filterObjList
    (std::vector<const ApiObject*>, bool (ApiObject::*)(T) const, T);
template<typename T> std::set<const ApiFunc*> filterFuncList(
    std::set<const ApiFunc*>, bool (ApiFunc::*)(T) const, T);

class ApiFuzzer {
    protected:
        std::set<const ApiType*> types;
        std::set<const ApiFunc*> funcs;
        std::vector<const ApiObject*> objs;
        std::vector<std::string> instrs;
        unsigned int next_obj_id;
        unsigned int depth;
        unsigned int max_depth;
        const unsigned int seed;
        std::mt19937* rng;


        virtual const ApiObject* generateObject(const ApiType*) = 0;

    public:
        ApiFuzzer(unsigned int _seed, std::mt19937* _rng): next_obj_id(0), depth(0),
            instrs(std::vector<std::string>()),
            objs(std::vector<const ApiObject*>()),
            types(std::set<const ApiType*>()),
            funcs(std::set<const ApiFunc*>()), rng(_rng), seed(_seed) {};

        //std::vector<const ApiInstruction*> getInstrList() const;
        std::vector<std::string> getInstrStrs() const;
        std::vector<const ApiObject*> getObjList() const;
        std::set<const ApiFunc*> getFuncList() const;
        std::set<const ApiType*> getTypeList() const;
        std::mt19937* getRNG() { return this->rng; };

        int getRandInt(int = 0, int = std::numeric_limits<int>::max());
        unsigned int getNextID();

        bool hasTypeName(std::string);
        bool hasFuncName(std::string);

        const ApiType* getTypeByName(std::string);
        template<typename T> std::vector<const ApiObject*> filterObjs(
            bool (ApiObject::*)(T) const, T);
        template<typename T> std::set<const ApiFunc*> filterFuncs(
            bool (ApiFunc::*)(T) const, T);
        const ApiFunc* getFuncByName(std::string);

        void addInstr(const ApiInstruction*);
        void addObj(const ApiObject*);
        void addType(const ApiType*);
        void addFunc(const ApiFunc*);

        virtual void generateSet() = 0;

    protected:
        ApiObject* generateApiObjectAndDecl(std::string, std::string,
            std::string, std::initializer_list<std::string>);
        const ApiObject* generateNamedObject(std::string, const ApiType*,
            const ApiFunc*, const ApiObject*, std::vector<const ApiObject*>);
        const ApiObject* generateApiObject(std::string, const ApiType*,
            const ApiFunc*, const ApiObject*, std::vector<const ApiObject*>);
        void applyFunc(const ApiFunc*, const ApiObject*, const ApiObject*);
        void applyFunc(const ApiFunc*, const ApiObject*, const ApiObject*,
            std::vector<const ApiObject*>);
        std::vector<const ApiObject*> getFuncArgs(const ApiFunc*);

    private:
        std::string emitFuncCond(const ApiFunc*, const ApiObject*,
            std::vector<const ApiObject*>);
        std::string parseCondition(std::string, const ApiObject*,
            std::vector<const ApiObject*>);
};

class ApiFuzzerNew : public ApiFuzzer {
    std::map<std::string, const ApiObject*> fuzzer_input;
    std::vector<YAML::Node> set_gen_instrs;
    std::unique_ptr<SetMetaTester> smt;
    const ApiObject* output_var;

    public:
        ApiFuzzerNew(std::string&, unsigned int, std::mt19937*,
            std::unique_ptr<SetMetaTester>);
        //~ApiFuzzerNew();

    private:
        void initPrimitiveTypes();
        void initInputs(YAML::Node);
        void initTypes(YAML::Node);
        void initFuncs(YAML::Node);
        ApiFunc* genNewApiFunc(YAML::Node);
        void initConstructors(YAML::Node);
        void initGenConfig(YAML::Node);
        void runGeneration(YAML::Node);
        void generateForLoop(YAML::Node);
        void generateFunc(YAML::Node, int = -1);
        const ApiObject* getSingletonObject(const ApiType*);
        const ApiObject* getOutputVar(const ApiType*);

        const ApiObject* generateObject(const ApiType*);
        const ApiObject* generateNewObject(const ApiType*);
        const ApiObject* generatePrimitiveObject(const PrimitiveType*);
        const ApiObject* generatePrimitiveObject(const PrimitiveType*, std::string);
        void generateSet();
        const ApiObject* getInputObject(std::string);
        template<typename T> T getInputObjectData(std::string);

        std::pair<int, int> parseRange(std::string);
        int parseRangeSubstr(std::string);
        const ApiType* parseTypeStr(std::string);
        std::string getGeneratorData(std::string) const;
        std::string makeLinearExpr(std::vector<const ApiObject*>);
};

#endif
