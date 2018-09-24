#include "set_meta_tester.hpp"

int
getRandInt(std::mt19937* rng, int min, int max)
{
    assert(max != 0);
    return (*rng)() % max + min;
}


std::string
getRandSetElem(std::mt19937* rng, std::set<std::string>& set_in)
{
    assert(set_in.size() > 0);
    std::set<std::string>::const_iterator it = set_in.begin();
    std::advance(it, getRandInt(rng, 0, set_in.size()));
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
    bool first = true;
    std::for_each(this->concrete_relations.begin(), this->concrete_relations.end(),
        [&](const MetaRelation* rel)
        {
            api_instructions.push_back(rel->toApiInstruction(first));
            first = false;
        });
    return api_instructions;
}

//void
//MetaTest::updateFirstInputVarName()
//{
    //this->input_var_names.at(0) = this->getFullMetaVarName();
//}

void
SetMetaTesterNew::finalizeTest(MetaTest* new_test) const
{
    for (const MetaRelation* meta_check : this->meta_checks)
    {
        new_test->addRelation(meta_check->concretizeVars(
            new_test->getVariantVar(), this->meta_variants, this->meta_in_vars));
;
    }
}

std::queue<std::string>
SetMetaTesterNew::makeAbstractMetaRelChain(unsigned int rel_count)
{
    std::set<std::string> abstract_relations;
    for (const MetaRelation* rel : this->relations)
    {
        abstract_relations.insert(rel->getAbstractRelation());
    }
    while (rel_count > 0)
    {
        std::set<std::string>::const_iterator it = abstract_relations.begin();
        std::advance(it, getRandInt(this->rng, 0, abstract_relations.size()));
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

const MetaRelation*
SetMetaTesterNew::getConcreteMetaRel(std::string rel_type,
    const ApiObject* meta_variant_var, std::vector<const ApiObject*> input_vars)
    const
{
    std::vector<const MetaRelation*> concrete_relation_candidates;
    for (std::vector<const MetaRelation*>::const_iterator it =
            this->relations.begin(); it != this->relations.end(); ++it)
    {
        if (!(*it)->getAbstractRelation().compare(rel_type))
        {
            concrete_relation_candidates.push_back(*it);
        }
    }
    assert(!concrete_relation_candidates.empty());
    const MetaRelation* concrete_relation = concrete_relation_candidates.at(
        getRandInt(this->rng, 0, concrete_relation_candidates.size()));
    const MetaRelation* concretized_relation =
        concrete_relation->concretizeVars(meta_variant_var, this->meta_variants,
            input_vars);
    return concretized_relation;
}

void
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
        while (!rel_chain_copy.empty())
        {
            const MetaRelation* concrete_meta_rel = this->getConcreteMetaRel(
                rel_chain_copy.front(), meta_variant_var, input_vars);
            new_test->addRelation(concrete_meta_rel);
            input_vars.at(0) = meta_variant_var;
            rel_chain_copy.pop();
        }
        this->finalizeTest(new_test);
        if (this->addMetaTest(new_test))
        {
            return;
        }
        else
        {
            delete new_test;
        }
    }
}

std::vector<const ApiInstructionInterface*>
SetMetaTesterNew::testsToApiInstrs(void) const
{
    std::vector<const ApiInstructionInterface*> api_instrs;
    std::for_each(this->meta_tests.begin(), this->meta_tests.end(),
        [&](const MetaTest* meta_test)
        {
            api_instrs.push_back(new ApiComment(fmt::format(
                "Test for {}", meta_test->getVariantVar()->toStr())));
            std::vector<const ApiInstructionInterface*> test_instrs = meta_test->getApiInstructions();
            api_instrs.insert(api_instrs.end(), test_instrs.begin(), test_instrs.end());
        });
    return api_instrs;
}

std::vector<const ApiInstructionInterface*>
SetMetaTesterNew::genMetaTests(unsigned int rel_cnt)
{
    assert(!this->meta_in_vars.empty());
    std::queue<std::string> rel_chain = this->makeAbstractMetaRelChain(rel_cnt);
    size_t rel_counter = 0;
    for (const ApiObject* variant : this->meta_variants)
    {
        this->genOneMetaTest(rel_chain, variant);
    }
    return this->testsToApiInstrs();
}

SetMetaTesterNew::SetMetaTesterNew(
    const std::vector<const MetaRelation*>& _relations,
    const std::vector<const MetaRelation*>& _meta_checks,
    const std::vector<const ApiObject*>& _meta_in_vars,
    const std::vector<const MetaVarObject*>& _meta_vars,
    const std::vector<const ApiObject*>& _meta_variants,
    const ApiType* _meta_var_type, std::mt19937* _rng
    ) : relations(_relations), meta_checks(_meta_checks), meta_vars(_meta_vars),
        meta_in_vars(_meta_in_vars), meta_variants(_meta_variants),
        meta_var_type(_meta_var_type), rng(_rng), meta_var_name("r") { }
