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

size_t
MetaTest::getHash(void) const
{
    size_t string_hash;
    for (MetaInstr* instr : this->getInstrs())
    {
        string_hash += instr->getHash();
    }
    return string_hash;
}

std::vector<MetaInstr*>
MetaTest::getInstrs(void) const
{
    return this->instrs;
}

std::vector<std::string>
MetaTest::getInstrStrs(void) const
{
    std::vector<std::string> instrStrs;
    for (MetaInstr* instr : this->getInstrs())
    {
        instrStrs.push_back(instr->toStr());
    }
    return instrStrs;
}

std::string
MetaTest::getFullMetaVarName()
{
    return fmt::format("{}_{}", this->meta_var_name, this->meta_var_id);
}

void
MetaTest::parseAndAddInstr(std::string instr)
{
    MetaInstr* meta_instr = new MetaInstr(this->replaceMetaInputs(instr));
    this->addInstr(std::move(meta_instr));
}

std::string
MetaTest::replaceMetaInputs(std::string input_string)
{
    if (input_string.rfind("%m =") == 0 && !this->result_var_defined)
    {
        input_string.replace(0, 2,
            fmt::format("{} {}_{}", this->meta_var_type, this->meta_var_name,
                this->meta_var_id));
        this->result_var_defined = true;
    }
    while (true) {
        int pos = input_string.find("%");
        if (pos == std::string::npos)
        {
            break;
        }
        char type = input_string[pos + 1];
        if (std::isdigit(type))
        {
            int input_var_id = type - '0';
            //assert(this->input_var_names.size() > input_var_id);
            assert(!std::isdigit(input_string[pos + 2]));
            //input_string.replace(pos, 2, this->input_var_names.at(input_var_id));
            input_string.replace(pos, 2, this->input_var_names.at(0));
        }
        else
        {
            switch (type)
            {
                case 'e':
                {
                    input_string.replace(pos, 2,
                        getRandSetElem(this->rng, (*this->generators)["empty"]));
                    break;
                }
                case 'u':
                {
                    input_string.replace(pos, 2,
                        getRandSetElem(this->rng, (*this->generators)["universe"]));
                    break;
                }
                case 'm':
                {
                    if (std::isdigit(input_string[pos + 2]))
                    {
                        input_string.replace(pos, 2,
                            fmt::format("{}_", this->meta_var_name));
                    }
                    else
                    {
                        input_string.replace(pos, 2,
                            this->getFullMetaVarName());
                    }
                    break;
                }
                default:
                {
                    std::cout << "Unknown input modifier %" << type << std::endl;
                    exit(1);
                }
            }
        }
    }
    return input_string;
}

void
MetaTest::updateFirstInputVarName()
{
    this->input_var_names.at(0) = this->getFullMetaVarName();
}

void
MetaTest::finalizeTest(std::string check_str)
{
    check_str = this->replaceMetaInputs(check_str);
    MetaInstr* check_instr = new MetaInstr(fmt::format("assert({});", check_str));
    this->addInstr(check_instr);
}

void
SetMetaTester::initMetaRels(std::map<std::string, std::set<std::string>>& target_map, YAML::Node meta_rel_yaml)
{
    YAML::const_iterator rel_desc_it = meta_rel_yaml.begin();
    for (; rel_desc_it != meta_rel_yaml.end(); rel_desc_it++)
    {
        std::string rel_type_name = rel_desc_it->first.as<std::string>();
        std::set<std::string> meta_rels_for_type;
        YAML::const_iterator rel_it = rel_desc_it->second.begin();
        for (; rel_it != rel_desc_it->second.end(); rel_it++)
        {
            meta_rels_for_type.insert(rel_it->as<std::string>());
        }
        target_map.insert(std::make_pair(rel_type_name, meta_rels_for_type));
    }
}


std::queue<std::string>
SetMetaTester::makeMetaRelChain(unsigned int rel_count)
{
    while (rel_count > 0)
    {
        std::map<std::string, std::set<std::string>>::const_iterator it = this->relations.begin();
        std::advance(it, getRandInt(this->rng, 0, this->relations.size()));
        this->rel_chain.push(it->first);
        rel_count--;
    }
    return this->rel_chain;
}

std::string
SetMetaTester::getMetaRelChain(void) const
{
    assert(!this->rel_chain.empty());
    std::string rel_acc = "";
    std::queue<std::string> rel_chain_cp(this->rel_chain);
    while (!rel_chain_cp.empty())
    {
        rel_acc += rel_chain_cp.front() + '-';
        rel_chain_cp.pop();
    }
    rel_acc.pop_back();
    return rel_acc;
}

bool
SetMetaTester::addMetaTest(MetaTest* expr)
{
    size_t expr_hash = expr->getHash();
    if (meta_expr_hashes.count(expr_hash))
    {
        return false;
    }
    this->meta_exprs.push_back(expr);
    this->meta_expr_hashes.insert(expr_hash);
    return true;
}

std::string
SetMetaTester::genMetaExprStr(std::string rel_type)
{
    assert(this->relations.count(rel_type));
    std::string rel_expr = getRandSetElem(this->rng, this->relations[rel_type]);
    return rel_expr + ";";
}

void
SetMetaTester::genOneMetaTest(std::queue<std::string> rel_chain)
{
    int try_count = 10;
    while (try_count >= 0)
    {
        MetaTest* new_test = new MetaTest(this->meta_var_type,
            this->meta_var_name, this->meta_var_id, this->input_var_names,
            &this->generators, this->rng);
        std::queue<std::string> rel_chain_copy(rel_chain);
        while (!rel_chain_copy.empty())
        {
            new_test->parseAndAddInstr(this->genMetaExprStr(
                rel_chain_copy.front()));
            new_test->updateFirstInputVarName();
            rel_chain_copy.pop();
        }
        new_test->finalizeTest(this->meta_check_str);
        if (this->addMetaTest(new_test))
        {
            ++this->meta_var_id;
            break;
        }
        else
        {
            free(new_test);
        }
    }
}

std::vector<std::string>
SetMetaTester::getMetaTestStrs(void) const
{
    std::vector<std::string> expr_strs;
    for (MetaTest* meta_expr : this->meta_exprs)
    {
        std::vector<std::string> meta_expr_strs = meta_expr->getInstrStrs();
        expr_strs.insert(expr_strs.end(), meta_expr_strs.begin(), meta_expr_strs.end());
    }
    return expr_strs;
}

std::vector<std::string>
SetMetaTester::genMetaTests(unsigned int rel_cnt, unsigned int expr_cnt)
{
    assert(!this->input_var_names.empty());
    std::queue<std::string> rel_chain = this->makeMetaRelChain(rel_cnt);
    while (expr_cnt > 0)
    {
        this->genOneMetaTest(rel_chain);
        --expr_cnt;
    }
    return this->getMetaTestStrs();
}

SetMetaTester::SetMetaTester(std::string& meta_test_file, std::mt19937* _rng)
{
    this->meta_var_name = "r";
    this->meta_var_id = 0;
    this->rng = _rng;
    this->relations = std::map<std::string, std::set<std::string>>();
    this->generators = std::map<std::string, std::set<std::string>>();
    this->meta_exprs = std::vector<MetaTest*>();
    YAML::Node meta_test_yaml = YAML::LoadFile(meta_test_file);
    this->meta_var_type = meta_test_yaml["meta_var_type"].as<std::string>();
    this->meta_check_str = meta_test_yaml["meta_check"].as<std::string>();
    this->initMetaRels(this->relations, meta_test_yaml["relations"]);
    this->initMetaRels(this->generators, meta_test_yaml["generators"]);
    //std::queue<std::string> rel_chain = this->makeMetaRelChain(5);
    //unsigned int meta_exprs_count = 20;
    //for (int i = 0; i < meta_exprs_count; i++)
    //{
        //this->genMetaTest(rel_chain);
    //}
}
