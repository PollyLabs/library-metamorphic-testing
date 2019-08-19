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

class ApiFuzzerNew;
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
        std::vector<const ApiInstructionInterface*> getApiInstructions() const;

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
        ApiFuzzerNew* fuzzer;
        const ApiObject* curr_meta_variant;
        const std::string meta_var_name;
        const ApiType* meta_var_type;
        const std::vector<const MetaRelation*> relations;
        const std::vector<const MetaRelation*> meta_checks;
        const std::vector<const ApiObject*> meta_in_vars;
        const std::vector<MetaVarObject*> meta_vars;
        const std::vector<const ApiObject*> meta_variants;
        std::map<const MetaVarObject*, const ApiObject*> meta_var_inits;

        std::queue<std::string> abstract_rel_chain;
        std::vector<const MetaTest*> meta_tests;
        std::set<size_t> meta_test_hashes;

        std::mt19937* rng;

    public:
        SetMetaTesterNew(ApiFuzzerNew* fuzzer);
            //const std::vector<const MetaRelation*>&,
            //const std::vector<const MetaRelation*>&,
            //const std::vector<const ApiObject*>&,
            //const std::vector<const MetaVarObject*>&,
            //const std::vector<const ApiObject*>&,
            //const ApiType*, std::mt19937*);
        void genMetaTests(unsigned int);
        std::string getAbstractMetaRelChain() const;
        const ApiObject* getCurrentMetaVar() const
        {
            return this->curr_meta_variant;
        };
	
        std::vector<const MetaTest*> getMetaTests()
	{
		return meta_tests;
	};

	const std::vector<const MetaRelation*> getRelations()
	{
		return relations;
	}

    private:
        void initMetaRels(std::map<std::string, std::set<std::string>>&, YAML::Node);
        std::queue<std::string> makeAbstractMetaRelChain(unsigned int);
        const MetaTest* genOneMetaTest(std::queue<std::string>, const ApiObject*);
        bool addMetaTest(MetaTest* expr);
        const MetaRelation* getConcreteMetaRel(std::string, const ApiObject*,
            std::vector<const ApiObject*>, bool) const;
        std::vector<const ApiInstructionInterface*> testToApiInstrs(const MetaTest*) const;
        void finalizeTest(MetaTest*) const;
};

#endif
