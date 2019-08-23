#ifndef API_FUZZER_HPP
#define API_FUZZER_HPP

#include "api_elements.hpp"
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

#include "set_meta_tester.hpp"
#include "test_emitter.hpp"

#include "fmt/format.h"
#include "yaml-cpp/yaml.h"

#define RECUR_LIMIT 10

extern char delim_front, delim_back, delim_mid;
extern unsigned int RECUR_TIMES;

void CHECK_YAML_FIELD(std::string);
template<typename T> T getRandomVectorElem(const std::vector<T>&, std::mt19937*);
template<typename T> T getRandomSetElem(const std::set<T>&, std::mt19937*);
std::vector<const ApiObject*> filterObjList
    (std::vector<const ApiObject*>, bool (ApiObject::*)() const);
template<typename T> std::vector<const ApiObject*> filterObjList
    (std::vector<const ApiObject*>, bool (ApiObject::*)(T) const, T);
std::set<const ApiFunc*> filterFuncList(
    std::set<const ApiFunc*>, bool (ApiFunc::*)() const);
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
        mutable unsigned int next_obj_id;
        unsigned int depth;
        unsigned int max_depth;
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

        ApiFuzzer(unsigned int _seed, std::mt19937* _rng): next_obj_id(0), depth(0),
            instrs(std::vector<const ApiInstructionInterface*>()),
            objs(std::vector<const ApiObject*>()),
            all_objs(std::vector<const ApiObject*>()),
            types(std::set<const ApiType*>()),
            funcs(std::set<const ApiFunc*>()), rng(_rng), seed(_seed) {};
        virtual ~ApiFuzzer() = default;

        std::vector<const ApiInstructionInterface*> getInstrList() const;
        std::vector<std::string> getInstrStrs() const;
        std::vector<const ApiObject*> getObjList() const;
        std::vector<const ApiObject*> getAllObjList() const;
        std::set<const ApiFunc*> getFuncList() const;
        std::set<const ApiType*> getTypeList() const;
        std::mt19937* getRNG() const { return this->rng; };

        int getRandInt(int = 0, int = std::numeric_limits<int>::max());
        unsigned int getNextID() const;

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
        const ApiFunc* getFuncBySignature(std::string, const ApiType*,
            const ApiType*, std::vector<const ApiType*>) const;

        void addInstr(const ApiInstructionInterface*);
        void addInstrVector(std::vector<const ApiInstructionInterface*>);
        const ApiObject* addNewObj(const ApiType*);
        const ApiObject* addNewObj(std::string, const ApiType*);
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
        ApiFuzzerNew(std::string&, std::string&, unsigned int, std::mt19937*);
        ~ApiFuzzerNew();

        const MetaRelation* concretizeRelation(const MetaRelation*,
            const ApiObject*, bool, bool);

	std::map<size_t, std::vector<const ApiInstructionInterface*> > MetaVariant_Instr;
	std::map<size_t, std::vector<const ApiInstructionInterface*> > InputVar_Instr;

	DependenceTree replaceSubTree(DependenceTree tree, NodeT* node, NodeT* new_node);
	EdgeT* getNewEdge(EdgeT* old_edge, NodeT* node, NodeT* new_node);
	const ApiInstructionInterface* getNewInstruction(const ApiInstruction* old_instr, NodeT* node, NodeT* new_node);

        std::vector<const ApiInstructionInterface*> InputInstrs;
	std::vector<const ApiObject*> getApiObjects(std::vector<std::string> var);

	std::vector<int> MHReduceInstr(std::string compile_err, std::string exe_err, std::vector<const ApiObject*> var, std::vector<int> rel_indices, std::string output_file);
	std::vector<const ApiInstructionInterface*> MHReduceInstrPrep(std::string compile_err, std::string exe_err, std::vector<const ApiObject*> var, std::string output_file);
	std::pair<std::string, std::string> createTestCaseMHReduce(std::string exe_err, std::vector<const ApiObject*> var, std::vector<int> rel_indices, std::string output_file);
	std::vector<int> indexMerge(std::vector<int> v1, std::vector<int> v2);
	bool assertReduction(const ApiInstructionInterface* assert, std::vector<const ApiObject*> var);
        std::vector<const ApiInstructionInterface*> MetaVariantReduce(std::vector<const ApiObject*> var);

	std::vector<const ApiObject*> verticalReduction(std::string compile_err, std::string exe_err, std::vector<const ApiObject*> mvar, std::string output_file);	
	std::vector<const ApiObject*> verticalReductionMerge(std::vector<const ApiObject*> mvar1, std::vector<const ApiObject*> mvar2);

	std::pair<std::string, std::string> createTestCase(std::vector<const ApiObject*> var, std::string output_file);
	std::pair<std::string, std::string> createTestCaseTree(DependenceTree tree, std::string output_file, std::vector<const ApiInstructionInterface*> red);
	bool checkTestCase(std::string c_err, std::string n_c_err, std::string e_err, std::string n_e_err);
	
	DependenceTree tree;

	std::vector<const ApiInstructionInterface*> replaceMetaInputVariables(std::string compiler_err, std::string exe_err, std::vector<const ApiInstructionInterface*> red, std::string output_file);
	std::pair<std::string, std::string> createTestCaseForInputVarReduction(std::vector<const ApiInstructionInterface*> red, std::string output_file, DependenceTree tree);
	const ApiInstructionInterface* getNewInstructionForMetaRelation(const ApiInstructionInterface* instr, const ApiObject* original_obj, const ApiObject* new_obj);
	const FuncObject* processFuncObject(const FuncObject* f_obj, const ApiObject* original_obj, const ApiObject* new_obj);
	
	void insertInstructionInTheTree(const ApiInstructionInterface* int_instr);

	std::pair<std::string, std::string> createTestCaseEdge(NodeT* node, std::vector<EdgeT*> new_child, std::string output_file, std::vector<const ApiInstructionInterface*> red);
	std::vector<EdgeT*> childReduction(std::string compile_err, std::string exe_err, NodeT* node, std::vector<EdgeT*> child, std::string output_file, std::vector<const ApiInstructionInterface*> red);
	void nodeReduction(std::string compile_err, std::string exe_err, NodeT* node, std::string output_file, std::vector<const ApiInstructionInterface*> red);
	std::vector<const ApiInstructionInterface*> fuzzerReduction(std::string compile_err, std::string exe_err, std::string output_file, std::vector<const ApiInstructionInterface*> red);
	std::vector<EdgeT*> edgeMerge(std::vector<EdgeT*> mvar1, std::vector<EdgeT*> mvar2);

	std::vector<const ApiInstructionInterface*> reduceSubTree(std::string compile_err, std::string exe_err, std::string output_file, std::vector<const ApiInstructionInterface*> red);
	void subTreeReduction(std::string compile_err, std::string exe_err, NodeT* node, std::string output_file, std::vector<const ApiInstructionInterface*> red);

	std::vector<const ApiInstructionInterface*> simplifyMetaRelationsPrep(std::string compile_err, std::string exe_err, std::vector<const ApiObject*> var, std::string output_file, std::vector<const ApiInstructionInterface*> input_insts, std::vector<const ApiInstructionInterface*> red);
	std::vector<const ApiInstructionInterface*> simplifyMetaRelations(std::string compile_err, std::string exe_err, std::string output_file, std::vector<const ApiInstructionInterface*> input_insts, std::vector<const ApiInstructionInterface*> red, std::vector<const ApiInstructionInterface*> var, std::map<const ApiInstructionInterface*, const ApiInstructionInterface*> map_relations);
	std::pair<std::string, std::string> createTestCaseSimplify(std::vector<const ApiInstructionInterface*> input_insts, std::vector<const ApiInstructionInterface*> red, std::vector<const ApiInstructionInterface*> var, std::map<const ApiInstructionInterface*, const ApiInstructionInterface*> map_relations, std::string output_file);
	std::vector<const ApiInstructionInterface*> instructionMerge(std::vector<const ApiInstructionInterface*> mvar1, std::vector<const ApiInstructionInterface*> mvar2);
	
	const ApiObject* getReplacementObject(const ApiType* type);
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

        const ApiObject* generateObject(const ApiType*);
        const ApiObject* generateNewObject(const ApiType*, const ApiObject* = nullptr);
        const ApiObject* generatePrimitiveObject(const PrimitiveType*);
        const ApiObject* generatePrimitiveObject(const PrimitiveType*,
            std::string);
        const ApiObject* generatePrimitiveObject(const PrimitiveType*,
            std::string, std::string);
        template<typename T> const ApiObject* generatePrimitiveObject(
            const PrimitiveType*, std::string, T);
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

        const FuncObject* concretizeGenerators(const MetaVarObject*, bool simplify);
        const FuncObject* concretizeGenerators(const FuncObject*, bool simplify);
        std::map<const MetaVarObject*, const ApiObject*>
            makeConcretizationMap(std::vector<const ApiObject*>);
        const FuncObject* concretizeFuncObject(const FuncObject*,
            std::map<const MetaVarObject*, const ApiObject*>);
        const ApiObject* concretizeMetaVarObject(const MetaVarObject*);

};

extern NodeT *null_node;

typedef std::pair<std::vector<const MetaRelation*>, std::vector<const MetaRelation*> > original_simplified_mapping;

extern std::map<const ApiObject*, original_simplified_mapping> mvar_relations;

const ApiInstructionInterface* isPresent(std::vector<const ApiInstructionInterface*> red, const ApiInstructionInterface* instr);
extern int try_outs;
#endif
