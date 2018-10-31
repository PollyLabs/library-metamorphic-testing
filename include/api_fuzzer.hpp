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
#include <stack>
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
        /* Fuzzing members */
        std::set<const ApiType*> types;
        std::set<const ApiFunc*> funcs;
        std::vector<const ApiInstructionInterface*> instrs;
        std::vector<const ApiObject*> objs;
        std::vector<const ApiObject*> all_objs;
        unsigned int next_obj_id;
        unsigned int depth;
        unsigned int max_depth;
        const unsigned int seed;
        std::mt19937* rng;

        /* Metamorphic testing members */
        // TODO change these to sets
        const ApiType* meta_variant_type;
        const std::string meta_variant_name = "r";
        const size_t meta_variant_count = 20;
        std::vector<MetaVarObject*> meta_vars;
        std::vector<const MetaRelation*> relations;
        std::vector<const MetaRelation*> meta_checks;
        std::vector<const ApiObject*> meta_in_vars;
        std::vector<const ApiObject*> meta_variants;

        virtual const ApiObject* generateObject(const ApiType*) = 0;

    public:
        ApiFuzzer(unsigned int _seed, std::mt19937* _rng): next_obj_id(0), depth(0),
            instrs(std::vector<const ApiInstructionInterface*>()),
            objs(std::vector<const ApiObject*>()),
            all_objs(std::vector<const ApiObject*>()),
            types(std::set<const ApiType*>()),
            funcs(std::set<const ApiFunc*>()), rng(_rng), seed(_seed) {};

        std::vector<const ApiInstructionInterface*> getInstrList() const;
        std::vector<std::string> getInstrStrs() const;
        std::vector<const ApiObject*> getObjList() const;
        std::vector<const ApiObject*> getAllObjList() const;
        std::set<const ApiFunc*> getFuncList() const;
        std::set<const ApiType*> getTypeList() const;
        std::mt19937* getRNG() const { return this->rng; };

        int getRandInt(int = 0, int = std::numeric_limits<int>::max());
        unsigned int getNextID();

        bool hasTypeName(std::string);
        bool hasFuncName(std::string);

        const ApiType* getTypeByName(std::string) const;
        template<typename T> std::vector<const ApiObject*> filterObjs(
            bool (ApiObject::*)(T) const, T) const;
        template<typename T> std::vector<const ApiObject*> filterAllObjs(
            bool (ApiObject::*)(T) const, T) const;
        template<typename T> std::set<const ApiFunc*> filterFuncs(
            bool (ApiFunc::*)(T) const, T) const;
        const ApiFunc* getAnyFuncByName(std::string) const;
        const ApiFunc* getSingleFuncByName(std::string) const;
        const ApiFunc* getFuncBySignature(std::string, std::vector<const ApiType*>) const;

        void addInstr(const ApiInstructionInterface*);
        void addObj(const ApiObject*);
        void addType(const ApiType*);
        void addFunc(const ApiFunc*);

        virtual void generateSet() = 0;

    protected:
        const ApiObject* generateNamedObject(std::string, const ApiType*,
            const ApiFunc*, const ApiObject*, std::vector<const ApiObject*>);
        const ApiObject* generateApiObject(std::string, const ApiType*,
            const ApiFunc*, const ApiObject*, std::vector<const ApiObject*>);
        const ApiObject* generateApiObjectDecl(std::string, const ApiType*,
            bool = true);
        void applyFunc(const ApiFunc*, const ApiObject*, const ApiObject*);
        void applyFunc(const ApiFunc*, const ApiObject*, const ApiObject*,
            std::vector<const ApiObject*>);
        std::vector<const ApiObject*> getFuncArgs(const ApiFunc*);

        void addRelation(const MetaRelation*);
        void addMetaCheck(const MetaRelation*);
        void addMetaVar(std::string, const ApiType*);
        void addMetaVar(std::string, const ApiType*, std::vector<const MetaRelation*>&);
        void addInputMetaVar(size_t);
        MetaVarObject* getMetaVar(std::string) const;
        const ApiObject* getMetaVariant(size_t id) const;

    private:
        std::string emitFuncCond(const ApiFunc*, const ApiObject*,
            std::vector<const ApiObject*>);
        std::string parseCondition(std::string, const ApiObject*,
            std::vector<const ApiObject*>);
};

class ApiFuzzerNew : public ApiFuzzer {
    std::map<std::string, const ApiObject*> fuzzer_input;
    std::vector<YAML::Node> set_gen_instrs;
    std::unique_ptr<SetMetaTesterNew> smt;
    const ApiObject* current_output_var;
    std::vector<const ApiObject*> output_vars;

    public:
        ApiFuzzerNew(std::string&, std::string&, unsigned int, std::mt19937*);
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


        void initMetaVarObjs(YAML::Node, YAML::Node);
        void initMetaVariantVars();
        void initMetaRelations(YAML::Node);
        void initMetaGenerators(YAML::Node);
        void initMetaChecks(YAML::Node);
        MetaRelation* parseRelationString(std::string, std::string) const;
        const ApiObject* parseRelationStringVar(std::string) const;
        const FuncObject* parseRelationStringFunc(std::string) const;
        const ApiObject* parseRelationStringSubstr(std::string) const;

        const ApiObject* generateObject(const ApiType*);
        const ApiObject* generateNewObject(const ApiType*);
        const ApiObject* generatePrimitiveObject(const PrimitiveType*);
        const ApiObject* generatePrimitiveObject(const PrimitiveType*, std::string);
        void generateSet();
        const ApiObject* getInputObject(std::string);
        template<typename T> T getInputObjectData(std::string);
        const ApiObject* getSingletonObject(const ApiType*);
        const ApiObject* getCurrOutputVar(const ApiType*);

        std::pair<int, int> parseRange(std::string);
        int parseRangeSubstr(std::string);
        const ApiType* parseTypeStr(std::string);
        std::string getGeneratorData(std::string) const;
        std::string makeLinearExpr(std::vector<const ApiObject*>);
};

#endif
