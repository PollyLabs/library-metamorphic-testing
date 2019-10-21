#ifndef REDUCTION_HPP
#define REDUCTION_HPP

#include "api_fuzzer.hpp"
#include "test_emitter.hpp"


class Reduction
{
	public: 
//		Reduction(std::unique_ptr<ApiFuzzerNew> api_fuzzer_) : api_fuzzer(api_fuzzer_) {};
		Reduction() {};
		~Reduction() = default;

		std::shared_ptr<ApiFuzzerNew> api_fuzzer;

		DependenceTree replaceSubTree(DependenceTree tree, NodeT* node, NodeT* new_node);
		EdgeT* getNewEdge(EdgeT* old_edge, NodeT* node, NodeT* new_node);
		const ApiInstructionInterface* getNewInstruction(const ApiInstruction* old_instr, NodeT* node, NodeT* new_node);

		std::vector<const ApiObject*> getApiObjects(std::vector<std::string> var);

		std::vector<int> MHReduceInstr(std::string compile_err, std::string exe_err, std::vector<int> rel_indices, std::string output_file);
		void MHReduceInstrPrep(std::string compile_err, std::string exe_err, std::string output_file);
		std::pair<std::string, std::string> createTestCaseMHReduce(std::string exe_err, std::vector<int> rel_indices, std::string output_file);
		std::vector<int> indexMerge(std::vector<int> v1, std::vector<int> v2);
		bool assertReduction(const ApiInstructionInterface* assert, std::vector<const ApiObject*> var);
		std::vector<const ApiInstructionInterface*> MetaVariantReduce(std::vector<const ApiObject*> var);

		std::vector<const ApiObject*> verticalReduction(std::string compile_err, std::string exe_err, std::vector<const ApiObject*> mvar, std::string output_file);	
		std::vector<const ApiObject*> verticalReductionMerge(std::vector<const ApiObject*> mvar1, std::vector<const ApiObject*> mvar2);

		std::pair<std::string, std::string> createTestCase(std::vector<const ApiObject*> var, std::string output_file);
		std::pair<std::string, std::string> createTestCaseTree(DependenceTree tree, std::string output_file);
		bool checkTestCase(std::string c_err, std::string n_c_err, std::string e_err, std::string n_e_err);


		void replaceMetaInputVariables(std::string compiler_err, std::string exe_err, std::string output_file);
		std::pair<std::string, std::string> createTestCaseForInputVarReduction(std::vector<const ApiInstructionInterface*> red, std::string output_file, DependenceTree tree);
		const ApiInstructionInterface* getNewInstructionForMetaRelation(const ApiInstructionInterface* instr, const ApiObject* original_obj, const ApiObject* new_obj);
		const FuncObject* processFuncObject(const FuncObject* f_obj, const ApiObject* original_obj, const ApiObject* new_obj);

		std::pair<std::string, std::string> createTestCaseEdge(NodeT* node, std::vector<EdgeT*> new_child, std::string output_file);
		std::vector<EdgeT*> childReduction(std::string compile_err, std::string exe_err, NodeT* node, std::vector<EdgeT*> child, std::string output_file);
		void nodeReduction(std::string compile_err, std::string exe_err, NodeT* node, std::string output_file);
		void fuzzerReduction(std::string compile_err, std::string exe_err, std::string output_file);
		std::vector<EdgeT*> edgeMerge(std::vector<EdgeT*> mvar1, std::vector<EdgeT*> mvar2);

		void reduceSubTree(std::string compile_err, std::string exe_err, std::string output_file);
		void subTreeReduction(std::string compile_err, std::string exe_err, NodeT* node, std::string output_file);

		void simplifyMetaRelationsPrep(std::string compile_err, std::string exe_err, std::string output_file);
		std::vector<const ApiInstructionInterface*> simplifyMetaRelations(std::string compile_err, std::string exe_err, std::string output_file, std::vector<const ApiInstructionInterface*> var, std::map<const ApiInstructionInterface*, const ApiInstructionInterface*> map_relations);
		std::pair<std::string, std::string> createTestCaseSimplify(std::vector<const ApiInstructionInterface*> var, std::map<const ApiInstructionInterface*, const ApiInstructionInterface*> map_relations, std::string output_file);
		std::vector<const ApiInstructionInterface*> instructionMerge(std::vector<const ApiInstructionInterface*> mvar1, std::vector<const ApiInstructionInterface*> mvar2);

		const ApiObject* getReplacementObject(const ApiType* type);

};

const ApiInstructionInterface* isPresent(std::vector<const ApiInstructionInterface*> red, const ApiInstructionInterface* instr);

#endif

  
