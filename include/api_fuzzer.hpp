#ifndef API_FUZZER_HPP
#define API_FUZZER_HPP

#include <algorithm>
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

#include "fmt/format.h"
#include "yaml-cpp/yaml.h"

extern char delim_front, delim_back, delim_mid;

typedef
    std::set<const ApiFunc*, decltype(&ApiFunc::pointerCmp)> ApiFunc_c;
typedef std::vector<const ApiObject*> ApiObject_c;
typedef std::vector<const ApiType*> ApiType_c;

void CHECK_YAML_FIELD(std::string);
template<typename T> T getRandomVectorElem(const std::vector<T>&, std::mt19937*);
template<typename T> T getRandomSetElem(const std::set<T>&, std::mt19937*);
template<typename T, typename U> T
    getRandomSetElem(const std::set<T, U>&, std::mt19937*);
std::vector<const ApiObject*> filterObjList
    (std::vector<const ApiObject*>, bool (ApiObject::*)() const);
template<typename T> std::vector<const ApiObject*> filterObjList
    (std::vector<const ApiObject*>, bool (ApiObject::*)(T) const, T);
std::set<const ApiFunc*, decltype(&ApiFunc::pointerCmp)> filterFuncList(
    std::set<const ApiFunc*>, bool (ApiFunc::*)() const);
template<typename T> std::set<const ApiFunc*, decltype(&ApiFunc::pointerCmp)>
    filterFuncList(std::set<const ApiFunc*, decltype(&ApiFunc::pointerCmp)>,
        bool (ApiFunc::*)(T) const, T);

class ApiFuzzer {
    protected:
        /* Fuzzing members */
        std::set<const ApiType*, decltype(&ApiType::pointerCmp)> types;
        std::set<const ApiFunc*, decltype(&ApiFunc::pointerCmp)> funcs;
        std::vector<const ApiInstructionInterface*> instrs;
        std::vector<const ApiObject*> objs;
        std::vector<const ApiObject*> all_objs;
        mutable unsigned int next_obj_id;
        size_t depth;
        size_t max_depth;
        const unsigned int seed;
        std::mt19937* rng;


        virtual const ApiObject* generateObject(const ApiType*) = 0;

    public:
        /* Metamorphic testing members */
        // TODO change these to sets
        std::vector<MetaVarObject*> meta_vars;
        std::vector<const MetaRelation*> relations;
        std::vector<const MetaRelation*> meta_checks;
        std::vector<const ApiObject*> meta_in_vars;
        std::vector<const ApiObject*> meta_variants;
        const ApiType* meta_variant_type;
        const std::string meta_variant_name = "r";
        size_t meta_variant_count;
        size_t meta_test_count;

        bool max_depth_obj_gen = false;

        ApiFuzzer(unsigned int _seed, std::mt19937* _rng):
            types(std::set<const ApiType*, decltype(&ApiType::pointerCmp)>
                (&ApiType::pointerCmp)),
            funcs(std::set<const ApiFunc*, decltype(&ApiFunc::pointerCmp)>
                (&ApiFunc::pointerCmp)),
            instrs(std::vector<const ApiInstructionInterface*>()),
            objs(std::vector<const ApiObject*>()),
            all_objs(std::vector<const ApiObject*>()),
            next_obj_id(0), depth(0), max_depth(10), seed(_seed), rng(_rng) {};
        virtual ~ApiFuzzer() = default;

        std::vector<const ApiInstructionInterface*> getInstrList() const;
        std::vector<std::string> getInstrStrs() const;
        std::vector<const ApiObject*> getObjList() const;
        std::vector<const ApiObject*> getAllObjList() const;
        std::set<const ApiFunc*, decltype(&ApiFunc::pointerCmp)> getFuncList() const;
        std::set<const ApiType*, decltype(&ApiType::pointerCmp)> getTypeList() const;
        std::mt19937* getRNG() const { return this->rng; };

        int getRandInt(int = 0, int = std::numeric_limits<int>::max());
        long getRandLong(long = 0, long = std::numeric_limits<long>::max());
        double getRandDouble(double = 0, double = std::numeric_limits<double>::max());
        std::string getRandString(uint8_t = 0, uint8_t = std::numeric_limits<uint8_t>::max());
        unsigned int getNextID() const;

        bool hasTypeName(std::string);
        bool hasFuncName(std::string);

        const ApiType* getTypeByName(std::string) const;
        TemplateInstance getTemplateInstanceFromString(std::string);
        const ApiObject* getObjectByName(std::string) const;
        template<typename T> std::vector<const ApiObject*> filterObjs(
            bool (ApiObject::*)(T) const, T) const;
        template<typename T> std::vector<const ApiObject*> filterAllObjs(
            bool (ApiObject::*)(T) const, T) const;
        template<typename T>
            std::set<const ApiFunc*, decltype(&ApiFunc::pointerCmp)>
            filterFuncs(bool (ApiFunc::*)(T) const, T) const;
        const ApiFunc* getAnyFuncByName(std::string) const;
        const ApiFunc* getSingleFuncByName(std::string) const;
        const ApiFunc* getFuncBySignature(std::string, const ApiType*,
            const ApiType*, std::vector<const ApiType*>) const;

        void addInstr(const ApiInstructionInterface*);
        void addInstrVector(std::vector<const ApiInstructionInterface*>);
        const ApiObject* addNewObj(const ApiType*);
        const ApiObject* addNewObj(std::string, const ApiType*);
        const ApiObject* addNewNamedObj(std::string, const ApiType*);
        const ApiObject* addNewTemplateObj(std::string, std::string);
        void addObj(const ApiObject*);
        void addType(const ApiType*);
        void addFunc(const ApiFunc*);

        void flushInstrs() { this->instrs.clear(); };

        void setMaxDepthObjGen(bool val) { this->max_depth_obj_gen = val; };

        virtual void generateSet() = 0;

    protected:
        const ApiObject* generateNamedObject(std::string, const ApiType*,
            const ApiFunc*, const ApiObject*, std::vector<const ApiObject*>);
        const ApiObject* generateApiObject(std::string, const ApiType*,
            const ApiFunc*, const ApiObject*, std::vector<const ApiObject*>);
        const ApiObject* generateApiObjectDecl(std::string, const ApiType*,
            bool = true);
        void applyFunc(const ApiFunc*);
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

        std::string getGenericVariableName(const ApiType*) const;

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
        ApiFuzzerNew(size_t);
        ApiFuzzerNew(std::string&, std::string&, unsigned int, std::mt19937*);
        ~ApiFuzzerNew();

        void fuzzerMetaInit(const std::string&);

        const ApiObject* generateObject(const ApiType*);
        const ApiObject* generateNewObject(const ApiType*,
            const ApiObject* = nullptr);
        const ApiObject* constructObject(const ApiType*,
            const ApiObject* = nullptr);
        const MetaRelation* concretizeRelation(const MetaRelation*,
            const ApiObject*, bool);
        void resetApiObjs() { this->objs.clear(); };
        void printDepth() { std::cout << this->depth << std::endl; };
        void setMetaInputVars(std::vector<const ApiObject*>& in_vars)
            { this->meta_in_vars = in_vars; }

    private:
        void initPrimitiveTypes();
        void initInputs(YAML::Node);
        void initTypes(YAML::Node);
        void initFuncs(YAML::Node);
        ApiFunc* genNewApiFunc(YAML::Node);
        void initConstructors(YAML::Node);
        void initVariables(YAML::Node);
        void initGenConfig(YAML::Node);
        void runGeneration(YAML::Node);
        void generateSeq(size_t);
        void generateDecl(YAML::Node);
        void generateForLoop(YAML::Node);
        void generateFunc(YAML::Node, int = -1);

        void initMetaVarObjs(YAML::Node, YAML::Node);
        void initMetaVariantVars();
        void initMetaRelations(YAML::Node);
        void initMetaGenerators(YAML::Node);
        void initMetaChecks(YAML::Node);
        MetaRelation* parseRelationString(std::string, std::string);
        const ApiObject* parseRelationStringVar(std::string);
        const FuncObject* parseRelationStringFunc(std::string);
        const ApiObject* parseRelationStringSubstr(std::string);

        const ApiObject* generatePrimitiveObject(const PrimitiveType*);
        const ApiObject* generatePrimitiveObject(const PrimitiveType*,
            std::string);
        const ApiObject* generatePrimitiveObject(const PrimitiveType*,
            std::string, std::string);
        template<typename T> const ApiObject* generatePrimitiveObject(
            const PrimitiveType*, std::string, T);
        const ApiObject* retrieveObjectByType(const ApiType*);
        const ApiObject* retrieveExplicitObject(const ExplicitType*);
        template<typename T> T parseDescriptor(std::string);
        void generateSet();
        const ApiObject* getInputObject(std::string);
        template<typename T> T getInputObjectData(std::string);
        const ApiObject* getSingletonObject(const ApiType*);
        const ApiObject* getCurrOutputVar(const ApiType*);

        std::pair<int, int> parseRange(std::string);
        int parseRangeSubstr(std::string);
        const ExplicitType* parseComprehension(std::string);
        const ApiType* parseTypeStr(std::string);
        std::string getGeneratorData(std::string) const;
        std::string makeLinearExpr(std::vector<const ApiObject*>);

        const FuncObject* concretizeGenerators(const MetaVarObject*);
        const FuncObject* concretizeGenerators(const FuncObject*);
        std::map<const MetaVarObject*, const ApiObject*>
            makeConcretizationMap(std::vector<const ApiObject*>);
        const FuncObject* concretizeFuncObject(const FuncObject*,
            std::map<const MetaVarObject*, const ApiObject*>);
        const ApiObject* concretizeMetaVarObject(const MetaVarObject*);
};

#endif
