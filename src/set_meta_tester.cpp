#include "set_meta_tester.hpp"
#include "api_fuzzer.hpp"

int
getRandInt(std::mt19937* rng, int min, int max)
{
    return (*rng)() % (max - min + 1) + min;
}

std::string
getRandSetElem(std::mt19937* rng, std::set<std::string>& set_in)
{
    assert(set_in.size() > 0);
    std::set<std::string>::const_iterator it = set_in.begin();
    std::advance(it, getRandInt(rng, 0, set_in.size() - 1));
    return *it;
}

size_t
MetaInstr::getHash(void) const
{
    std::hash<std::string> string_hash_func;
    return string_hash_func(this->instr_str);
}

MetaTest::~MetaTest()
{
    std::for_each(this->concrete_relations.begin(), this->concrete_relations.end(),
        [](const MetaRelation* concrete_mr)
        {
            delete concrete_mr;
        });
}

size_t
MetaTest::getHash(void) const
{
    size_t string_hash;
    std::hash<std::string> string_hash_func;
    for (const MetaRelation* rel : this->getRelations())
    {
        string_hash += string_hash_func(rel->toStr());
    }
    return string_hash;
}

std::vector<const ApiInstructionInterface*>
MetaTest::getApiInstructions() const
{
    std::vector<const ApiInstructionInterface*> api_instructions;
    std::for_each(this->concrete_relations.begin(), this->concrete_relations.end(),
        [&](const MetaRelation* rel)
        {
            api_instructions.push_back(rel->toApiInstruction());
        });
    return api_instructions;
}

void
SetMetaTesterNew::finalizeTest(MetaTest* new_test) const
{
    for (const MetaRelation* meta_check : this->meta_checks)
    {
        new_test->addRelation(meta_check->concretizeVars(
            new_test->getVariantVar(), this->meta_variants, this->meta_in_vars));
    }
}

std::queue<std::string>
SetMetaTesterNew::makeAbstractMetaRelChain(unsigned int rel_count)
{
    std::set<std::string> abstract_relations;
    for (const MetaRelation* rel : this->relations)
    {
//	std::cout << "AR: " << rel->getAbstractRelation() << std::endl;
        abstract_relations.insert(rel->getAbstractRelation());
    }
    while (rel_count > 0)
    {
        std::set<std::string>::const_iterator it = abstract_relations.begin();
        std::advance(it, getRandInt(this->rng, 0, abstract_relations.size() - 1));
        this->abstract_rel_chain.push(*it);
        rel_count--;
    }
    return this->abstract_rel_chain;
}

std::string
SetMetaTesterNew::getAbstractMetaRelChain(void) const
{
    assert(!this->abstract_rel_chain.empty());
    std::string rel_acc = "";
    std::queue<std::string> rel_chain_cp(this->abstract_rel_chain);
    while (!rel_chain_cp.empty())
    {
        rel_acc += rel_chain_cp.front() + '-';
        rel_chain_cp.pop();
    }
    rel_acc.pop_back();
    return rel_acc;
}

bool
SetMetaTesterNew::addMetaTest(MetaTest* test)
{
    size_t test_hash = test->getHash();
    if (this->meta_test_hashes.count(test_hash))
    {
        return false;
    }
    this->meta_tests.push_back(test);
    this->meta_test_hashes.insert(test_hash);
    return true;
}

/* @brief Randomly select a metamorphic relation for the corresponding family
 *
 * @param rel_type String representing the family of relations to choose from
 * @param meta_variant_var Instance of curerntly generated metamorphic variant
 * @param input_vars Vector containing all existing metamorphic inputs
 * @return A concrete instance of the randomly chosen abstract metamorphic
 * relation
 */


const MetaRelation*
SetMetaTesterNew::getConcreteMetaRel(std::string rel_type,
    const ApiObject* meta_variant_var, std::vector<const ApiObject*> input_vars,
    bool first)
    const
{
//    std::cout << "Rel Type: " << rel_type << std::endl;
//    std::cout << "Meta Variant: " << meta_variant_var->toStr() << std::endl;

    std::vector<const MetaRelation*> concrete_relation_candidates;
    for (std::vector<const MetaRelation*>::const_iterator it =
            this->relations.begin(); it != this->relations.end(); ++it)
    {
        if (!(*it)->getAbstractRelation().compare(rel_type))
        {
            concrete_relation_candidates.push_back(*it);
        }
    }
    CHECK_CONDITION(!concrete_relation_candidates.empty(),
        fmt::format("No concrete candidates for relation `{}` found", rel_type));
    const MetaRelation* concrete_relation = concrete_relation_candidates.at(
        getRandInt(this->rng, 0, concrete_relation_candidates.size() - 1));

    const MetaRelation* res;
    const MetaRelation* simp_res;

    res = this->fuzzer->concretizeRelation(concrete_relation, meta_variant_var, first, false);
    simp_res = this->fuzzer->concretizeRelation(concrete_relation_candidates.at(0), meta_variant_var, first, true);

    original_simplified_mapping vecs = mvar_relations[meta_variant_var];

    std::vector<const MetaRelation*> rel_temp1, rel_temp2;
    rel_temp1 = vecs.first;
    rel_temp2 = vecs.second;

//    if(find(rel_temp1.begin(), rel_temp1.end(), res) == rel_temp1.end())
    {
	rel_temp1.push_back(res);
	rel_temp2.push_back(simp_res);

	vecs = std::make_pair(rel_temp1, rel_temp2);
	mvar_relations[meta_variant_var] = vecs;
    }	
		
    #if 0	
//    std::cout << "Concrete Rel: " << concrete_relation->toStr() << std::endl;
    std::cout << "Concrete Res: " << res->toStr() << std::endl;
   std::cout << "Simplified Res: " << simp_res->toStr() << std::endl;
    std::cout << "Abstract Rel: " << res->getAbstractRelation() << std::endl; 
    std::cout << "Base Func: " << res->getBaseFunc()->getFunc()->getName() << std::endl; 
    std::cout << "Store Var: " << res->getStoreVar()->toStr() << std::endl; 
    #endif	

    return res;
    //const MetaRelation* concretized_relation =
        //concrete_relation->concretizeVars(meta_variant_var, this->meta_variants,
            //input_vars);
    //delete concrete_relation;
    //return concretized_relation;
}

/* @brief Creates a complete metamorphic test based on the given relation chain
 *
 * @details Based on the given chain of abstract metamorphic family relations,
 * randomly selects instances of operations for each abstract relation in turn,
 * and parses the contained comprehensions to concretize the relations in valid
 * test instructions, based on the existing symbol table. At the end, appends
 * the declared metamorphic check. Also ensures that an identical test has not
 * been generated before, by caching hashes of previously generated tests.
 *
 * @param rel_chain Represents, in order, the sequence of abstract metamorphic
 * relations to generate
 * @param meta_variant_var The metamorphic variant variable instance for which
 * the test is generated
 * @return A reference to the generated test case, if a unique one was
 * generated, or nullptr otherwise
 */

const MetaTest*
SetMetaTesterNew::genOneMetaTest(std::queue<std::string> rel_chain,
    const ApiObject* meta_variant_var)
{
    int try_count = 10;
    while (try_count >= 0)
    {
        MetaTest* new_test = new MetaTest(meta_variant_var);
        //std::unique_ptr<MetaTest*> new_test = std::unique_ptr<MetaTest*>(new MetaTest());
        std::queue<std::string> rel_chain_copy(rel_chain);
        std::vector<const ApiObject*> input_vars(this->meta_in_vars);
        bool first = true;
        while (!rel_chain_copy.empty())
        {
            const MetaRelation* concrete_meta_rel = this->getConcreteMetaRel(
                rel_chain_copy.front(), meta_variant_var, input_vars, first);
            first = false;
            new_test->addRelation(concrete_meta_rel);
            rel_chain_copy.pop();
        }
        this->finalizeTest(new_test);
        if (this->addMetaTest(new_test))
        {
            return new_test;
        }
        else
        {
            delete new_test;
        }
    }
    return nullptr;
}

/**
* @brief Yields the corresponding ApiInstructionInterfaces from a given MetaTest
*
* Transforms a given MetaTest object into a sequence of
* ApiInstructionInterfaces, including a comment to distinguish each generated
* variant. This is used to transform the variants into printable form.
*
* @param meta_test The MetaTest to convert to ApiInstructionInterfaces
*
* @return A vector containing corresponding ApiInstructionInterfaces for each
* metamorphic relation in the given MetaTest
*/

std::vector<const ApiInstructionInterface*>
SetMetaTesterNew::testToApiInstrs(const MetaTest* meta_test) const
{
    std::vector<const ApiInstructionInterface*> api_instrs;
    api_instrs.push_back(new ApiComment(fmt::format(
        "Test for {}", meta_test->getVariantVar()->toStr())));
    //api_instrs.push_back(new ObjectDeclInstruction(meta_test->getVariantVar()));
    std::vector<const ApiInstructionInterface*> test_instrs = meta_test->getApiInstructions();
    api_instrs.insert(api_instrs.end(), test_instrs.begin(), test_instrs.end());
    return api_instrs;
}

/**
* @brief Generates specified number of metamorphic variants
*
* @details Wrapper function which dispatches calls to generate variants via
* `genOneMetaTest`, based on a generated sequence of metamorphic families,
* common across all generated variants. While there is a hard limit imposed on
* the number of total variants generated, if a variant fails to generate, then
* the function does not attempt to change anything and retry, instead it simply
* returns however much it generated.
*
* @param rel_cnt The number of variants to generate
*/

void
SetMetaTesterNew::genMetaTests(unsigned int rel_cnt)
{
    CHECK_CONDITION(!this->meta_in_vars.empty(),
        fmt::format("No metamorphic inputs found for tester."));
    std::vector<const ApiInstructionInterface*> instrs;
    std::queue<std::string> rel_chain = this->makeAbstractMetaRelChain(rel_cnt);
    CHECK_CONDITION(!this->abstract_rel_chain.empty(),
        fmt::format("Abstract relation chain empty in metamorphic tester."));
    this->fuzzer->addInstr(new ApiComment(
        fmt::format("CURR META TEST: {}", this->getAbstractMetaRelChain())));

    size_t rel_counter = 0;
    for (const ApiObject* meta_variant : this->meta_variants)
    {
        this->curr_meta_variant = meta_variant;
        const MetaTest* new_test = this->genOneMetaTest(rel_chain, meta_variant);
        if (new_test)
        {
            instrs = this->testToApiInstrs(new_test);
            this->fuzzer->addInstrVector(instrs);

	    // Added by Pritam	
            this->fuzzer->MetaVariant_Instr[meta_variant->getID()] = instrs;
        }
        else
        {
            break;
        }
    }
}

SetMetaTesterNew::SetMetaTesterNew(ApiFuzzerNew* _fuzzer
    //const std::vector<const MetaRelation*>& _relations,
    //const std::vector<const MetaRelation*>& _meta_checks,
    //const std::vector<const ApiObject*>& _meta_in_vars,
    //const std::vector<const MetaVarObject*>& _meta_vars,
    //const std::vector<const ApiObject*>& _meta_variants,
    //const ApiType* _meta_var_type, std::mt19937* _rng
    ) : relations(_fuzzer->relations), meta_checks(_fuzzer->meta_checks),
        meta_vars(_fuzzer->meta_vars), meta_in_vars(_fuzzer->meta_in_vars),
        meta_variants(_fuzzer->meta_variants), fuzzer(_fuzzer),
        meta_var_type(_fuzzer->meta_variant_type), rng(_fuzzer->getRNG()),
        meta_var_name(_fuzzer->meta_variant_name) { }
