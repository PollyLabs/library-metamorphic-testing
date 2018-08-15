#ifndef SET_META_TESTER_NEW_HPP
#define SET_META_TESTER_NEW_HPP

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
        std::string meta_var_type;
        std::string meta_var_name;
        unsigned int meta_var_id;
        std::vector<std::string> input_var_names;
        std::map<std::string, std::set<std::string>>* generators;
        std::mt19937* rng;
        std::vector<MetaInstr*> instrs;
        bool result_var_defined = false;

    public:
        MetaTest(std::string _mvt, std::string _mvn, unsigned int _mvi,
            std::vector<std::string>& _ivn,
            std::map<std::string, std::set<std::string>>* _gen,
            std::mt19937* _rng) :
            meta_var_type(_mvt), meta_var_name(_mvn), meta_var_id(_mvi),
            input_var_names(_ivn), generators(_gen), rng(_rng) {};

        void parseAndAddInstr(std::string);
        void finalizeTest(std::string);

        void addInstr(MetaInstr* instr)
        {
            this->instrs.push_back(instr);
        };
        size_t getHash(void) const;
        std::vector<MetaInstr*> getInstrs(void) const;
        std::vector<std::string> getInstrStrs(void) const;

    private:
        std::string replaceMetaInputs(std::string);
};

class SetMetaTester {
    protected:
        std::vector<std::string> input_var_names;
        std::string meta_var_type;
        std::string meta_var_name;
        unsigned int meta_var_id;
        std::string meta_check_str;
        std::queue<std::string> rel_chain;
        std::map<std::string, std::set<std::string>> relations;
        std::map<std::string, std::set<std::string>> generators;
        std::vector<MetaTest*> meta_exprs;
        std::set<size_t> meta_expr_hashes;
        std::mt19937* rng;

    public:
        SetMetaTester(std::string&, std::mt19937*);
        std::vector<std::string> getMetaTestStrs(void) const;
        std::vector<std::string> genMetaTests(unsigned int, unsigned int);
        std::string getMetaRelChain(void) const;
        void setInputVarNames(std::vector<std::string> _ivn)
        {
            this->input_var_names = _ivn;
        };

    private:
        void initMetaRels(std::map<std::string, std::set<std::string>>&, YAML::Node);
        std::queue<std::string> makeMetaRelChain(unsigned int);
        void genOneMetaTest(std::queue<std::string>);
        bool addMetaTest(MetaTest* expr);
        std::string genMetaExprStr(std::string);

};

#endif
