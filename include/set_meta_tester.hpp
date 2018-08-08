#ifndef SET_META_TESTER_NEW_HPP
#define SET_META_TESTER_NEW_HPP

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <queue>
#include <random>

#include "yaml-cpp/yaml.h"
#include "fmt/format.h"

class MetaExpr {
    protected:
        std::vector<std::string> instrs;

    public:
        void addInstr(std::string);
        int getHash(void);
        std::vector<std::string> getInstrs(void);
};

class SetMetaTester {
    protected:
        std::string input_var_name;
        std::string meta_var_type;
        std::string meta_var_name;
        unsigned int meta_var_id;
        std::string meta_check_str;
        std::map<std::string, std::set<std::string>> relations;
        std::map<std::string, std::set<std::string>> generators;
        std::vector<MetaExpr*> meta_exprs;
        std::set<size_t> meta_expr_hashes;
        std::mt19937* rng;

    public:
        SetMetaTester(std::string&, std::mt19937*);
        int getRandInt(int, int) const;
        std::vector<std::string> getMetaExprStrs(void) const;

    private:
        void initMetaRels(std::map<std::string, std::set<std::string>>&, YAML::Node);
        std::queue<std::string> makeMetaRelChain(unsigned int);
        void genMetaExpr(std::queue<std::string>);
        bool addMetaExpr(MetaExpr* expr);
        std::string genInstr(std::string, std::string, std::string);
        std::string replaceMetaInputs(std::string, std::string);
        void finalizeExpr(MetaExpr*);

        std::string getRandSetElem(std::set<std::string>&) const;
};

#endif
