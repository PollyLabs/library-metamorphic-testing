#include "api_fuzzer.hpp"

/* TODO list:
 * - add a depth for expression generation
 */


char delim_front = '<';
char delim_back = '>';
char delim_mid = '=';

/*******************************************************************************
 * Helper functions
 ******************************************************************************/

template<typename T>
T
getRandomVectorElem(std::vector<T>& vector_in, std::mt19937* rng)
{
    unsigned int rand_val = (*rng)();
    logDebug(fmt::format("RAND GEN {}", rand_val));
    return vector_in.at(rand_val % vector_in.size());
}

template<typename T>
T
getRandomSetElem(std::set<T>& set_in, std::mt19937* rng)
{
    typename std::set<T>::iterator it = set_in.begin();
    unsigned int rand_val = (*rng)();
    logDebug(fmt::format("RAND GEN {}", rand_val));
    int advance_count = rand_val % set_in.size();
    std::advance(it, advance_count);
    return *it;
}

template<typename T>
std::vector<const ApiObject*>
filterObjList(std::vector<const ApiObject*> obj_list,
    bool (ApiObject::*filter_func)(T) const, T filter_check)
{
    std::vector<const ApiObject*> filtered_objs;
    for (const ApiObject* obj : obj_list)
    {
        if ((obj->*filter_func)(filter_check))
        {
            filtered_objs.push_back(obj);
        }
    }
    return filtered_objs;
}

std::set<const ApiFunc*>
filterFuncList(std::set<const ApiFunc*> func_list,
    bool (ApiFunc::*filter_func)() const)
{
    std::set<const ApiFunc*> filtered_funcs;
    for (const ApiFunc* fn : func_list)
    {
        if ((fn->*filter_func)())
        {
            filtered_funcs.insert(fn);
        }
    }
    return filtered_funcs;
}

template<typename T>
std::set<const ApiFunc*>
filterFuncList(std::set<const ApiFunc*> func_list,
    bool (ApiFunc::*filter_func)(T) const, T filter_check)
{
    std::set<const ApiFunc*> filtered_funcs;
    for (const ApiFunc* fn : func_list)
    {
        if ((fn->*filter_func)(filter_check))
        {
            filtered_funcs.insert(fn);
        }
    }
    return filtered_funcs;
}



/*******************************************************************************
 * ApiFuzzer functions
 ******************************************************************************/

std::vector<const ApiInstructionInterface*>
ApiFuzzer::getInstrList() const
{
    return this->instrs;
}

std::vector<std::string>
ApiFuzzer::getInstrStrs() const
{
    std::vector<std::string> instr_strs;
    for (const ApiInstructionInterface* api_instr : this->getInstrList())
    {
        instr_strs.push_back(api_instr->toStr());
    }
    return instr_strs;
}

std::vector<const ApiObject*>
ApiFuzzer::getObjList() const
{
    return this->objs;
}

std::vector<const ApiObject*>
ApiFuzzer::getAllObjList() const
{
    return this->all_objs;
}

std::set<const ApiType*>
ApiFuzzer::getTypeList() const
{
    return this->types;
}

std::set<const ApiFunc*>
ApiFuzzer::getFuncList() const
{
    return this->funcs;
}

int
ApiFuzzer::getRandInt(int min, int max)
{
    assert(max >= min);
    return (*this->rng)() % (max + 1) + min;
}

unsigned int
ApiFuzzer::getNextID()
{
    this->next_obj_id++;
    return this->next_obj_id - 1;
}

bool
ApiFuzzer::hasTypeName(std::string type_check)
{
    for (const ApiType* type : this->getTypeList())
    {
        if (type->hasName(type_check))
        {
            return true;
        }
    }
    return false;
}

bool
ApiFuzzer::hasFuncName(std::string func_check)
{
    for (const ApiFunc* func : this->getFuncList())
    {
        if (func->hasName(func_check))
        {
            return true;
        }
    }
    return false;
}

void
ApiFuzzer::addInstr(const ApiInstructionInterface* instr)
{
    this->instrs.push_back(instr);
    //static int counter = 0;
    //this->instrs.push_back(instr->toStr());
    //this->instrs.push_back(fmt::format("fprintf(stderr, \"{}\n\");\n", ++counter));
}

void
ApiFuzzer::addObj(const ApiObject* obj)
{
    this->objs.push_back(obj);
    this->all_objs.push_back(obj);
}

void
ApiFuzzer::addType(const ApiType* type)
{
    this->types.insert(type);
}

void
ApiFuzzer::addFunc(const ApiFunc* func)
{
    this->funcs.insert(func);
}

void
ApiFuzzer::addRelation(const MetaRelation* meta_rel)
{
    this->relations.push_back(meta_rel);
}

void
ApiFuzzer::addMetaCheck(const MetaRelation* meta_check)
{
    this->meta_checks.push_back(meta_check);
}

void
ApiFuzzer::addMetaVar(std::string identifier, const ApiType* meta_var_type)
{
    std::vector<const MetaRelation*> empty_relations({});
    this->addMetaVar(identifier, meta_var_type, empty_relations);
}

void
ApiFuzzer::addMetaVar(std::string identifier, const ApiType* meta_var_type,
    std::vector<const MetaRelation*>& meta_var_relations)
{
    MetaVarObject* new_meta_obj(
        new MetaVarObject(identifier, meta_var_type, this->rng));
    std::for_each(meta_var_relations.begin(), meta_var_relations.end(),
        [&](const MetaRelation* meta_rel)
        {
            new_meta_obj->addRelation(meta_rel);
        });
    this->meta_vars.push_back(new_meta_obj);
}

void
ApiFuzzer::addInputMetaVar(size_t id)
{
    this->addMetaVar(std::to_string(id), this->meta_variant_type);
}

const ApiObject*
ApiFuzzer::getMetaVariant(size_t id) const
{
    for (const ApiObject* meta_var : this->meta_variants)
    {
        if (meta_var->getID() == id)
        {
            return meta_var;
        }
    }
    std::cout << "Failed retrieving meta variant variable with id " << id;
    std::cout  << std::endl;
    assert(false);
}

MetaVarObject*
ApiFuzzer::getMetaVar(std::string id_check) const
{
    std::vector<MetaVarObject*>::const_iterator it = this->meta_vars.begin();
    for (; it != this->meta_vars.end(); it++)
    {
        if (!(*it)->getIdentifier().compare(id_check))
        {
            // TODO replace this with check that there is only one
            return *it;
        }
    }
    std::cout << "Could not find MetaVar of type " << id_check << std::endl;
    assert(false);
}

const ApiType*
ApiFuzzer::getTypeByName(std::string type_check) const
{
    for (const ApiType* type : this->getTypeList())
    {
        if (type->hasName(type_check))
        {
            return type;
        }
    }
    std::cout << "Could not find type " << type_check << std::endl;
    std::cout << "List of types:" << std::endl;
    for (const ApiType* type : this->getTypeList())
    {
        std::cout << "\t" << type->toStr() << std::endl;
    }
    assert(false);
}

template<typename T>
std::vector<const ApiObject*>
ApiFuzzer::filterObjs(bool (ApiObject::*filter_func)(T) const, T filter_check) const
{
    return filterObjList(this->getObjList(), filter_func, filter_check);
}

template<typename T>
std::vector<const ApiObject*>
ApiFuzzer::filterAllObjs(bool (ApiObject::*filter_func)(T) const, T filter_check) const
{
    return filterObjList(this->getAllObjList(), filter_func, filter_check);
}

template<typename T>
std::set<const ApiFunc*>
ApiFuzzer::filterFuncs(bool (ApiFunc::*filter_func)(T) const, T filter_check) const
{
    return filterFuncList(this->getFuncList(), filter_func, filter_check);
}

const ApiFunc*
ApiFuzzer::getAnyFuncByName(std::string name) const
{
    std::set<const ApiFunc*> filtered_funcs = filterFuncs(&ApiFunc::hasName, name);
    logDebug("Searching for any func name " + name);
    return getRandomSetElem(filtered_funcs, this->getRNG());
}

const ApiFunc*
ApiFuzzer::getSingleFuncByName(std::string name) const
{
    std::set<const ApiFunc*> filtered_funcs = filterFuncs(&ApiFunc::hasName, name);
    logDebug("Searching for single func name " + name);
    assert(filtered_funcs.size() == 1);
    return getRandomSetElem(filtered_funcs, this->getRNG());
}

const ApiFunc*
ApiFuzzer::getFuncBySignature(std::string name,
    std::vector<const ApiType*> param_types) const
{
    logDebug("Searching for func signature with name " + name + " and types " +
        makeArgString(param_types));
    std::set<const ApiFunc*> filtered_funcs = filterFuncs(&ApiFunc::hasName, name);
    filtered_funcs = filterFuncList(filtered_funcs, &ApiFunc::hasParamTypes,
        param_types);
    assert(filtered_funcs.size() == 1);
    return *(filtered_funcs.begin());
}

const ApiObject*
ApiFuzzer::generateNamedObject(std::string name, const ApiType* type,
    const ApiFunc* init_func, const ApiObject* target_obj,
    std::vector<const ApiObject*> init_func_args)
{
    bool new_obj_decl = true;
    const NamedObject* new_obj = new NamedObject(name, type);
    const ApiInstruction* new_instr = new ApiInstruction(init_func, new_obj,
        target_obj, init_func_args);
    this->addObj(new_obj);
    this->addInstr(new_instr);
    return new_obj;
}

const ApiObject*
ApiFuzzer::generateApiObject(std::string name, const ApiType* type,
    const ApiFunc* init_func, const ApiObject* target_obj,
    std::vector<const ApiObject*> init_func_args)
{
    const ApiObject* new_obj = new ApiObject(name, this->getNextID(), type);
    const ApiInstruction* new_instr = new ApiInstruction(init_func, new_obj,
        target_obj, init_func_args);
    this->addObj(new_obj);
    this->addInstr(new_instr);
    return new_obj;
}

const ApiObject*
ApiFuzzer::generateApiObjectDecl(std::string name, const ApiType* type,
    bool emit_instr)
{
    assert(!type->isSingleton() ||
        this->filterAllObjs(&ApiObject::hasType, type).size() == 0);
    const ApiObject* new_obj = new ApiObject(name, this->getNextID(), type);
    this->addObj(new_obj);
    if (emit_instr)
    {
        const ApiInstructionInterface* new_instr =
            new ObjectDeclInstruction(new_obj);
        this->addInstr(new_instr);
    }
    return new_obj;
}

void
ApiFuzzer::applyFunc(const ApiFunc* func, const ApiObject* target_obj,
    const ApiObject* result_obj)
{
    std::vector<const ApiObject*> func_args = getFuncArgs(func);
    applyFunc(func, target_obj, result_obj, func_args);
}

void
ApiFuzzer::applyFunc(const ApiFunc* func, const ApiObject* target_obj,
    const ApiObject* result_obj, std::vector<const ApiObject*> func_args)
{
    this->addInstr(new ApiInstruction(func, result_obj, target_obj, func_args));
}

std::string
ApiFuzzer::emitFuncCond(const ApiFunc* func, const ApiObject* target_obj,
    std::vector<const ApiObject*> func_args)
{
    std::string cond_str = "";
    for (std::string cond : func->getConditions())
    {
        cond_str += this->parseCondition(cond, target_obj, func_args);
        cond_str += " && ";
    }
    cond_str = cond_str.substr(0, cond_str.length() - strlen(" && "));
    cond_str += " ? ";
    return cond_str;
}

std::string
ApiFuzzer::parseCondition(std::string condition, const ApiObject* target_obj,
    std::vector<const ApiObject*> func_args)
{
    while (condition.find(delim_front) != std::string::npos)
    {
        assert(condition.find(delim_back) != std::string::npos);
        assert(condition.find(delim_front) != std::string::npos);
        if (condition.find("<member>") != std::string::npos)
        {
            condition = condition.replace(condition.find("<member>"),
                strlen("<member>"), target_obj->toStr());
            continue;
        }
        // TODO finish
        assert(false);
    }
    return condition;
}

std::vector<const ApiObject*>
ApiFuzzer::getFuncArgs(const ApiFunc* func)
{
    logDebug(fmt::format("Curr depth is {}", this->depth));
    std::vector<const ApiType*> param_types = func->getParamTypes();
    std::vector<const ApiObject*> params;
    for (const ApiType* param_type : param_types)
    {
        if (!param_type->isExplicit() &&
                this->getRandInt(0, this->max_depth) < this->depth || this->depth > this->max_depth)
        {
            std::vector<const ApiObject*> candidate_params =
                this->filterObjs(&ApiObject::hasType, param_type);
            if (!candidate_params.empty())
            {
                params.push_back(getRandomVectorElem(candidate_params, this->getRNG()));
                continue;
            }
        }
        logDebug("Depth is " + std::to_string(this->depth));
        params.push_back(this->generateObject(param_type));
    }
    return params;
}

/*******************************************************************************
 * ApiFuzzerNew functions
 ******************************************************************************/

ApiFuzzerNew::ApiFuzzerNew(std::string& api_fuzzer_path, std::string& meta_test_path,
    unsigned int _seed, std::mt19937* _rng) :
    ApiFuzzer(_seed, _rng)
{
    /* Fuzzer initialization */
    YAML::Node api_fuzzer_data = YAML::LoadFile(api_fuzzer_path);
    this->initPrimitiveTypes();
    this->initInputs(api_fuzzer_data["inputs"]);
    this->max_depth = this->getInputObjectData<unsigned int>("depth_max");
    for (std::pair<std::string, const ApiObject*> pair_in : this->fuzzer_input)
    {
        const PrimitiveObject<unsigned int>* po =
            dynamic_cast<const PrimitiveObject<unsigned int>*>(pair_in.second);
        logDebug(fmt::format("{} = {}", pair_in.first, po->toStr()));
    }
    this->initTypes(api_fuzzer_data["types"]);
    this->initTypes(api_fuzzer_data["singleton_types"]);
    this->initFuncs(api_fuzzer_data["funcs"]);
    this->initFuncs(api_fuzzer_data["special_funcs"]);
    this->initConstructors(api_fuzzer_data["constructors"]);
    this->initGenConfig(api_fuzzer_data["set_gen"]);

    /* Metamorphic testing initialization */
    YAML::Node meta_test_data = YAML::LoadFile(meta_test_path);
    this->meta_variant_type = this->getTypeByName(
        meta_test_data["meta_var_type"].as<std::string>());
    this->meta_variant_count = meta_test_data["variant_count"].as<size_t>();
    this->initMetaVariantVars();
    this->initMetaVarObjs(meta_test_data["generators"],
        meta_test_data["input_count"]);
    this->initMetaGenerators(meta_test_data["generators"]);
    this->initMetaRelations(meta_test_data["relations"]);

    /* Object fuzzing */
    size_t input_var_count = meta_test_data["input_count"].as<size_t>();
    for (int i = 1; i <= input_var_count; ++i)
    {
        this->current_output_var =
            this->generateApiObjectDecl("out", this->meta_variant_type, false);
        this->objs.clear();
        this->output_vars.push_back(this->current_output_var);
        this->addInstr(new ApiComment(fmt::format(
            "Start generate input var id {} in variable {}",
            i, this->current_output_var->toStr())));
        this->generateSet();
    }
    assert(!this->output_vars.empty());

    // TODO reconsider how meta checks are generated; perhaps only generate
    // them when required
    this->initMetaChecks(meta_test_data["meta_check"]);

    /* Metamorphic tests generation */
    // TODO Ideally, meta_vars should be vector of const, but need to rethink
    // generator initialization process
    std::vector<const MetaVarObject*> meta_vars_const(this->meta_vars.begin(),
        this->meta_vars.end());
    this->smt = std::unique_ptr<SetMetaTesterNew>(new SetMetaTesterNew(
        relations, meta_checks, this->output_vars, meta_vars_const, meta_variants,
        this->meta_variant_type, rng));

    std::vector<const ApiInstructionInterface*> meta_instrs = smt->genMetaTests(5);
    logDebug(fmt::format("META TEST = {}", smt->getAbstractMetaRelChain()));
    this->instrs.push_back(new ApiComment(fmt::format("CURR META TEST: {}",
        smt->getAbstractMetaRelChain())));
    this->instrs.insert(this->instrs.end(), meta_instrs.begin(), meta_instrs.end());

    //for (std::string inst : this->getInstrList())
    //{
        //std::cout << inst << std::endl;
    //}
}

const ApiObject*
ApiFuzzerNew::getCurrOutputVar(const ApiType* output_type = nullptr)
{
    assert(this->current_output_var);
    if (output_type)
    {
        assert(this->meta_variant_type->isType(output_type));
    }
    return this->current_output_var;
}


void
ApiFuzzerNew::initPrimitiveTypes()
{
    for (std::pair<std::string, PrimitiveTypeEnum> primitive_type_decl :
            primitives_map)
    {
        this->addType(new PrimitiveType(primitive_type_decl.first));
    }
}

void
ApiFuzzerNew::initInputs(YAML::Node inputs_config)
{
    for (YAML::Node input_yaml : inputs_config)
    {
        std::string name = input_yaml["name"].as<std::string>();
        const ApiType* obj_type = this->getTypeByName(
            input_yaml["type"].as<std::string>());
        const ApiObject* obj;
        if (input_yaml["range"].IsDefined())
        {
            obj = this->generatePrimitiveObject( (PrimitiveType*) obj_type,
                input_yaml["range"].as<std::string>());
            logDebug(fmt::format("Generated object with data {} for range {}.",
                dynamic_cast<const PrimitiveObject<unsigned int>*>(obj)->getData(),
                input_yaml["range"].as<std::string>()));
        }
        else
        {
            obj = this->generateObject(obj_type);
        }
        this->fuzzer_input.insert(std::pair<std::string, const ApiObject*>(name, obj));
    }
}

void
ApiFuzzerNew::initTypes(YAML::Node types_config)
{
    for (YAML::Node type_yaml : types_config) {
        std::string type_name = type_yaml["name"].as<std::string>();
        bool singleton = false;
        logDebug(fmt::format("YAML SINGLE {}", type_yaml["singleton"].IsDefined()));
        if (type_yaml["singleton"].IsDefined() && type_yaml["singleton"].as<bool>())
        {
            this->addType(new SingletonType(type_name));
        }
        else
        {
            this->addType(new ApiType(type_name));
        }
    }
}

void
ApiFuzzerNew::initFuncs(YAML::Node funcs_config)
{
    for (YAML::Node func_yaml : funcs_config) {
        if (func_yaml["conditions"].size() != 0)
        {
            logDebug(fmt::format("Skipping adding func {} due to constraints "
                "declared.", func_yaml["name"].as<std::string>()));
            continue;
        }
        this->addFunc(this->genNewApiFunc(func_yaml));
    }
}

ApiFunc*
ApiFuzzerNew::genNewApiFunc(YAML::Node func_yaml)
{
    std::string func_name = func_yaml["name"].as<std::string>();
    logDebug("Adding func with name " + func_name);
    const ApiType* member_type = nullptr;
    if (func_yaml["member_type"].IsDefined())
    {
        std::string member_str = func_yaml["member_type"].as<std::string>();
        member_type = member_str == "" ? nullptr :
            this->parseTypeStr(member_str);
    }
    const ApiType* return_type = nullptr;
    if (func_yaml["return_type"].IsDefined())
    {
        std::string return_str = func_yaml["return_type"].as<std::string>();
        return_type = return_str == "" ? nullptr :
            this->parseTypeStr(return_str);
    }
    std::vector<const ApiType*> param_type_list;
    if (func_yaml["param_types"].IsDefined())
    {
        YAML::Node param_types_list_yaml = func_yaml["param_types"];
        for (YAML::Node param_type_yaml : param_types_list_yaml)
        {
            param_type_list.push_back(
                this->parseTypeStr(param_type_yaml.as<std::string>()));
        }
    }
    YAML::Node cond_list_yaml = func_yaml["conditions"];
    std::vector<std::string> cond_list;
    for (YAML::Node cond_yaml : cond_list_yaml)
    {
        cond_list.push_back(cond_yaml.as<std::string>());
    }
    bool special = false, statik = false;
    if (func_yaml["special"].IsDefined())
        special = func_yaml["special"].as<bool>();
    if (func_yaml["static"].IsDefined())
        statik = func_yaml["static"].as<bool>();
    return new ApiFunc(func_name, member_type, return_type, param_type_list,
        cond_list, special, statik);
}

void
ApiFuzzerNew::initConstructors(YAML::Node ctors_yaml)
{
    for (YAML::Node ctor_yaml : ctors_yaml) {
        assert(ctor_yaml["name"].IsDefined());
        assert(ctor_yaml["param_types"].IsDefined());
        std::string func_name = ctor_yaml["name"].as<std::string>();
        const ApiType* member_type = nullptr;
        if (ctor_yaml["member_type"].IsDefined())
        {
            member_type =
                this->parseTypeStr(ctor_yaml["member_type"].as<std::string>());
        }
        // TODO could we handle this better?
        // this is used to not run out of functions when reaching depth limit,
        // in case we want to use a specific function to generate certain objects
        const ApiType* return_type;
        if (ctor_yaml["return_type"].IsDefined())
        {
            return_type =
                this->parseTypeStr(ctor_yaml["return_type"].as<std::string>());
        }
        else
        {
            return_type = this->parseTypeStr(func_name);
        }
        YAML::Node param_types_list_yaml = ctor_yaml["param_types"];
        std::vector<const ApiType *> param_types_list;
        for (YAML::Node param_types_yaml : param_types_list_yaml)
        {
            param_types_list.push_back(
                this->parseTypeStr(param_types_yaml.as<std::string>()));
        }
        std::vector<std::string> cond_list;
        bool new_func_special = false;
        bool new_func_statik = false;
        bool new_func_ctor = true;
        bool max_depth = ctor_yaml["max_depth"].IsDefined() &&
            ctor_yaml["max_depth"].as<bool>();
        this->addFunc(new ApiFunc(func_name, member_type, return_type,
            param_types_list, cond_list, new_func_special, new_func_statik,
            new_func_ctor));
    }
}

const ApiType*
ApiFuzzerNew::parseTypeStr(std::string type_str)
{
    if (type_str.front() == delim_front && type_str.back() == delim_back) {
        if (type_str.find("output_var") != std::string::npos)
        {
            assert(this->current_output_var);
            return new ExplicitType(type_str, this->getCurrOutputVar()->getType());
        }
        if (type_str.find("seed") != std::string::npos)
        {
            return new ExplicitType(type_str, this->getTypeByName("unsigned int"));
        }
        if (type_str.find("rand") != std::string::npos)
        {
            return new ExplicitType(type_str, this->getTypeByName("unsigned int"));
        }
        if (type_str.find("var_name") != std::string::npos)
        {
            return new ExplicitType(type_str, this->getTypeByName("string"));
        }
        assert (type_str.find(delim_mid) != std::string::npos);
        if (type_str.find(fmt::format("input{}", delim_mid)) != std::string::npos)
        {
            // HACK: replace by type of input
            return new ExplicitType(type_str,
                this->getTypeByName("unsigned int"));
        }
        else if (type_str.find(fmt::format("string{}", delim_mid)) != std::string::npos)
        {
            return new ExplicitType(type_str, this->getTypeByName("string"));
        }
        else if (type_str.find(fmt::format("new{}", delim_mid)) != std::string::npos)
        {
            std::string type_substr = this->getGeneratorData(type_str);
            return this->getTypeByName(type_substr);
        }
        else if (type_str.find(fmt::format("expr{}", delim_mid)) != std::string::npos)
        {
            return new ExplicitType(type_str, this->getTypeByName("string"));
        }
        else if (type_str.find(fmt::format("range{}", delim_mid)) != std::string::npos)
        {
            return new ExplicitType(type_str, this->getTypeByName("unsigned int"));
        }
        assert(false);
    }
    return this->getTypeByName(type_str);
}

void
ApiFuzzerNew::initGenConfig(YAML::Node gen_config_yaml)
{
    for (YAML::Node gen_config_instr : gen_config_yaml)
    {
        this->set_gen_instrs.push_back(gen_config_instr);
    }
}

void
ApiFuzzerNew::initMetaVariantVars()
{
    for (size_t i = 0; i < this->meta_variant_count; ++i)
    {
        const ApiObject* new_meta_variant_var =
            new ApiObject(this->meta_variant_name, i, this->meta_variant_type);
        this->meta_variants.push_back(new_meta_variant_var);
    }
}

void
ApiFuzzerNew::initMetaVarObjs(YAML::Node meta_gen_data,
    YAML::Node input_count_data)
{
    this->addMetaVar("<m_curr>", this->meta_variant_type);
    for (int i = input_count_data.as<size_t>(); i > 0; --i)
    {
        this->addInputMetaVar(i);
    }
    for (YAML::const_iterator meta_gen_it = meta_gen_data.begin();
        meta_gen_it != meta_gen_data.end(); ++meta_gen_it)
    {
        this->addMetaVar(meta_gen_it->second["identifier"].as<std::string>(),
            this->meta_variant_type);
    }
}

void
ApiFuzzerNew::initMetaGenerators(YAML::Node meta_gen_yaml)
{
    YAML::const_iterator meta_gen_it = meta_gen_yaml.begin();
    for (; meta_gen_it != meta_gen_yaml.end(); meta_gen_it++)
    {
        std::string gen_type = meta_gen_it->second["identifier"].as<std::string>();
        // TODO need to further customise this
        YAML::const_iterator gen_it = meta_gen_it->second["relations"].begin();
        std::vector<const MetaRelation*> gen_rels;
        for (; gen_it != meta_gen_it->second["relations"].end(); gen_it++)
        {
            logDebug(fmt::format("Adding gen rel {}", gen_it->as<std::string>()));
            this->getMetaVar(gen_type)->addRelation(
                parseRelationString(gen_it->as<std::string>(), gen_type));
        }
    }
}

void
ApiFuzzerNew::initMetaRelations(YAML::Node meta_relation_yaml)
{
    YAML::const_iterator rel_desc_it = meta_relation_yaml.begin();
    for (; rel_desc_it != meta_relation_yaml.end(); rel_desc_it++)
    {
        std::string rel_type_name = rel_desc_it->first.as<std::string>();
        YAML::const_iterator rel_it = rel_desc_it->second.begin();
        for (; rel_it != rel_desc_it->second.end(); rel_it++)
        {
            logDebug(fmt::format("Parsing relation {}", rel_it->as<std::string>()));
            const MetaRelation* new_rel =
                parseRelationString(rel_it->as<std::string>(), rel_type_name);
            this->addRelation(new_rel);
        }
    }
}

void
ApiFuzzerNew::initMetaChecks(YAML::Node check_list_yaml)
{
    for (YAML::const_iterator check_list_it = check_list_yaml.begin();
        check_list_it != check_list_yaml.end(); ++check_list_it)
    {
        this->addMetaCheck(
            parseRelationString(check_list_it->as<std::string>(), "check"));
    }
}

const ApiObject*
ApiFuzzerNew::parseRelationStringVar(std::string rel_string_var) const
{
    if (rel_string_var[0] == '%')
    {
        assert(rel_string_var.length() <= 3);
        assert(rel_string_var.length() >= 1);
        if (std::isdigit(rel_string_var[1]))
        {
            assert(rel_string_var.size() == 2);
            return this->getMetaVar(rel_string_var.substr(1));
        }
        if (rel_string_var[1] == 'm')
        {
            if (std::isdigit(rel_string_var[2]))
            {
                return this->getMetaVariant(stoi(rel_string_var.substr(2)));
            }
            return this->getMetaVar("<m_curr>");
        }
        assert(rel_string_var.size() == 2);
        return this->getMetaVar(std::string(1, rel_string_var[1]));
    }
    std::vector<const ApiObject*> filtered_objs = this->filterAllObjs(
        &ApiObject::hasName, rel_string_var);
    assert(filtered_objs.size() == 1);
    return filtered_objs.at(0);
}

const ApiObject*
ApiFuzzerNew::parseRelationStringSubstr(std::string rel_substr) const
{
    if (std::isspace(rel_substr.front()))
    {
        size_t space_counter = 1;
        while (std::isspace(rel_substr.at(space_counter)))
        {
            ++space_counter;
        }
        rel_substr = rel_substr.substr(space_counter);
    }
    logDebug("Parsing substring " + rel_substr);
    if (rel_substr.find('(') != std::string::npos &&
            rel_substr.find(')') != std::string::npos)
    {
        return parseRelationStringFunc(rel_substr);
    }
    else if (rel_substr.front() == '%')
    {
        return parseRelationStringVar(rel_substr);
    }
    else if (rel_substr.front() == delim_front &&
        rel_substr.back() == delim_back)
    {
        // TODO develop for more comprehensions and move to own function
        assert(rel_substr.find("var-") != std::string::npos);
        std::string var_name = rel_substr.substr(rel_substr.find('-') + 1,
            rel_substr.find(delim_back) - rel_substr.find('-') - 1);
        std::vector<const ApiObject*> candidate_objs =
            this->filterAllObjs(&ApiObject::hasName, var_name);
        assert(candidate_objs.size() == 1);
        return candidate_objs.at(0);
    }
    else
    {
        return parseRelationStringVar(rel_substr);
    }
}

const FuncObject*
ApiFuzzerNew::parseRelationStringFunc(std::string rel_string) const
{
    size_t paren_count = 0;
    std::stack<std::string> funcs;
    std::vector<std::string> params;
    std::vector<const ApiObject*> param_objs;
    std::vector<const ApiType*> param_types;
    std::string func_name;
    std::string accumulator = "";
    std::string::reverse_iterator r_it = rel_string.rbegin();
    size_t it_count = 0;
    for (; r_it != rel_string.rend(); r_it++)
    {
        ++it_count;
        if (*r_it == ')')
        {
            if (paren_count != 0)
            {
                accumulator.insert(0, 1, *r_it);
            }
            ++paren_count;
        }
        else if (*r_it == '(')
        {
            --paren_count;
            if (paren_count == 0)
            {
                if (accumulator != "")
                {
                    params.insert(params.begin(), accumulator);
                    accumulator.clear();
                }
            }
            else
            {
                accumulator.insert(0, 1, *r_it);
            }
        }
        else if (*r_it == ',' && paren_count == 1)
        {
            assert(accumulator != "");
            params.insert(params.begin(), accumulator);
            accumulator.clear();
        }
        else if (*r_it == '.' && paren_count == 0)
        {
            assert(accumulator != "");
            func_name = accumulator;
            accumulator.clear();
            break;
        }
        else if (*r_it == ';')
        {
            continue;
        }
        else
        {
            accumulator.insert(0, 1, *r_it);
        }
    }
    assert (paren_count == 0);
    if (accumulator != "")
    {
        assert(func_name.empty());
        func_name = accumulator;
    }
    assert(!func_name.empty());
    const ApiObject* target = nullptr;
    if (it_count != rel_string.size())
    {
        target = this->parseRelationStringSubstr(
            rel_string.substr(0, rel_string.size() - it_count));
    }
    for (std::string param : params)
    {
        const ApiObject* func_param = this->parseRelationStringSubstr(param);
        param_objs.push_back(func_param);
        param_types.push_back(func_param->getType());
    }
    const ApiFunc* func = this->getFuncBySignature(func_name, param_types);
    return new FuncObject(func, target, param_objs);
}

MetaRelation*
ApiFuzzerNew::parseRelationString(std::string rel_string, std::string rel_name)
    const
{
    const ApiObject* store_result_var = nullptr;

    size_t eq_pos = rel_string.find('=');
    // TODO properly parse strings with multiple equals that are not assignments
    if (eq_pos != std::string::npos &&
            rel_string.find('=', eq_pos + 1) == std::string::npos)
    {
        store_result_var = this->parseRelationStringVar(
            rel_string.substr(0, eq_pos));
        rel_string = rel_string.substr(eq_pos + 1);
    }
    const FuncObject* base_func = parseRelationStringFunc(rel_string);
    return new MetaRelation(rel_name, base_func, store_result_var);
}

const ApiObject*
ApiFuzzerNew::generateObject(const ApiType* obj_type)
{
    logDebug("Generating object of type " + obj_type->toStr());
    if (obj_type->isSingleton())
    {
        return this->getSingletonObject(obj_type);
    }
    else if (obj_type->isPrimitive())
    {
        return this->generatePrimitiveObject((PrimitiveType*) obj_type);
    }
    else if (obj_type->isExplicit())
    {
        const ExplicitType* expl_type = dynamic_cast<const ExplicitType*>(obj_type);
        if (expl_type->isRange())
        {
            const ApiType* obj_type = this->getTypeByName("unsigned int");
            assert(obj_type->isPrimitive());
            const PrimitiveType* prim_type = dynamic_cast<const PrimitiveType*>(obj_type);
            return this->generatePrimitiveObject(prim_type,
                this->getGeneratorData(expl_type->getDefinition()));
        }
        else if (expl_type->isInput())
        {
            std::string input_name =
                expl_type->getDefinition();
            logDebug(fmt::format("DEF {}",
                expl_type->getDefinition()));
            input_name = input_name.substr(input_name.find(delim_mid) + 1,
                input_name.find(delim_back) - input_name.find(delim_mid) - 1);
            logDebug(fmt::format("INPUT_NAME {}", input_name));
            return this->getInputObject(input_name);
        }
        else if (expl_type->isExpr())
        {
            const ApiType* expr_param_type = this->getTypeByName(
                this->getGeneratorData(expl_type->getDefinition()));
            std::vector<const ApiObject*> expr_params = this->filterObjs(
                &ApiObject::hasType, expr_param_type);
            const ExprObject* new_expr_obj = new ExprObject(this->makeLinearExpr(expr_params),
                dynamic_cast<const PrimitiveType*>(expl_type->getUnderlyingType()));
            return new_expr_obj;
        }
        else if (expl_type->getDefinition().find("output_var") != std::string::npos)
        {
            return this->getCurrOutputVar(obj_type);
        }
        else if (expl_type->getDefinition().find("seed") != std::string::npos)
        {
            assert(expl_type->getUnderlyingType()->isPrimitive());
            return new PrimitiveObject<unsigned int>(
                dynamic_cast<const PrimitiveType*>(expl_type->getUnderlyingType()),
                this->seed);
        }
        else if (expl_type->getDefinition().find("rand") != std::string::npos)
        {
            assert(expl_type->getUnderlyingType()->isPrimitive());
            return new PrimitiveObject<unsigned int>(
                dynamic_cast<const PrimitiveType*>(expl_type->getUnderlyingType()),
                this->getRandInt(0, std::numeric_limits<int>::max()));
        }
        else if (expl_type->getDefinition().find("var_name") != std::string::npos)
        {
            return new PrimitiveObject<std::string>(
                dynamic_cast<const PrimitiveType*>(expl_type->getUnderlyingType()),
                "\"p_" + std::to_string(this->getNextID()) + "\"");
        }
        else
        {
            return expl_type->retrieveObj();
        }
    }
    else
    {
        return this->generateNewObject(obj_type);
    }
}

const ApiObject*
ApiFuzzerNew::getInputObject(std::string input_name)
{
    logDebug("Looking for input " + input_name);
    assert(this->fuzzer_input.count(input_name) != 0);
    return this->fuzzer_input[input_name];
}

template<typename T>
T
ApiFuzzerNew::getInputObjectData(std::string input_name)
{
    const ApiObject* input_obj = this->getInputObject(input_name);
    assert(input_obj->isPrimitive());
    return dynamic_cast<const PrimitiveObject<T>*>(input_obj)->getData();
}

const ApiObject*
ApiFuzzerNew::generateNewObject(const ApiType* obj_type)
{
    if (obj_type->isPrimitive())
    {
        return generatePrimitiveObject(dynamic_cast<const PrimitiveType*> (obj_type));
    }
    ++this->depth;
    std::set<const ApiFunc*> ctor_func_candidates = this->filterFuncs(
        &ApiFunc::hasReturnType, obj_type);
    if (this->depth >= this->max_depth)
    {
        ctor_func_candidates = filterFuncList(ctor_func_candidates,
            &ApiFunc::isCtor);
    }
    else
    {
        ctor_func_candidates = filterFuncList(ctor_func_candidates,
            &ApiFunc::notIsSpecial);
        ctor_func_candidates = filterFuncList(ctor_func_candidates,
            &ApiFunc::notIsMaxDepth);
        std::set<const ApiFunc*> non_ctor_func_cands = filterFuncList(ctor_func_candidates,
            &ApiFunc::notIsCtor);
        // TODO this currently forces tests to be produced with full depth
        if (!non_ctor_func_cands.empty())
        {
            ctor_func_candidates = non_ctor_func_cands;
        }
    }
    assert(ctor_func_candidates.size() != 0);
    logDebug("Candidate funcs:");
    for (const ApiFunc* func_cand : ctor_func_candidates)
    {
        logDebug(fmt::format("\t{}", func_cand->getName()));
    }
    const ApiFunc* gen_func = getRandomSetElem(ctor_func_candidates, this->getRNG());
    logDebug(fmt::format("Selected func = {}", gen_func->getName()));
    logDebug("Generating " + obj_type->toStr() + " type object with func " +
        gen_func->getName());
    std::vector<const ApiObject*> ctor_args = this->getFuncArgs(gen_func);
    const ApiObject* target_obj = nullptr;
    if (gen_func->getMemberType() != nullptr)
    {
        const ApiType* target_type = gen_func->getMemberType();
        std::vector<const ApiObject*> target_obj_candidates =
            this->filterObjs(&ApiObject::hasType, target_type);
        if (target_obj_candidates.empty())
        {
            target_obj = this->generateObject(target_type);
        }
        else
        {
            target_obj = getRandomVectorElem(target_obj_candidates, this->getRNG());
        }
    }
    std::string var_name;
    if (obj_type->toStr().find(delim_mid) != std::string::npos)
    {
        var_name = obj_type->toStr().substr(obj_type->toStr().rfind(delim_mid) + 1);
    }
    else
    {
        // TODO change
        var_name = obj_type->toStr().substr(obj_type->toStr().rfind(':') + 1);
        // hack
        while (var_name.find('*') != std::string::npos)
        {
            var_name = var_name.replace(var_name.find('*'), 1, "");
        }
    }
    --this->depth;
    return generateApiObject(var_name, obj_type, gen_func, target_obj, ctor_args);
}

const ApiObject*
ApiFuzzerNew::generatePrimitiveObject(const PrimitiveType* obj_type)
{
    assert(obj_type->isPrimitive());
    logDebug("Generating primitive object with type " + obj_type->toStr());
    switch(obj_type->getTypeEnum()) {
        case UINT:
            // HACKS
            return new PrimitiveObject<unsigned int>(obj_type, this->getRandInt(0, 10));
    }
    assert(false);
}

const ApiObject*
ApiFuzzerNew::generatePrimitiveObject(const PrimitiveType* obj_type,
    std::string range)
{
    assert(obj_type->hasName("unsigned int"));
    logDebug("Generating primitive object with type " + obj_type->toStr() +
        " and range " + range);
    switch(obj_type->getTypeEnum()) {
        case UINT: {
            std::pair<int, int> int_range = this->parseRange(range);
            return new PrimitiveObject<unsigned int>(obj_type,
                this->getRandInt(int_range.first, int_range.second));
        }
    }
    assert(false);
}

const ApiObject*
ApiFuzzerNew::getSingletonObject(const ApiType* obj_type)
{
    std::vector<const ApiObject*> filtered_objs = this->filterAllObjs(
        &ApiObject::hasType, obj_type);
    assert (filtered_objs.size() <= 1);
    if (filtered_objs.size() == 0)
        return generateNewObject(obj_type);
    return filtered_objs.at(0);
}

void
ApiFuzzerNew::generateSet()
{
    for (YAML::Node gen_instr_yaml : this->set_gen_instrs)
    {
        std::string gen_instr_type = gen_instr_yaml["type"].as<std::string>();
        logDebug(fmt::format("Instr type {}", gen_instr_type));
        if (!gen_instr_type.compare("for"))
        {
            logDebug("Make for");
            this->generateForLoop(gen_instr_yaml);
        }
        else if (!gen_instr_type.compare("func"))
        {
            logDebug("Make func");
            this->generateFunc(gen_instr_yaml);
        }
        else if (!gen_instr_type.compare("decl"))
        {
            logDebug("Make decl");
            this->generateDecl(gen_instr_yaml);
        }
        else
        {
            assert(false);
        }
    }
}

void
ApiFuzzerNew::generateDecl(YAML::Node instr_config)
{
    const ApiType* var_type =
        this->getTypeByName(instr_config["var_type"].as<std::string>());
    if (var_type->isSingleton() && this->filterAllObjs(&ApiObject::hasType,
            var_type).size() != 0)
    {
        return;
    }
    std::string var_name = instr_config["var_name"].as<std::string>();
    bool init = instr_config["init"].IsDefined() && instr_config["init"].as<bool>();
    if (instr_config["init"].IsDefined() && instr_config["init"].as<bool>())
    {
        //TODO pipe this through generateNewObject possibly?
        assert(false);
    }
    else
    {
        this->generateApiObjectDecl(var_name, var_type, true);
    }
}

void
ApiFuzzerNew::generateForLoop(YAML::Node instr_config)
{
    std::pair<int, int> iteration_count =
        this->parseRange(instr_config["counter"].as<std::string>());

    logDebug(fmt::format("Range is {} - {}", iteration_count.first,
        iteration_count.second));
    for (unsigned int i = iteration_count.first; i <= iteration_count.second; ++i)
    {
        this->generateFunc(instr_config, i);
    }
}

std::pair<int, int>
ApiFuzzerNew::parseRange(std::string range_str)
{
    assert(range_str.find(",") != std::string::npos);
    assert(range_str.front() == '(' || range_str.front() == '[');
    assert(range_str.back() == ')' || range_str.back() == ']');
    bool from_exclusive = false;
    int from_int, to_int;
    std::string accumulator = "";
    for (char& c : range_str)
    {
        if (c == '(' || c == '[')
        {
            from_exclusive = (c == '(');
        }
        else if (c == ')' || c == ']')
        {
            to_int = this->parseRangeSubstr(accumulator);
            if (c == ')')
            {
                --to_int;
            }
            break;
        }
        else if (c == ',')
        {
            from_int = this->parseRangeSubstr(accumulator);
            accumulator = "";
        }
        else
        {
            accumulator += c;
        }
    }
    if (from_exclusive)
    {
        ++from_int;
    }
    return std::pair<int, int>(from_int, to_int);
}

int
ApiFuzzerNew::parseRangeSubstr(std::string range_substr)
{
    if (range_substr.find(fmt::format("input{}", delim_mid)) != std::string::npos)
    {
        std::string input_name = this->getGeneratorData(range_substr);
        const ApiObject* input_obj = this->getInputObject(input_name);
        assert(input_obj->getType()->isPrimitive());
        assert(dynamic_cast<const PrimitiveType*>(input_obj->getType())->getTypeEnum()
            == PrimitiveTypeEnum::UINT);
        return dynamic_cast<const PrimitiveObject<unsigned int>*>(input_obj)->getData();
    }
    else
    {
        for (char& c : range_substr)
        {
            assert(std::isdigit(c));
        }
        // TODO more checks here
        return atoi(range_substr.c_str());
    }
    assert(false);
}

void
ApiFuzzerNew::generateFunc(YAML::Node instr_config, int loop_counter)
{
    std::string func_name = instr_config["func"].as<std::string>();
    const ApiFunc* func;
    if (instr_config["func_params"].IsDefined())
    {
        std::vector<const ApiType*> param_types;
        for (YAML::Node func_param_yaml : instr_config["func_params"])
        {
            if (func_param_yaml.as<std::string>().find("<loop_counter>") != std::string::npos)
            {
                param_types.push_back(this->getTypeByName("unsigned int"));
            }
            else
            {
                param_types.push_back(
                    this->parseTypeStr(func_param_yaml.as<std::string>()));
            }
        }
        func = this->getFuncBySignature(func_name, param_types);
    }
    else
    {
        func = this->getAnyFuncByName(func_name);
    }
    // Set target object, if any declared
    const ApiObject* target_obj = nullptr;
    if (instr_config["target"].IsDefined())
    {
        // TODO consider more possibilities
        std::string target_type = instr_config["target"].as<std::string>();
        if (target_type.find("output_var") != std::string::npos)
        {
            target_obj = this->getCurrOutputVar(func->getMemberType());
        }
        else if (target_type.find(fmt::format("var{}", delim_mid)) !=
                std::string::npos)
        {
            std::string obj_name = this->getGeneratorData(
                instr_config["target"].as<std::string>());
            std::vector<const ApiObject*> candidate_objs =
                this->filterObjs(&ApiObject::hasName, obj_name);
            assert(candidate_objs.size() == 1);
            target_obj = candidate_objs.at(0);
        }
        assert(target_obj->getType()->isType(func->getMemberType()));
    }
    else if (func->getMemberType() && !func->isStatic())
    {
        target_obj = this->generateObject(func->getMemberType());
    }
    // Set return object, if any declared
    const ApiObject* return_obj = nullptr;
    // TODO should we automatically create a return object if not defined?
    bool gen_new_obj = false;
    bool gen_new_named_obj = false;
    if (instr_config["return"].IsDefined())
    {
        std::string obj_name = instr_config["return"].as<std::string>();
        if (obj_name.find(fmt::format("new{}", delim_mid)) !=
            std::string::npos)
        {
            gen_new_obj = true;
        }
        else if (obj_name.find(fmt::format("var{}", delim_mid)) !=
            std::string::npos)
        {
            std::vector<const ApiObject*> candidate_objs =
                this->filterAllObjs(&ApiObject::hasName,
                this->getGeneratorData(instr_config["return"].as<std::string>()));
            assert(candidate_objs.size() <= 1);
            if (candidate_objs.size() == 1)
            {
                return_obj = candidate_objs.at(0);
            }
            else
            {
                gen_new_named_obj = true;
                // TODO check for singleton object? Pipe this through generateObject
            }
        }
        else if (obj_name.find("output_var") != std::string::npos)
        {
            // TODO collapse this into first case
            return_obj = this->getCurrOutputVar(func->getReturnType());
        }
        else
        {
            std::vector<const ApiObject*> candidate_objs =
                this->filterObjs(&ApiObject::hasName, obj_name);
            assert(candidate_objs.size() <= 1);
            if (candidate_objs.size() == 1)
            {
                return_obj = candidate_objs.at(0);
                assert(return_obj->getType()->isType(func->getReturnType()));
            }
            else
            {
                return_obj = this->generateNewObject(func->getReturnType());
            }
        }
    }
    else if (func->getReturnType())
    {
        return_obj = this->generateObject(func->getReturnType());
    }
    // Set function parameters
    std::vector<const ApiObject*> func_params;
    if (instr_config["func_params"].IsDefined())
    {
        for (YAML::Node func_param_yaml : instr_config["func_params"])
        {
            std::string type_str = func_param_yaml.as<std::string>();
            // TODO might need further checks for loop_counter?
            if (type_str.find("<loop_counter>") != std::string::npos)
            {
                func_params.push_back(new PrimitiveObject<unsigned int>(
                    dynamic_cast<const PrimitiveType*>(
                    this->getTypeByName("unsigned int")), loop_counter));
            }
            else
            {
                const ApiType* type = this->parseTypeStr(type_str);
                func_params.push_back(this->generateObject(type));
            }
        }
    }
    else
    {
        func_params = this->getFuncArgs(func);
    }
    if (gen_new_obj)
    {
        const ApiType* return_type = this->parseTypeStr(
            instr_config["return"].as<std::string>());
        std::string var_name = return_type->toStr();
        var_name = var_name.substr(var_name.rfind(':') + 1);
        return_obj = this->generateApiObject(var_name, return_type, func,
            target_obj, func_params);
    }
    else if (gen_new_named_obj)
    {
        return_obj = this->generateNamedObject(
            this->getGeneratorData(instr_config["return"].as<std::string>()),
            func->getReturnType(), func, target_obj, func_params);
    }
    else
    {
        this->applyFunc(func, target_obj, return_obj,
            func_params);
    }
    if (this->getRandInt(0, 1))
    {

    }
}

std::string
ApiFuzzerNew::getGeneratorData(std::string gen_desc) const
{
    assert(gen_desc.front() == delim_front);
    assert(gen_desc.back() == delim_back);
    assert(gen_desc.find(delim_mid) != std::string::npos);
    return gen_desc.substr(gen_desc.find(delim_mid) + 1,
        gen_desc.rfind(delim_back) - gen_desc.find(delim_mid) - 1);
}

// TODO make this prettier and better
// TODO PPL doesn't like strict inequalities; why?
std::string expr_ops[3] = {"==", "<=", ">="};

std::string
ApiFuzzerNew::makeLinearExpr(std::vector<const ApiObject*> expr_objs)
{
    std::stringstream expr_ss;
    std::vector<const ApiObject*>::iterator it;
    for (it = expr_objs.begin(); it != std::prev(expr_objs.end(), 1); ++it)
    {
        expr_ss << this->getRandInt(0, 10) << '*' << (*it)->toStr();
        expr_ss << (this->getRandInt(0, 1) ? '+' : '-');
    }
    expr_ss << this->getRandInt(0, 10) << '*' << (*it)->toStr();
    expr_ss << expr_ops[this->getRandInt(0, (sizeof(expr_ops) / sizeof(expr_ops[0])))]
        << "0";
    return expr_ss.str();
}
