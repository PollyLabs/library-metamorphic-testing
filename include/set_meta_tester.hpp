#ifndef SET_META_TESTER_HPP
#define SET_META_TESTER_HPP

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <queue>
#include <random>

#include "api_elements.hpp"

#include "yaml-cpp/yaml.h"
#include "fmt/format.h"

int getRandInt(std::mt19937*, int, int);
std::string getRandSetElem(std::mt19937*, std::set<std::string>&);


class MetaInstr {
    protected:
        std::string instr_str;

    public:
        MetaInstr(std::string _instr_str) : instr_str(_instr_str) {};

        size_t getHash(void) const;
        std::string toStr() { return this->instr_str; };
};

class MetaTest {
    protected:
        const ApiObject* meta_variant_var;
        std::vector<const MetaRelation*> concrete_relations;
        bool result_var_defined;

    public:
        MetaTest(const ApiObject* _meta_variant_var) :
            meta_variant_var(_meta_variant_var), result_var_defined(false) {};
        ~MetaTest();

        void parseAndAddInstr(std::string);
        void updateFirstInputVarName();
        std::vector<const ApiInstruction*> getApiInstructions() const;

        void addRelation(const MetaRelation* meta_rel)
        {
            this->concrete_relations.push_back(meta_rel);
        };

        std::vector<const MetaRelation*> getRelations() const { return this->concrete_relations; };
        const ApiObject* getVariantVar() const { return this->meta_variant_var; }

        size_t getHash(void) const;

    private:
        std::string replaceMetaInputs(std::string);
};

class SetMetaTesterNew {
    protected:
        const std::string meta_var_name;
        const ApiType* meta_var_type;
        const std::vector<const MetaRelation*> relations;
        const std::vector<const MetaRelation*> meta_checks;
        const std::vector<const ApiObject*> meta_in_vars;
        const std::vector<const MetaVarObject*> meta_vars;
        const std::vector<const ApiObject*> meta_variants;

        std::queue<std::string> abstract_rel_chain;
        std::vector<const MetaTest*> meta_tests;
        std::set<size_t> meta_test_hashes;

        std::mt19937* rng;

        //std::vector<std::string> input_var_names;
        //std::string meta_var_type;
        //std::string meta_var_name;
        //unsigned int meta_var_id;
        //std::string meta_check_str;
        //std::queue<std::string> rel_chain;
        //std::vector<MetaTest*> meta_exprs;

    public:
        SetMetaTesterNew(
            const std::vector<const MetaRelation*>&,
            const std::vector<const MetaRelation*>&,
            const std::vector<const ApiObject*>&,
            const std::vector<const MetaVarObject*>&,
            const std::vector<const ApiObject*>&,
            const ApiType*, std::mt19937*);
        std::vector<const ApiInstruction*> genMetaTests(unsigned int);
        std::string getAbstractMetaRelChain() const;

    private:
        void initMetaRels(std::map<std::string, std::set<std::string>>&, YAML::Node);
        std::queue<std::string> makeAbstractMetaRelChain(unsigned int);
        void genOneMetaTest(std::queue<std::string>, const ApiObject*);
        bool addMetaTest(MetaTest* expr);
        const MetaRelation* getConcreteMetaRel(std::string, const ApiObject*,
            std::vector<const ApiObject*>) const;
        std::vector<const ApiInstruction*> testsToApiInstrs(void) const;
        void finalizeTest(MetaTest*) const;
};

#endif
