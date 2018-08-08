#include "set_meta_tester.hpp"

void
MetaExpr::addInstr(std::string instr)
{
    this->instrs.push_back(instr);
}

int
MetaExpr::getHash(void)
{
    std::hash<std::string> string_hash_func;
    size_t string_hash;
    for (std::string instr : this->instrs)
    {
        string_hash += string_hash_func(instr);
    }
    return string_hash;
}

std::vector<std::string>
MetaExpr::getInstrs(void)
{
    return this->instrs;
}

int
SetMetaTester::getRandInt(int min, int max) const
{
    assert(max != 0);
    return (*this->rng)() % max + min;
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
    std::queue<std::string> rel_chain;
    while (rel_count > 0)
    {
        std::map<std::string, std::set<std::string>>::const_iterator it = this->relations.begin();
        std::advance(it, this->getRandInt(0, this->relations.size()));
        rel_chain.push(it->first);
        rel_count--;
    }
    return rel_chain;
}

bool
SetMetaTester::addMetaExpr(MetaExpr* expr)
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
SetMetaTester::getRandSetElem(std::set<std::string>& set_in) const
{
    assert(set_in.size() > 0);
    std::set<std::string>::const_iterator it = set_in.begin();
    std::advance(it, this->getRandInt(0, set_in.size()));
    return *it;
}

std::string
SetMetaTester::replaceMetaInputs(std::string input_string, std::string input_var)
{
    while (true) {
        int pos = input_string.find("%");
        if (pos == std::string::npos)
        {
            break;
        }
        char type = input_string[pos + 1];
        if (std::isdigit(type))
        {
            input_string.replace(pos, 2, input_var);
        }
        else
        {
            switch (type)
            {
                case 'e':
                {
                    input_string.replace(pos, 2,
                        this->getRandSetElem(this->generators["empty"]));
                    break;
                }
                case 'u':
                {
                    input_string.replace(pos, 2,
                        this->getRandSetElem(this->generators["universe"]));
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
                            fmt::format("{}_{}", this->meta_var_name,
                                this->meta_var_id));
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

std::string
SetMetaTester::genInstr(std::string rel_type, std::string input_var, std::string result_var)
{
    assert(this->relations.count(rel_type));
    std::string rel_expr = this->getRandSetElem(this->relations[rel_type]);
    rel_expr = this->replaceMetaInputs(rel_expr, input_var);
    if (rel_expr[0] == '.')
    {
        rel_expr.insert(0, this->input_var_name);
    }
    rel_expr.insert(0, fmt::format("{} = ", result_var));
    rel_expr.push_back(';');
    return rel_expr;
}

void
SetMetaTester::finalizeExpr(MetaExpr* expr)
{
    std::string check_str = this->meta_check_str;
    check_str = this->replaceMetaInputs(check_str, this->input_var_name);
    expr->addInstr(fmt::format("assert({});", check_str));
}

void
SetMetaTester::genMetaExpr(std::queue<std::string> rel_chain)
{
    MetaExpr* expr = new MetaExpr();
    int try_count = 10;
    while (try_count >= 0)
    {
        bool first = true;
        std::queue<std::string> rel_chain_copy(rel_chain);
        std::string input_var = this->input_var_name;
        std::string result_var = fmt::format("{} {}_{}", this->meta_var_type,
            this->meta_var_name, this->meta_var_id);
        while (!rel_chain_copy.empty())
        {
            expr->addInstr(this->genInstr(rel_chain_copy.front(), input_var, result_var));
            input_var = fmt::format("{}_{}", this->meta_var_name, this->meta_var_id);
            result_var = input_var;
            rel_chain_copy.pop();
        }
        this->finalizeExpr(expr);
        if (this->addMetaExpr(expr))
        {
            ++this->meta_var_id;
            break;
        }
    }
}

std::vector<std::string>
SetMetaTester::getMetaExprStrs(void) const
{
    std::vector<std::string> expr_strs;
    for (MetaExpr* meta_expr : this->meta_exprs)
    {
        std::vector<std::string> meta_expr_strs = meta_expr->getInstrs();
        expr_strs.insert(expr_strs.end(), meta_expr_strs.begin(), meta_expr_strs.end());
    }
    return expr_strs;
}

SetMetaTester::SetMetaTester(std::string& meta_test_file, std::mt19937* _rng)
{
    this->input_var_name = "s";
    this->meta_var_name = "r";
    this->meta_var_id = 0;
    this->rng = _rng;
    this->relations = std::map<std::string, std::set<std::string>>();
    this->generators = std::map<std::string, std::set<std::string>>();
    this->meta_exprs = std::vector<MetaExpr*>();
    YAML::Node meta_test_yaml = YAML::LoadFile(meta_test_file);
    this->meta_var_type = meta_test_yaml["meta_var_type"].as<std::string>();
    this->meta_check_str = meta_test_yaml["meta_check"].as<std::string>();
    this->initMetaRels(this->relations, meta_test_yaml["relations"]);
    this->initMetaRels(this->generators, meta_test_yaml["generators"]);
    //for (std::pair<std::string, std::set<std::string>> rels :
        //this->relations)
    //{
        //std::cout << "REL NAME " << rels.first << std::endl;
        //for (std::string rel_desc : rels.second)
        //{
            //std::cout << "\t" << rel_desc << std::endl;
        //}
    //}
    std::queue<std::string> rel_chain = this->makeMetaRelChain(5);
    //std::queue<std::string> rel_copy(rel_chain);
    //while (!rel_copy.empty())
    //{
        //std::cout << rel_copy.front() << " -- ";
        //rel_copy.pop();
    //}
    unsigned int meta_exprs_count = 20;
    for (int i = 0; i < meta_exprs_count; i++)
    {
        this->genMetaExpr(rel_chain);
    }
}
