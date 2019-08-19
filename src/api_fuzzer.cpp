#include "api_fuzzer.hpp"

char delim_front = '<';
char delim_back = '>';
char delim_mid = '=';

unsigned int RECUR_TIMES = 0;

unsigned int global_count = 0;

NodeT *null_node;
std::map<const ApiObject*, original_simplified_mapping> mvar_relations;
int try_outs = 0;
std::pair<const ApiObject*, const ApiInstructionInterface*> special_obj;


/*******************************************************************************
 * Helper functions
 ******************************************************************************/

void
CHECK_YAML_FIELD(YAML::Node node, std::string field_name)
{
    std::string message = fmt::format(
        "Did not find field `{}` for unnamed node.", field_name);
    if (node["name"].IsDefined())
    {
        message = fmt::format("Did not find field `{}` for named node `{}`.",
            field_name, node["name"].as<std::string>());
    }
    CHECK_CONDITION(node[field_name].IsDefined(), message);
}

template<typename T>
T
getRandomVectorElem(const std::vector<T>& vector_in, std::mt19937* rng)
{
    unsigned int rand_val = (*rng)();
    logDebug(fmt::format("RAND GEN {}", rand_val));
    CHECK_CONDITION(vector_in.size() != 0,
        fmt::format("Attempt to get element of empty vector."));
    return vector_in.at(rand_val % vector_in.size());
}

template<typename T>
T
getRandomSetElem(const std::set<T>& set_in, std::mt19937* rng)
{
    typename std::set<T>::iterator it = set_in.begin();
    unsigned int rand_val = (*rng)();
    logDebug(fmt::format("RAND GEN {}", rand_val));
    CHECK_CONDITION(set_in.size() != 0,
        fmt::format("Attempt to get element of empty set."));
    int advance_count = rand_val % set_in.size();
    std::advance(it, advance_count);
    return *it;
}

std::vector<const ApiObject*>
filterObjList(std::vector<const ApiObject*> obj_list,
    bool (ApiObject::*filter_func)() const)
{
    std::vector<const ApiObject*> filtered_objs;
    for (const ApiObject* obj : obj_list)
    {
        if ((obj->*filter_func)())
        {
            filtered_objs.push_back(obj);
        }
    }
    return filtered_objs;
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
    return (*this->rng)() % (max - min + 1) + min;
}

unsigned int
ApiFuzzer::getNextID() const
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
}

void
ApiFuzzer::addInstrVector(std::vector<const ApiInstructionInterface*> instr_vec)
{ this->instrs.insert(this->instrs.end(), instr_vec.begin(), instr_vec.end());
}

std::string
ApiFuzzer::getGenericVariableName(const ApiType* type) const
{
    return type->toStr().substr(type->toStr().rfind(':') + 1);
}

const ApiObject*
ApiFuzzer::addNewObj(const ApiType* type)
{
    return this->addNewObj(this->getGenericVariableName(type), type);
}

const ApiObject*
ApiFuzzer::addNewObj(std::string name, const ApiType* type)
{
    const ApiObject* new_obj = new ApiObject(name, this->getNextID(), type);
    this->addObj(new_obj);
    return new_obj;
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
    CHECK_CONDITION(false,
        fmt::format("Did not find metamorphic input with id {}.", id));
    return nullptr; /* Should not get here */
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
    CHECK_CONDITION(false,
        fmt::format("Unable to find MetaVar with id {}.", id_check));
    return nullptr; /* Should not get here */
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
    const ApiType* return_type, const ApiType* target_type,
    std::vector<const ApiType*> param_types) const
{
    logDebug(
        fmt::format("Searching for func signature with name `{}` and types [{}]."
            , name, makeArgString(param_types)));
    std::set<const ApiFunc*> filtered_funcs = filterFuncs(&ApiFunc::hasName, name);
    filtered_funcs = filterFuncList(filtered_funcs, &ApiFunc::hasParamTypes,
        param_types);
    if (return_type)
    {
        filtered_funcs = filterFuncList(filtered_funcs, &ApiFunc::hasReturnType,
            return_type);
    }
    if (target_type)
    {
        filtered_funcs = filterFuncList(filtered_funcs, &ApiFunc::hasClassType,
            target_type);
    }
    CHECK_CONDITION(filtered_funcs.size() == 1,
        fmt::format("Signature filtering for func `{}` yielded {} results, "
                    "expected 1.", name, filtered_funcs.size()));
    assert(filtered_funcs.size() == 1);
    return *(filtered_funcs.begin());
}

const ApiObject*
ApiFuzzer::generateNamedObject(std::string name, const ApiType* type,
    const ApiFunc* init_func, const ApiObject* target_obj,
    std::vector<const ApiObject*> init_func_args)
{
    bool new_obj_decl = true;
    const NamedObject* new_obj = new NamedObject(name, this->getNextID(), type);
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


/**
* @brief Create a new ApiObject, and optionally emit a declaration instruction,
* without initialization
*
* Creates a new ApiObject instance, based on the given name and type parameters,
* and optionally emits an ApiInstruction which declares, but not initializes,
* that object.
*
* @param name Name for created ApiObject
* @param type Type for created ApiObject
* @param emit_instr Whether to emit a declaration instruction or not
*
* @return A pointer to the newly generated ApiObject
*/
const ApiObject*
ApiFuzzer::generateApiObjectDecl(std::string name, const ApiType* type,
    bool emit_instr)
{
    CHECK_CONDITION(!type->checkFlag("singleton") ||
        this->filterAllObjs(&ApiObject::hasType, type).size() == 0,
        fmt::format("Asked to generate another object of singleton type `{}`.",
            type->toStr()));
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
ApiFuzzer::applyFunc(const ApiFunc* func)
{
    const ApiType* return_type = func->getReturnType();
    const ApiObject* return_obj =
        return_type ? this->generateObject(return_type) : nullptr;
    const ApiType* enclosing_class = func->getClassType();
    const ApiObject* member_obj =
        enclosing_class ? this->generateObject(enclosing_class) : nullptr;
    applyFunc(func, member_obj, return_obj);
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
                (this->getRandInt(0, this->max_depth) < this->depth ||
                 this->depth > this->max_depth))
        {
            std::vector<const ApiObject*> candidate_params =
                this->filterObjs(&ApiObject::hasType, param_type);
            //candidate_params = filterObjList(candidate_params,
                //&ApiObject::isDeclared);
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
	
    null_node = new NodeT();
    null_node->id = -1;

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
    this->initVariables(api_fuzzer_data["var_decl"]);
    this->initGenConfig(api_fuzzer_data["set_gen"]);

    /* Metamorphic testing initialization */
    YAML::Node meta_test_data = YAML::LoadFile(meta_test_path);
    CHECK_YAML_FIELD(meta_test_data, "meta_var_type");
    CHECK_YAML_FIELD(meta_test_data, "variant_count");
    CHECK_YAML_FIELD(meta_test_data, "meta_test_count");
    this->meta_variant_type = this->getTypeByName(
        meta_test_data["meta_var_type"].as<std::string>());
    this->meta_variant_count = meta_test_data["variant_count"].as<size_t>();
    this->meta_test_count = meta_test_data["meta_test_count"].as<size_t>();
    this->initMetaVariantVars();
    this->initMetaVarObjs(meta_test_data["generators"],
        meta_test_data["input_count"]);
    this->initMetaGenerators(meta_test_data["generators"]);
    this->initMetaRelations(meta_test_data["relations"]);

    /* Object fuzzing */
    size_t input_var_count = meta_test_data["input_count"].as<size_t>();
    for (int i = 1; i <= input_var_count; ++i)
    {
        this->current_output_var = this->addNewObj("out", this->meta_variant_type);
        this->objs.clear();
        this->output_vars.push_back(this->current_output_var);
        this->addInstr(new ApiComment(fmt::format(
            "Start generate input var id {} in variable {}",
            i, this->current_output_var->toStr())));
        this->generateSet();
    }

    for(std::vector<const ApiInstructionInterface*>::iterator it = instrs.begin(); it != instrs.end(); it++)
    {
	InputInstrs.push_back(*it);
    } 
	
    assert(this->output_vars.size() == input_var_count);
    this->meta_in_vars = this->output_vars;

    // TODO reconsider how meta checks are generated; perhaps only generate
    // them when required
    this->initMetaChecks(meta_test_data["meta_check"]);

    /* Metamorphic tests generation */
    // TODO Ideally, meta_vars should be vector of const, but need to rethink
    // generator initialization process
    std::vector<const MetaVarObject*> meta_vars_const(this->meta_vars.begin(),
        this->meta_vars.end());
    this->smt = std::unique_ptr<SetMetaTesterNew>(new SetMetaTesterNew(this));
    smt->genMetaTests(this->meta_test_count);
}

/**
 * @brief Destroy all references generated throughout the fuzzing process
 *
 * Current references freed:
 * * ApiObjects (including meta variants)
 * * ApiTypes (excludes certain ExplicitTypes)
 * * ApiInstructionInterface
 * * MetaRelations (expressing metamorphic checks, abstract metamorphic
 * relations)
 *
 * @todo Extend these to other containers containing references, and further log
 * other allocated memory to remove all memory leaks
 */

ApiFuzzerNew::~ApiFuzzerNew()
{
    std::cout << ">> Cleaning funcs..." << std::endl;
    for (const ApiFunc* api_func : this->getFuncList())
    {
        delete api_func;
    }
    //std::for_each(this->getFuncList().begin(), this->getFuncList().end(),
        //[](const ApiFunc* api_fnc){ std::cout << api_fnc << std::endl; delete api_fnc; });
    //this->getFuncList().clear();
    std::cout << ">> Cleaning objs..." << std::endl;
    std::for_each(this->all_objs.begin(), this->all_objs.end(),
        [](const ApiObject* api_obj){ delete api_obj; });
    std::cout << ">> Cleaning types..." << std::endl;
    for (const ApiType* api_type : this->getTypeList())
    {
        delete api_type;
    }
    std::cout << ">> Cleaning instructions..." << std::endl;
    std::for_each(this->instrs.begin(), this->instrs.end(),
        [](const ApiInstructionInterface* instr){ delete instr; });
    //std::for_each(this->getTypeList().begin(), this->getTypeList().end(),
        //[](const ApiType* api_typ){ std::cout << api_typ << std::endl; delete api_typ; });
    std::cout << ">> Cleaning meta check expressions..." << std::endl;
    std::for_each(this->meta_checks.begin(), this->meta_checks.end(),
        [](const MetaRelation* meta_check){ delete meta_check; });
    std::cout << ">> Cleaning meta relations..." << std::endl;
    std::for_each(this->relations.begin(), this->relations.end(),
        [](const MetaRelation* meta_rel){ delete meta_rel; });
    //std::cout << ">> Cleaning meta input variables..." << std::endl;
    //std::for_each(this->meta_in_vars.begin(), this->meta_in_vars.end(),
        //[](const ApiObject* meta_in_var){ delete meta_in_var; });
    std::cout << ">> Cleaning meta variants..." << std::endl;
    std::for_each(this->meta_variants.begin(), this->meta_variants.end(),
        [](const ApiObject* meta_var){ delete meta_var; });
}

/**
* @brief Return a pointer to the current metamorphic input being generated
*
* If an instance of the ApiObject representing the currently generated
* metamorphic input has been already generated, ensures that the expected type
* corresponds; otherwise, generates a new ApiObject to reference the metamorphic
* input variable.
*
* @param output_type The expected type of the metamorphic input
*
* @return A pointer to the current metamorphic input object
*/

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
        CHECK_YAML_FIELD(input_yaml, "name");
        std::string name = input_yaml["name"].as<std::string>();
        const ApiObject* obj;
        //if (obj_type->isPrimitive())
        //{
        CHECK_YAML_FIELD(input_yaml, "descriptor");
        std::string descriptor = input_yaml["descriptor"].as<std::string>();
        const ApiType* input_type =
            this->parseTypeStr(descriptor);
        CHECK_CONDITION(input_type->isExplicit(),
            fmt::format("Expected comprehension, got `{}`.", descriptor));
        obj = this->retrieveExplicitObject(
            dynamic_cast<const ExplicitType*>(input_type));
        this->fuzzer_input.insert(std::pair<std::string, const ApiObject*>(name, obj));
    }
}

void
ApiFuzzerNew::initTypes(YAML::Node types_config)
{
    for (YAML::Node type_yaml : types_config) {
        std::string type_name = type_yaml["name"].as<std::string>();
        bool singleton = false, pointer = false;
        if (type_yaml["singleton"].IsDefined())
        {
            singleton = type_yaml["singleton"].as<bool>();
        }
        if (type_yaml["pointer"].IsDefined())
        {
            pointer = type_yaml["pointer"].as<bool>();
        }
        logDebug(fmt::format("ADDING TYPE {}", type_name));
        logDebug(fmt::format("YAML SINGLE {}", type_yaml["singleton"].IsDefined()));
        this->addType(new ApiType(type_name, pointer, singleton));
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
    const ApiType* enclosing_class = nullptr;
    if (func_yaml["enclosing_class"].IsDefined())
    {
        std::string class_str = func_yaml["enclosing_class"].as<std::string>();
        enclosing_class = class_str == "" ? nullptr :
            this->parseTypeStr(class_str);
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
    return new ApiFunc(func_name, enclosing_class, return_type, param_type_list,
        cond_list, special, statik);
}

void
ApiFuzzerNew::initVariables(YAML::Node vars_yaml)
{
    for (YAML::Node var_yaml : vars_yaml)
    {
        CHECK_YAML_FIELD(var_yaml, "name");
        CHECK_YAML_FIELD(var_yaml, "type");
        std::string name = var_yaml["name"].as<std::string>();
        const ApiType* type =
            this->getTypeByName(var_yaml["type"].as<std::string>());
        if (type->isPrimitive())
        {
            // TODO do a switch for various primitive types
            const PrimitiveType* p_type =
                dynamic_cast<const PrimitiveType*>(type);
            if (var_yaml["value"].IsDefined())
            {
                this->generatePrimitiveObject(
                    p_type, name, var_yaml["value"].as<std::string>());
            }
            else
            {
                this->generatePrimitiveObject(p_type, name);
            }
        }
        else
        {
            //const ApiObject* init_obj =
                //this->generateApiObjectDecl(name, type, true);
            if (var_yaml["func"].IsDefined())
            {
                const ApiFunc* init_func =
                    this->getAnyFuncByName(var_yaml["func"].as<std::string>());
                const ApiObject* target_obj =
                    init_func->getClassType() != nullptr &&
                        init_func->checkFlag(std::string("!statik"))
                    ? this->generateObject(init_func->getClassType())
                    : nullptr;
                this->generateApiObject(name, type, init_func, target_obj,
                    this->getFuncArgs(init_func));
            }
            else
            {
                this->generateApiObjectDecl(name, type, true);
            }
        }
    }
}

void
ApiFuzzerNew::initConstructors(YAML::Node ctors_yaml)
{
    for (YAML::Node ctor_yaml : ctors_yaml) {
        CHECK_YAML_FIELD(ctor_yaml, "return_type");
        CHECK_YAML_FIELD(ctor_yaml, "param_types");
        const ApiType* return_type =
            this->parseTypeStr(ctor_yaml["return_type"].as<std::string>());
        std::string func_name;
        if (ctor_yaml["name"].IsDefined())
        {
            func_name = ctor_yaml["name"].as<std::string>();
        }
        else
        {
            func_name = return_type->toStr();
        }
        const ApiType* enclosing_class = nullptr;
        if (ctor_yaml["enclosing_class"].IsDefined())
        {
            enclosing_class =
                this->parseTypeStr(
                    ctor_yaml["enclosing_class"].as<std::string>());
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

	this->addFunc(new ApiFunc(func_name, enclosing_class, return_type,
            param_types_list, cond_list, new_func_special, new_func_statik,
            new_func_ctor));
    }
}

const ExplicitType*
ApiFuzzerNew::parseComprehension(std::string comprehension)
{
    logDebug(fmt::format("Parsing comprehension {}", comprehension));
    CHECK_CONDITION(comprehension.front() == delim_front,
        fmt::format("Expected front delimiter `{}` for comprehensions `{}`.",
            delim_front, comprehension));
    CHECK_CONDITION(comprehension.find(delim_mid) != std::string::npos,
        fmt::format("Expected middle delimiter `{}` for comprehensions `{}`.",
            delim_mid, comprehension));
    CHECK_CONDITION(comprehension.back() == delim_back,
        fmt::format("Expected back delimiter `{}` for comprehensions `{}`.",
            delim_back, comprehension));
    const ApiType* api_type = parseTypeStr(comprehension);
    CHECK_CONDITION(api_type->isExplicit(),
        fmt::format("Expected explicit type; got `{}`.", api_type->toStr()));
    return dynamic_cast<const ExplicitType*>(api_type);
}

/* @brief Returns an ApiType corresponding to the given string
 *
 * @details Parses the given string to generate a corresponding ApiType. There
 * are two main ways of presenting strings: either as the name of an existing
 * type, or as a comprehension. In case of the latter, the comprehension is
 * transformed to an ExplicitType, passing the definition and evaluating the
 * underlying type in order to maintain type safety until concretization to an
 * object
 *
 * @param type_str A string representing a type or comprehension to parse
 * @return An ApiType* is the string given represents an existing type, or an
 * ExplicitType in the case of a comprehension
 */

const ApiType*
ApiFuzzerNew::parseTypeStr(std::string type_str)
{
    logDebug(fmt::format("Parsing type string {}", type_str));
    if (type_str.front() == delim_front) {
        CHECK_CONDITION(type_str.back() == delim_back,
            fmt::format("Expected back delimiter `{}` in `{}`.",
                delim_back, type_str));

        if (type_str.find("output_var") != std::string::npos)
        {
            return new ExplicitType(type_str, this->getCurrOutputVar()->getType());
        }
        else if (type_str.find("loop_counter") != std::string::npos)
        {
            return new ExplicitType(type_str, this->getTypeByName("unsigned int"));
        }
        else if (type_str.find("var_name") != std::string::npos)
        {
            return new ExplicitType(type_str, this->getTypeByName("string"));
        }

        size_t mid_1 = type_str.find(delim_mid);
        size_t mid_2 = type_str.find(delim_mid, mid_1 + 1);
        CHECK_CONDITION(mid_1 != std::string::npos,
            fmt::format("Expected middle delimiter `{}` in type string `{}`",
                delim_mid, type_str));
        CHECK_CONDITION(mid_2 != std::string::npos,
            fmt::format(
                "Expected second middle delimiter `{}` in type string `{}`",
                delim_mid, type_str));

        std::string out_type = type_str.substr(1, mid_1 - 1);
        std::string gen_type = type_str.substr(mid_1 + 1, mid_2 - mid_1 - 1);
        std::string descr = type_str.substr(mid_2 + 1);
        descr.pop_back();
        if (!out_type.compare("var"))
        {
            if (!gen_type.compare("type"))
            {
                return new ExplicitType(type_str, this->getTypeByName(descr));
            }
            else if (!gen_type.compare("name"))
            {
                std::vector<const ApiObject*> filtered_objs =
                    this->filterAllObjs(&ApiObject::hasName, descr);
                CHECK_CONDITION(filtered_objs.size() == 1,
                    fmt::format("Expected one object with name `{}`; found {}.",
                        descr, filtered_objs.size()));
                return new ExplicitType(type_str,
                    filtered_objs.front()->getType());
            }
            else if (!gen_type.compare("input"))
            {
                return new ExplicitType(type_str,
                    this->getInputObject(descr)->getType());
            }
            else if (!gen_type.compare("new"))
            {
                return new ExplicitType(type_str, this->getTypeByName(descr));
            }
            else if (!gen_type.compare("latest"))
            {
                std::vector<const ApiObject*> filtered_objs =
                    filterObjList(this->getAllObjList(), &ApiObject::notIsPrimitive);
                if (descr.compare(""))
                {
                    filtered_objs = filterObjList(filtered_objs, &ApiObject::hasType,
                        this->getTypeByName(descr));
                }
                CHECK_CONDITION(filtered_objs.size() > 0,
                    fmt::format("Could not find latest variable of type `{}`.",
                        descr));
                return new ExplicitType(fmt::format("{}var{}id{}{}{}",
                    delim_front, delim_mid, delim_mid, filtered_objs.back()->getID(),
                    delim_back), filtered_objs.back()->getType());
            }
            CHECK_CONDITION(false,
                fmt::format("Var comprehension not implemented: {}", type_str));
        }
        else if (!out_type.compare("meta"))
        {
            if (!gen_type.compare("input"))
            {
                size_t input_id = stoi(descr);
                CHECK_CONDITION(this->meta_in_vars.size() > input_id,
                    fmt::format("Asked to retrieve input id {}, but only {} "
                                "available.",
                                input_id, this->meta_in_vars.size()));
                return new ExplicitType(type_str,
                    this->meta_in_vars.at(input_id)->getType());
            }
            else if (!gen_type.compare("gen"))
            {
                // TODO replace with generator type
                return new ExplicitType(type_str, this->meta_variant_type);
            }
            else if (!gen_type.compare("var"))
            {
                return new ExplicitType(type_str, this->meta_variant_type);
            }
            CHECK_CONDITION(false,
                fmt::format("Meta comprehension not implemented: {}", type_str));
        }
        return new ExplicitType(type_str, this->getTypeByName(out_type));
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
ApiFuzzerNew::parseRelationStringVar(std::string rel_string_var)
{
    if (rel_string_var[0] == '%')
    {
        CHECK_CONDITION(rel_string_var.length() <= 3,
            fmt::format("Expected metamorphic variable comprehension, got `{}`.",
                rel_string_var));
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
    else if (std::all_of(rel_string_var.begin(), rel_string_var.end(),
            [](char c) { return std::isdigit(c); }))
    {
        return new PrimitiveObject<unsigned int>(
            dynamic_cast<const PrimitiveType*>(this->getTypeByName("unsigned int")),
            std::stoi(rel_string_var), this->getNextID());
    }
    std::vector<const ApiObject*> filtered_objs = this->filterAllObjs(
        &ApiObject::hasName, rel_string_var);
    CHECK_CONDITION(filtered_objs.size() == 1,
        fmt::format("Expected one object with name `{}`, found `{}`",
            rel_string_var, filtered_objs.size()));
    return filtered_objs.at(0);
}

const ApiObject*
ApiFuzzerNew::parseRelationStringSubstr(std::string rel_substr)
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
        const ApiType* comprehension = parseTypeStr(rel_substr);
        CHECK_CONDITION(comprehension->isExplicit(),
            fmt::format("Expected ExplicitType, found {}.",
                comprehension->toStr()));
        return this->retrieveExplicitObject(
            dynamic_cast<const ExplicitType*>(comprehension));
    }
    else
    {
        return parseRelationStringVar(rel_substr);
    }
}

const FuncObject*
ApiFuzzerNew::parseRelationStringFunc(std::string rel_string)
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
    CHECK_CONDITION(paren_count == 0,
        fmt::format("Expected parenthesis to be closed in input `{}`; got `{}` "
                    "remaining open.", rel_string, paren_count));
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
    const ApiFunc* func = this->getFuncBySignature(func_name, nullptr,
        target ? target->getType() : nullptr, param_types);
    return new FuncObject(func, target, param_objs);
}

MetaRelation*
ApiFuzzerNew::parseRelationString(std::string rel_string, std::string rel_name)
{
    const ApiObject* store_result_var = nullptr;

    size_t eq_pos = rel_string.find('=');
    // TODO properly parse strings with multiple equals that are not assignments
    if (eq_pos != std::string::npos && rel_string.front() == '%' &&
            rel_string.find('.') > eq_pos)
            //rel_string.find('=', eq_pos + 1) == std::string::npos &&
            //(rel_string.find(delim_front) == std::string::npos ||
            //(rel_string.find(delim_front) != std::string::npos &&
             //rel_string.find(delim_front) > eq_pos)))
            //&&
             //rel_string.find(delim_back, eq_pos) > eq_pos &&
             //rel_string.find(delim_mid, eq_pos + 1) != std::string::npos))
    {
        store_result_var = this->parseRelationStringVar(
            rel_string.substr(0, eq_pos));
        rel_string = rel_string.substr(eq_pos + 1);
    }
    const FuncObject* base_func = parseRelationStringFunc(rel_string);
    return new MetaRelation(rel_name, base_func, store_result_var);
}

/**
* @brief Create or retrieve a reference, dispatching to appropriate functions
* based on given type
*
* Distinguish between four types of distinct ApiTypes, and appopriately call the
* respective function, which will either generate a new ApiObject, or retrieve
* a pointer to one, respectively. The distinct types we observe are:
* * Singleton ApiTypes, in which case if such an object exists, it is retrieved,
* otherwise a new instance is generated
* * Primitive types, for which the `generatePrimitiveObject` function is called
* * comprehensions (encoded as ExplicitType), which defers to
* `retrieveExplicitObject`
* * if none of the above apply, a new object is generated via a call to
* `generateNewObject`
*
* @param obj_type The type of the object to be retrieved or constructed
*
* @return A pointer to the appropriate object, generally newly constructed, or
* retrieved in special instances
*/

const ApiObject*
ApiFuzzerNew::generateObject(const ApiType* obj_type)
{
    CHECK_CONDITION(obj_type != nullptr,
        fmt::format("Given null type to generate object for."));
    logDebug("Generating object of type " + obj_type->toStr());

    const ApiObject* res;	

    if (obj_type->checkFlag("singleton"))
    {
        res = this->getSingletonObject(obj_type);
    }
    else if (obj_type->isPrimitive())
    {
        res = this->generatePrimitiveObject((PrimitiveType*) obj_type);
    }
    else if (obj_type->isExplicit())
    {
        res = this->retrieveExplicitObject(
            dynamic_cast<const ExplicitType*>(obj_type));
    }
    else
    {
        res = this->generateNewObject(obj_type);
    }

    return res;
}

/* @brief Evaluate given comprehension into corresponding object
 *
 * @details Evaluates the definition of the given ExplicitType in order to
 * return an object as requested, depending on the corresponding type,
 * generation method and description in the definition. Currently implemented
 * definitions, by type and generation method (note that description is used in
 * by the generation method):
 *  - var
 *      - name
 *      - type
 *      - new - Does *not* initialize or otherwise declare the new object;
 *      returns a reference to a freshly created object internally
 *      - id
 *      - input
 *  - special
 *      - output_var
 *  - primitive type
 *      - val
 *      - len
 *      - range
 *      - random
 *
 * @param expl_type ExplicitType representing a comprehension
 * @return Evaluation of given ExplicitType to corresponding object
 */

const ApiObject*
ApiFuzzerNew::retrieveExplicitObject(const ExplicitType* expl_type)
{
    logDebug("Making explicit object from description " + expl_type->getDefinition());
    if (!expl_type->getGenType().compare("var"))
    {
        if (!expl_type->getGenMethod().compare("name"))
        {
            std::vector<const ApiObject*> filtered_objs =
                this->filterAllObjs(&ApiObject::hasName,
                    expl_type->getDescriptor());
            CHECK_CONDITION(filtered_objs.size() == 1,
                fmt::format("Could not find object with name {}",
                    expl_type->getDescriptor()));
            return filtered_objs.front();
        }
        else if (!expl_type->getGenMethod().compare("type"))
        {
            std::vector<const ApiObject*> filtered_objs =
                this->filterAllObjs(&ApiObject::hasType,
                    expl_type->getUnderlyingType());
            if (filtered_objs.empty())
            {
                return addNewObj(expl_type->getUnderlyingType());
                //return this->generateNewObject(expl_type->getUnderlyingType());
            }
            return getRandomVectorElem(filtered_objs, this->getRNG());
        }
        else if (!expl_type->getGenMethod().compare("new"))
        {
            return this->addNewObj(
                this->getTypeByName(expl_type->getDescriptor()));
            //return this->generateNewObject(
                //this->getTypeByName(expl_type->getDescriptor()));
        }
        else if (!expl_type->getGenMethod().compare("id"))
        {
            size_t id_check;
            std::sscanf(expl_type->getDescriptor().c_str(), "%zu", &id_check);
            std::vector<const ApiObject*> filtered_objs =
                this->filterAllObjs(&ApiObject::hasID, id_check);
            CHECK_CONDITION(filtered_objs.size() == 1,
                fmt::format("Could not find object with id {}",
                    expl_type->getDescriptor()));
            return filtered_objs.front();
        }
        else if (!expl_type->getGenMethod().compare("input"))
        {
            return this->getInputObject(expl_type->getDescriptor());
        }
        CHECK_CONDITION(false,
            fmt::format("Not implemented var object generation method for `{}`.",
                expl_type->getGenMethod()));
    }
    else if (!expl_type->getGenType().compare("special"))
    {
        if (!expl_type->getGenMethod().compare("output_var"))
        {
            return this->getCurrOutputVar();
        }
        else if (!expl_type->getGenMethod().compare("var_name"))
        {
            const PrimitiveType* str_type =
                dynamic_cast<const PrimitiveType*>(this->getTypeByName("string"));
            CHECK_CONDITION(str_type != nullptr,
                "Could not retrieve primitive `string` type");
            return this->generatePrimitiveObject(str_type,
                this->getGenericVariableName(str_type),
                fmt::format("var_{}", this->getNextID()));
        }
        CHECK_CONDITION(false,
            fmt::format("Not implemented special object generation method for `{}`.",
                expl_type->getGenMethod()));
    }
    else if (!expl_type->getGenType().compare("meta"))
    {
        if (!expl_type->getGenMethod().compare("input"))
        {
            size_t input_id = stoi(expl_type->getDescriptor());
            CHECK_CONDITION(this->meta_in_vars.size() > input_id,
                fmt::format("Asked to retrieve input id {}, but only {} "
                            "available.",
                            input_id, this->meta_in_vars.size()));
            return this->meta_in_vars.at(input_id);
        }
        else if (!expl_type->getGenMethod().compare("gen"))
        {

        }
        else if (!expl_type->getGenMethod().compare("var"))
        {
            if (!expl_type->getDescriptor().compare("current"))
            {
                return this->smt->getCurrentMetaVar();
            }
            else
            {
                size_t meta_variant_id = stoi(expl_type->getDescriptor());
                CHECK_CONDITION(this->meta_vars.size() > meta_variant_id,
                    fmt::format("Asked to retrieve meta variant id {}, but only "
                                "{} available.",
                                meta_variant_id, this->meta_vars.size()));
                return this->meta_vars.at(meta_variant_id);
            }
        }
        CHECK_CONDITION(false,
            fmt::format("Not implemented meta object generation method for `{}`.",
                expl_type->getGenMethod()));
    }
    else if (expl_type->getUnderlyingType()->isPrimitive())
    {
        const PrimitiveType* prim_type = dynamic_cast<const PrimitiveType*>(
            expl_type->getUnderlyingType());
        std::string var_name = fmt::format("{}_{}", prim_type->toStr(),
            this->getNextID());
        if (!expl_type->getGenMethod().compare("val"))
        {
            switch(prim_type->getTypeEnum())
            {
                case INT:
                {
                    return this->generatePrimitiveObject(prim_type, var_name,
                                        std::stoi(expl_type->getDescriptor()));
                }
                case UINT:
                {
                    return this->generatePrimitiveObject(prim_type, var_name,
                                static_cast<unsigned int>(
                                    std::stoi(expl_type->getDescriptor())));
                }
                case STRING:
                case NQSTRING:
                {
                    return this->generatePrimitiveObject(prim_type, var_name,
                                        expl_type->getDescriptor());
                }
                default:
                {
                    CHECK_CONDITION(false,
                        fmt::format(
                            "Not implemented descriptor conversion for type {}.",
                            prim_type->toStr()));
                    assert(false);
                }
            }
        }
        else if (!expl_type->getGenMethod().compare("len") ||
                    !expl_type->getGenMethod().compare("range"))
        {
            return this->generatePrimitiveObject(prim_type, var_name,
                fmt::format("{}range{}{}{}", delim_front, delim_mid,
                    expl_type->getDescriptor(), delim_back));
        }
        else if (!expl_type->getGenMethod().compare("random"))
        {
            return this->generatePrimitiveObject(prim_type, var_name,
                expl_type->getDescriptor());
        }
        CHECK_CONDITION(false,
            fmt::format(
                "Not implemented primitive object generation method `{}`",
                expl_type->getGenMethod()));
    }
    CHECK_CONDITION(false,
        fmt::format("Not implemented explicit object generation for type {}",
            expl_type->getDefinition()));
    assert(false);
}

const ApiObject*
ApiFuzzerNew::generateNewObject(const ApiType* obj_type, const ApiObject* result_obj)
{
	//unsigned int start = this->instrs.size();
    //logDebug("DEPTH " + std::to_string(this->depth));
    if (obj_type->isPrimitive())
    {
        CHECK_CONDITION(result_obj == nullptr,
            fmt::format("Not implemented new primitive object generation for "
                        "targeted result obj"));
        const ApiObject* res = generatePrimitiveObject(dynamic_cast<const PrimitiveType*> (obj_type));

	NodeT* node = tree.insertNode(res);

	return res;
    }
    //else if (result_obj && result_obj->toInitialize() && !result_obj->isDeclared())
    //{
        //this->generateApiObjectDecl(
        //return result_obj;
    //}
    ++this->depth;
    std::set<const ApiFunc*> ctor_func_candidates = this->filterFuncs(
        &ApiFunc::hasReturnType, obj_type);
    if (this->depth >= this->max_depth)
    {
        logDebug(fmt::format("Reached max depth creating object type `{}`"
                            ", filtering constructors.", obj_type->toStr()));
        ctor_func_candidates = filterFuncList(ctor_func_candidates,
            &ApiFunc::checkFlag, std::string("ctor"));
    }
    else
    {
        ctor_func_candidates = filterFuncList(ctor_func_candidates,
            &ApiFunc::checkFlag, std::string("!special"));
        ctor_func_candidates = filterFuncList(ctor_func_candidates,
            &ApiFunc::checkFlag, std::string("!max_depth"));
        std::set<const ApiFunc*> non_ctor_func_cands = filterFuncList(ctor_func_candidates,
            &ApiFunc::checkFlag, std::string("!ctor"));
        // TODO this currently forces tests to be produced with full depth
        if (!non_ctor_func_cands.empty())
        {
            ctor_func_candidates = non_ctor_func_cands;
        }
    }
    CHECK_CONDITION(ctor_func_candidates.size() != 0,
        fmt::format(
            "Could not find candidate constructor functions for type `{}`.",
            obj_type->toStr()));
    logDebug("Candidate funcs:");
    for (const ApiFunc* func_cand : ctor_func_candidates)
    {
        logDebug(fmt::format("\t{}", func_cand->getName()));
    }
    const ApiFunc* gen_func = getRandomSetElem(ctor_func_candidates, this->getRNG());
    logDebug(fmt::format("Selected func = {}", gen_func->getName()));
    logDebug("Generating " + obj_type->toStr() + " type object with func " +
        gen_func->getName());

    /* Check for target type; if exists, retrieve appropriate ApiObject pointer.
     * If object not declared, recursively call `generateNewObject` */
    const ApiObject* target_obj = nullptr;
    if (gen_func->getClassType() != nullptr)
    {
        const ApiType* target_type = gen_func->getClassType();
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
        if (!target_obj->isDeclared())
        {
            this->generateNewObject(target_type, target_obj);
        }
    }

    /* Retrieve appropriate function arguments; initialise those pointers which
     * have not been previously declared. */
    std::vector<const ApiObject*> ctor_args = this->getFuncArgs(gen_func);
    for (const ApiObject* ctor_arg : ctor_args)
    {
        if (!ctor_arg->isDeclared())
        {
            this->generateNewObject(ctor_arg->getType(), ctor_arg);
        }
    }

    if (result_obj != nullptr)
    {
        CHECK_CONDITION(result_obj->hasType(obj_type),
            fmt::format("Invalid types given for result object `{}` and "
                        "expected type `{}`.", result_obj->getType()->toStr(),
                        obj_type->toStr()));

        this->applyFunc(gen_func, target_obj, result_obj, ctor_args);

        --this->depth;

	// Added by Pritam	
	const ApiInstructionInterface* instrNode;
	instrNode = this->instrs.at(this->instrs.size()-1);	

	NodeT* node = tree.insertNode(result_obj);	

	std::vector<NodeT*> d_child;

	tree.addRoot(node);

	NodeT* target_node = null_node;	

	if(target_obj != NULL)
	{	
		target_node = tree.insertNode(target_obj);

		d_child.push_back(target_node);
	}

//	tree.addEdge(node, target_node);

	const ApiObject* param_obj;

	for(std::vector<const ApiObject*>::iterator it = ctor_args.begin(); it != ctor_args.end(); it++)
	{
		NodeT* temp_node = null_node;

		param_obj = *it;

		if(param_obj != NULL)
		{
			temp_node = tree.insertNode(param_obj);
		
			d_child.push_back(temp_node);
		}

//		tree.addEdge(node, temp_node);
	}	

	tree.addEdge(node, d_child, instrNode);
		
	// End of Added Code

        return result_obj;
    }

    std::string var_name = this->getGenericVariableName(obj_type);
    // hack
    while (var_name.find('*') != std::string::npos)
    {
        var_name = var_name.replace(var_name.find('*'), 1, "");
    }
    --this->depth;

    const ApiObject* res = generateApiObject(var_name, obj_type, gen_func, target_obj, ctor_args);

    // Added by Pritam	
    bool no_target = false;
    bool no_param = false;	
    const ApiInstructionInterface* instrNode;
    instrNode = this->instrs.at(this->instrs.size()-1);	

    NodeT* node = tree.insertNode(res);	

    std::vector<NodeT*> d_child;

    NodeT* target_node = null_node;	

    if(target_obj != NULL)
    {	
    	target_node = tree.insertNode(target_obj);

	d_child.push_back(target_node);
    }
    else
    {
	no_target = true;
    }	

//    tree.addEdge(node, target_node);

    const ApiObject* param_obj;

    for(std::vector<const ApiObject*>::iterator it = ctor_args.begin(); it != ctor_args.end(); it++)
    {
       	 NodeT* temp_node = null_node;

	param_obj = *it;

        if(param_obj != NULL)
	{
		temp_node = tree.insertNode(param_obj);

		d_child.push_back(temp_node);
	}

//	tree.addEdge(node, temp_node);
    }	

    if(d_child.size() == 0)
    {
        no_param = true;
    }	

    tree.addEdge(node, d_child, instrNode);

    if(no_target && no_param)
    {
    	if(try_outs == 0)
	{
           special_obj = std::make_pair(res, instrNode);
	   try_outs = 1;
	}	
    }
		
    return res;
}

/* @brief Return a reference to the input object with given name
 *
 * @param input_name Name of input object to search for
 * @return Reference to ApiObject representing respective input object
 */

const ApiObject*
ApiFuzzerNew::getInputObject(std::string input_name)
{
    logDebug("Looking for input " + input_name);
    assert(this->fuzzer_input.count(input_name) != 0);
    return this->fuzzer_input[input_name];
}

/* @brief Return the data of the corresponding primitive input object
 *
 * @param input_name Name of primitive input object
 * @return Data value of input object with given name
 */

template<typename T>
T
ApiFuzzerNew::getInputObjectData(std::string input_name)
{
    const ApiObject* input_obj = this->getInputObject(input_name);
    assert(input_obj->isPrimitive());
    return dynamic_cast<const PrimitiveObject<T>*>(input_obj)->getData();
}

template<>
unsigned int
ApiFuzzerNew::parseDescriptor<unsigned int>(std::string descriptor)
{
    std::pair<int, int> int_range = this->parseRange(descriptor);
    return static_cast<unsigned int>
        (this->getRandInt(int_range.first, int_range.second));
}

template<>
int
ApiFuzzerNew::parseDescriptor<int>(std::string descriptor)
{
    std::pair<int, int> int_range = this->parseRange(descriptor);
    return this->getRandInt(int_range.first, int_range.second);
}

template<>
std::string
ApiFuzzerNew::parseDescriptor<std::string>(std::string descriptor)
{
    std::pair<size_t, size_t> len_range = this->parseRange(descriptor);
    size_t len = this->getRandInt(len_range.first, len_range.second);
    std::mt19937* rng = this->getRNG();
    auto rand_char = [&rng]() -> char
        {
            std::map<std::string, std::vector<char>>::iterator rand_char_set =
                char_set.begin();
            std::advance(rand_char_set, (*rng)() % char_set.size());
            return getRandomVectorElem((*rand_char_set).second, rng);
        };
    std::string new_str(len, 0);
    std::generate_n(new_str.begin(), len, rand_char);
    return new_str;
}

template<>
char
ApiFuzzerNew::parseDescriptor<char>(std::string descriptor)
{
    std::map<std::string, std::vector<char>>::iterator restricted_char_set_it =
        char_set.find(descriptor);
    if (char_set.count(descriptor) != 0)
    {
        restricted_char_set_it = char_set.find(descriptor);
    }
    else
    {
        restricted_char_set_it = char_set.begin();
        std::advance(restricted_char_set_it,
            this->getRandInt(0, char_set.size() - 1));
    }
    return getRandomVectorElem((*restricted_char_set_it).second, this->getRNG());
}

template<typename T>
T
ApiFuzzerNew::parseDescriptor(std::string descriptor)
{
    assert(false);
}

/**
* @brief A generic PrimitiveObject generation method
*
* This method generates a PrimitiveObject with a generic name based on given
* type and hard-coded descriptor, defined further down the call chain. Useful
* when need an object of the underlying primitive type, but without specific
* requirements.
*
* @param obj_type The exposed PrimitiveType
*
* @return A pointer to the newly created ApiObject
*/
const ApiObject*
ApiFuzzerNew::generatePrimitiveObject(const PrimitiveType* obj_type)
{
    return this->generatePrimitiveObject(obj_type,
        fmt::format("{}_{}", obj_type->toStr(), this->getNextID()));
}

/**
* @brief Creates primitive objects with pre-defined descriptors
*
* For each primitive type, defines a specific descriptor in order to generate a
* fairly generic and representative PrimitiveObject.
*
* @param obj_type The exposed PrimitiveType
* @param name Name of the new ApiObject to create
*
* @return A pointer to the newly created ApiObject
*/
const ApiObject*
ApiFuzzerNew::generatePrimitiveObject(const PrimitiveType* obj_type,
    std::string name)
{
    switch(obj_type->getTypeEnum()) {
        case UINT: {
            std::string range = "[0,10]";
            range = fmt::format("{}range{}{}{}", delim_front, delim_mid,
                range, delim_back);
            return this->generatePrimitiveObject(obj_type, name, range);
        }
        case STRING:
        case NQSTRING: {
            std::string range = "[10,20]";
            range = fmt::format("{}range{}{}{}", delim_front, delim_mid,
                range, delim_back);
            return this->generatePrimitiveObject(obj_type, name, range);
        }
        case CHAR: {
            return this->generatePrimitiveObject(obj_type, name,
                std::string("all"));
        }
        case INT: {
            std::string range = "[-10,10]";
            range = fmt::format("{}range{}{}{}", delim_front, delim_mid,
                range, delim_back);
            return this->generatePrimitiveObject(obj_type, name, range);
        }
        default:
            CHECK_CONDITION(false,
                fmt::format("`{}` type enum default value not implemented.",
                obj_type->toStr()));
            assert(false);
    }
}

/**
* @brief Generates data corresponding to the descriptor, then calls the
* primitive object generation method
*
* Parses the given descriptor into a single value of the corresponding
* underlying primitive type, via the parseDescriptor functions. As the
* descriptor is a string, specially handles the situation where a string
* PrimitiveObject must be generated, by creating the object on the spot, after
* parsing the descriptor (which might represent a value or a comprehension)
*
* @param obj_type The exposed PrimitiveType
* @param name Name of the eventual ApiObject
* @param descriptor String representation of comprehension which dictates
* content of the object
*
* @return A pointer to the newly created ApiObject
*/
const ApiObject*
ApiFuzzerNew::generatePrimitiveObject(const PrimitiveType* obj_type,
    std::string name, std::string descriptor)
{
    logDebug("Generating primitive object with type " + obj_type->toStr() +
        " and descriptor " + descriptor);
    switch(obj_type->getTypeEnum()) {
        case UINT: {
            assert(descriptor.front() == delim_front);
            return this->generatePrimitiveObject<unsigned int>(obj_type,
                obj_type->toStr(), this->parseDescriptor<unsigned int>(descriptor));
        }
        case STRING:
        case NQSTRING: {
            if (descriptor.front() == delim_front)
            {
                descriptor = this->parseDescriptor<std::string>(descriptor);
            }
            const ApiObject* new_obj =
                new PrimitiveObject<std::string>(obj_type, descriptor, name,
                    this->getNextID());
            this->addObj(new_obj);
            return new_obj;
        }
        case CHAR: {
            return this->generatePrimitiveObject<char>(obj_type,
                obj_type->toStr(), this->parseDescriptor<char>(descriptor));
        }
        case INT: {
            return this->generatePrimitiveObject<int>(obj_type,
                obj_type->toStr(), this->parseDescriptor<int>(descriptor));
        }
        case BOOL:
        default:
            CHECK_CONDITION(false, fmt::format("`{}` type enum not implemented.",
                obj_type->toStr()));
            assert(false);
    }
}

/**
* @brief Create a PrimitiveObject with data of primitive type T
*
* @tparam T The underlying primitive type
* @param obj_type The fuzzer-exposed ApiType
* @param name The name of the object (generally unused)
* @param data The underlying data of type T
*
* @return A reference to the newly created ApiObject
*/
template<typename T>
const ApiObject*
ApiFuzzerNew::generatePrimitiveObject(const PrimitiveType* obj_type,
    std::string name, T data)
{
    assert(obj_type->isPrimitive());
    //assert(obj_type->hasName("unsigned int") || obj_type->hasName("string"));
    logDebug("Generating primitive object with type " + obj_type->toStr());
    const ApiObject* new_obj = new PrimitiveObject<T>(obj_type, data, name,
        this->getNextID());
    CHECK_CONDITION(new_obj != nullptr, "Could not generate primitive object.");
    this->addObj(new_obj);
    return new_obj;
}

/**
* @brief Returns a reference to a singleton ApiObject
*
* For a `obj_type` that has the `singleton` flag set, searches and retrieves the
* corresponding ApiObject from the symbol table, or creates one if no such
* reference exists. Note that singleton objects are unique across a test file,
* rather than a metamorphic input generation step.
*
* @param obj_type The singleton type to search for an instance of
*
* @return A reference to an ApiObject of that type
*/
const ApiObject*
ApiFuzzerNew::getSingletonObject(const ApiType* obj_type)
{
    std::vector<const ApiObject*> filtered_objs = this->filterAllObjs(
        &ApiObject::hasType, obj_type);
    //CHECK_CONDITION(filtered_objs.size() <= 1,
        //fmt::format("More than one singleton object instances found."));
    if (filtered_objs.size() == 0)
    {
        logDebug(fmt::format(
                 "Unable to find instance of singleton object of type `{}`, "
                 "generating new instance.", obj_type->toStr()));
        return generateNewObject(obj_type);
    }
    return filtered_objs.at(0);
}

/**
* @brief Main entry-point for sequence generation in the fuzzer
*
* Reads information from the `seq_gen` field of the fuzzer specification and
* dispatches it to the corresponding instruction generation function.
*/
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
        else if (!gen_instr_type.compare("seq"))
        {
            logDebug("Make seq");
            assert(gen_instr_yaml["count"].IsDefined());
            this->generateSeq(gen_instr_yaml["count"].as<size_t>());
        }
        else
        {
            CHECK_CONDITION(false,
                fmt::format("Not implemented sequence generation method {}.",
                    gen_instr_type));
        }
    }
}

/**
* @brief Generates a random sequence of instructions, up to the given
* `seq_count`
*
* This generation method randomly chooses random function calls to be applied,
* with minimal control, just like random testing. This might be useful to start
* out a new fuzzer specification, but should eventually be replaced with a
* proper thought-out generation sequence. Note that `seq_count` represents base
* random instructions, to which the depth limit still applies.
*
* @param seq_count The number of **base** random instructions to generate
*/

void ApiFuzzerNew::insertInstructionInTheTree(const ApiInstructionInterface* int_instr)
{
	const ApiInstruction* instr;
	
	instr = dynamic_cast<const ApiInstruction*>(int_instr);

        const ApiObject* t_obj = NULL;
        const ApiObject* r_obj = NULL;
        const ApiObject* p_obj = NULL;
	std::vector<const ApiObject*> param_list;

	t_obj = instr->getTargetObj();
	r_obj = instr->getResultObj();
	param_list = instr->getFuncParams();

	NodeT* t_node = NULL;	
	NodeT* r_node = NULL;	
	NodeT* p_node = NULL;	
	std::vector<NodeT*> param_nodes;
	const ApiInstruction* new_instr;

	if(t_obj)
	{
		t_node = this->tree.insertNode(t_obj);
	}

	if(r_obj)
	{
		r_node = this->tree.insertNode(r_obj);
	}

	if(t_node != NULL)
	{
		param_nodes.push_back(t_node);
	}

	for(std::vector<const ApiObject*>::iterator it = param_list.begin(); it != param_list.end(); it++)
	{
		p_obj = *it;

		if(p_obj)
		{
			p_node = this->tree.insertNode(p_obj);

			if(p_node)
			{
				param_nodes.push_back(p_node);
			}
		}
	}

	if(instr->isDeclInstr())
	{
		r_obj->resetDeclared();
	}

	this->tree.addEdge(r_node, param_nodes, int_instr);
}

void
ApiFuzzerNew::generateSeq(size_t seq_count)
{
    std::set<const ApiFunc*> func_list = this->filterFuncs(
        &ApiFunc::checkFlag, std::string("!special"));
    while (seq_count > 0)
    {
        const ApiFunc* seq_func = getRandomSetElem(func_list, this->getRNG());
        this->applyFunc(seq_func);

	const ApiInstructionInterface* api_new_instr;
	api_new_instr = this->instrs.at(this->instrs.size()-1);	
	insertInstructionInTheTree(api_new_instr);

        seq_count--;
    }
}

/**
* @brief Generates a declaration for the corresponding variables
*
* Declares and adds a variable of the given `var_type` to the symbol table,
* without initialising the variable. Useful in situation where variables are
* requested for function which perform in-place transformations
*
* @param instr_config A YAML node with information pertaining to the declaration
* instruction
*
* @todo Review why the false assertion is there
*/
void
ApiFuzzerNew::generateDecl(YAML::Node instr_config)
{
    const ApiType* var_type =
        this->getTypeByName(instr_config["var_type"].as<std::string>());
    if (var_type->checkFlag("singleton") && this->filterAllObjs(&ApiObject::hasType,
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
        const ApiObject* obj = this->generateApiObjectDecl(var_name, var_type, true);

	NodeT* new_node = this->tree.insertNode(obj);

	std::vector<NodeT*> dests;
	const ApiInstructionInterface* api_new_instr;
	api_new_instr = this->instrs.at(this->instrs.size()-1);	

	this->tree.addEdge(new_node, dests, api_new_instr);
    }
}

/**
* @brief Generates sequences of function calls equivalent to unrolling a for
* loop
*
* Correpsonds to a `for` type sequence instruction. Based on the given `counter`
* range, this call generates `func` type sequence instructions, where each
* instruction has access to a distinct, unique `loop_counter` variable. The
* calls are generated unrolled in the eventual test case.
*
* @param instr_config A YAML node with information pertaining to the `counter`
* and underlying `func` instruction
*/
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

/* @brief Parses a string representation of a mathematical range into a pair of
 * integers
 *
 * The given range_str argument is expected to have the same syntax as a
 * mathematical range. Given that, it returns a pair of integers, representing
 * the evaluated range limits. Further, the function also parses comprehensions
 * that return integer values.
 *
 * @param range_str A string representing a mathematical range
 * @return A pair of integers representing the minimum and maximum values,
 * inclusive, of the parsed range
 *
 * @todo Further polish comprehensions and ensure they return integer values
 */

std::pair<int, int>
ApiFuzzerNew::parseRange(std::string range_str)
{
    if (range_str.front() == delim_front)
    {
        range_str = this->getGeneratorData(range_str);
    }
    CHECK_CONDITION(range_str.find(",") != std::string::npos,
        fmt::format("Expected comma in range string not found."));
    CHECK_CONDITION(range_str.front() == '(' || range_str.front() == '[',
        fmt::format("Invalid start character for range parsing; `{}` given, "
                    "expected `(` or `[`", range_str.front()));
    CHECK_CONDITION(range_str.back() == ')' || range_str.back() == ']',
        fmt::format("Invalid end character for range parsing; `{}` given, "
                    "expected `)` or `]`", range_str.back()));
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

/**
 * @brief Parses a comprehension from a range string
 *
 * @details For a range comprehension, allow usage of further comprehensions to
 * represent specific values for sides of the interval, especially useful for
 * cases fuch as a `for` generation instruction.
 *
 * @param range_substr Part of a range comprehension referring to one of the
 * limits of the interval
 * @return The integer value of the given limit comprehension
 *
 * @todo Consider replacing with `parseTypeStr`
 */

int
ApiFuzzerNew::parseRangeSubstr(std::string range_substr)
{
    if (range_substr.front() == delim_front)
    {
        CHECK_CONDITION(range_substr.back() == delim_back,
            fmt::format("Expected ending delimiter `{}` in substring `{}`.",
                delim_back, range_substr));
        CHECK_CONDITION(range_substr.find(delim_mid) != std::string::npos,
            fmt::format("Expected middle delimiter `{}` in substring `{}`.",
                delim_mid, range_substr));
        const ApiType* parsed_type = parseTypeStr(range_substr);
        CHECK_CONDITION(parsed_type->isExplicit(),
            fmt::format("Expected comprehension, got `{}` with type `{}`.",
                range_substr, parsed_type->toStr()));
        const ApiObject* range_obj =
            retrieveExplicitObject(
                dynamic_cast<const ExplicitType*>(parsed_type));
        CHECK_CONDITION(range_obj->isPrimitive(),
            fmt::format("Expected object of primitive type, got object of type `{}`.",
                range_obj->getType()->toStr()));
        switch (range_obj->getType()->getTypeEnum())
        {
            case UINT:
                return dynamic_cast<const PrimitiveObject<unsigned int>*>(
                    range_obj)->getData();
            case INT:
                return dynamic_cast<const PrimitiveObject<int>*>(
                    range_obj)->getData();
            default:
                CHECK_CONDITION(false,
                    fmt::format("Expected `int` type, got type `{}`.",
                        range_obj->getType()->toStr()));
        }
        assert(false);
    }
    else
    {
        for (char& c : range_substr)
        {
            if (c == '-')
            {
                continue;
            }
            assert(std::isdigit(c));
        }
        // TODO more checks here
        return atoi(range_substr.c_str());
    }
    assert(false);
}

/**
 * @brief Builds a function call from provided YAML instructions
 *
 * @details Uses information contained within `instr_config` to yield a single
 * function call, as described. `instr_config` must at the least contain the
 * name of the function to be generated. `instr_config` might optionally set an
 * enclosing object for member functions, a target to return the result to, or
 * specific parameter values to be passed to the function.
 *
 * @param instr_config A node containing information about the function to be
 * generated; must include name of function.
 * @param loop_counter Reference to loop counter for functions calls generated
 * by an unrolled `for` loop
 * @return void - the results are stored directly in the symbol table of the
 * fuzzer
 */

void
ApiFuzzerNew::generateFunc(YAML::Node instr_config, int loop_counter)
{
    unsigned int start = this->instrs.size();

    CHECK_CONDITION(instr_config["func"].IsDefined(),
        "Fuzzer generation instruction must have `func` field defined.");
    std::string func_name = instr_config["func"].as<std::string>();
    const ApiFunc* func;
    if (instr_config["func_params"].IsDefined())
    {
        std::vector<const ApiType*> param_types;
        for (YAML::Node func_param_yaml : instr_config["func_params"])
        {
            param_types.push_back(
                this->parseTypeStr(
                    func_param_yaml.as<std::string>())->getUnderlyingType());
        }
        func = this->getFuncBySignature(func_name, nullptr, nullptr, param_types);
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
            target_obj = this->getCurrOutputVar(func->getClassType());
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
        CHECK_CONDITION(target_obj->getType()->isType(func->getClassType()),
            fmt::format("Generated target object type `{}` does not match "
                        "expected type `{}`.", target_obj->getType()->toStr(),
                        func->getClassType()->toStr()));
    }
    else if (func->getClassType() && !func->checkFlag("statik"))
    {
        target_obj = this->generateObject(func->getClassType());
    }

    // Set return object, if any declared
    const ApiObject* return_obj = nullptr;
    // TODO should we automatically create a return object if not defined?
    //bool gen_new_obj = false;
    //bool gen_new_named_obj = false;
    if (instr_config["return"].IsDefined())
    {
        std::string return_type_str = instr_config["return"].as<std::string>();
        if (return_type_str.front() == delim_front)
        {
            return_obj = retrieveExplicitObject(parseComprehension(return_type_str));
        }
        else
        {
            std::vector<const ApiObject*> candidate_objs =
                this->filterObjs(&ApiObject::hasName, return_type_str);
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

    bool special = false;

    // Set function parameters
    std::vector<const ApiObject*> func_params;
    if (instr_config["func_params"].IsDefined())
    {
        for (YAML::Node func_param_yaml : instr_config["func_params"])
        {
            std::string type_str = func_param_yaml.as<std::string>();
            const ApiType* type = this->parseTypeStr(type_str);
            if (type->isExplicit() &&
                   !dynamic_cast<const ExplicitType*>(type)->getGenMethod()
                    .compare("loop_counter"))
            {
                func_params.push_back(new PrimitiveObject<unsigned int>(
                    dynamic_cast<const PrimitiveType*>(
                    this->getTypeByName("unsigned int")), loop_counter,
                    this->getNextID()));
            }
            else
            {
                const ApiObject* param_obj = this->generateObject(type);
                if ((type->isExplicit() &&
                        !dynamic_cast<const ExplicitType*>(type)->getGenMethod()
                            .compare("new")) || !param_obj->isDeclared())
                {
                    this->generateNewObject(type->getUnderlyingType(), param_obj);
                }
                func_params.push_back(param_obj);
            }
        }
    }
    else
    {
        func_params = this->getFuncArgs(func);
    }

    this->applyFunc(func, target_obj, return_obj,
        func_params);

    // Added by Pritam	
	const ApiInstructionInterface* instrNode;
	instrNode = this->instrs.at(this->instrs.size()-1);	

	NodeT* node = tree.insertNode(return_obj);	

	std::vector<NodeT*> d_child;

	NodeT* target_node = null_node;	

	if(target_obj != NULL)
	{	
		target_node = tree.insertNode(target_obj);

		d_child.push_back(target_node);
	}

//	tree.addEdge(node, target_node);

	const ApiObject* param_obj;

	for(std::vector<const ApiObject*>::iterator it = func_params.begin(); it != func_params.end(); it++)
	{
		NodeT* temp_node = null_node;

		param_obj = *it;

		if(param_obj != NULL)
		{
			temp_node = tree.insertNode(param_obj);

			d_child.push_back(temp_node);
		}

//		tree.addEdge(node, temp_node);
	}	

	tree.addEdge(node, d_child, instrNode);
		
	// End of Added Code
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

[[deprecated]]
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

/**
* @brief Returns a corresponding FuncObject for a given generator MetaVarObject
*
* For the given `mv_obj`, which must be a generator type (i.e. have a list of
* `meta_relations`), randomly select one such relation and return its base
* FuncObject, which is a template to be further concretized.
*
* @param mv_obj A generator MetaVarObject to unroll into a given expression as
* defined in the metamorphic specification
*
* @return A random, unconcretized FuncObject, representing the generated type of
* the given `mv_obj`
*/
const FuncObject*
ApiFuzzerNew::concretizeGenerators(const MetaVarObject* mv_obj, bool simplify)
{
    CHECK_CONDITION(mv_obj->isGenerator(),
        fmt::format("Asked to concretize non-generator metamorphic "\
                    " comprehension {}.", mv_obj->identifier));

    if(!simplify)
    {		
    	return this->concretizeGenerators(getRandomVectorElem(mv_obj->meta_relations, this->getRNG())->getBaseFunc(), simplify);
    }
    else	
    {		
    	return this->concretizeGenerators(mv_obj->meta_relations.at(0)->getBaseFunc(), simplify);
    }
}

/**
* @brief Unrolls all instances of generator metamorphic comprehensions
*
* Iterates through all possible ApiObject references for `func_obj` (i.e. target
* object and all parameters), and unrolls any generator MetaVarObjs into
* corresponding FuncObjs. Is called recursively on any FuncObj references in the
* given func_obj.
*
* @param func_obj Base FuncObj for which to unroll generator comprehensions
*
* @return A new FuncObj instance with all generator comprehensions replaced by
* concrete ApiObject references
*/
const FuncObject*
ApiFuzzerNew::concretizeGenerators(const FuncObject* func_obj, bool simplify)
{
    const ApiObject* target_obj = func_obj->getTarget();

    if (func_obj->getTarget())
    {
        const FuncObject* func_obj_param = dynamic_cast<const FuncObject*>(target_obj);
        const MetaVarObject* mv_target_obj = dynamic_cast<const MetaVarObject*>(target_obj);
        if (func_obj_param)
        {
            target_obj = this->concretizeGenerators(func_obj_param, simplify);
        }
        else if (mv_target_obj && mv_target_obj->isGenerator())
        {
            target_obj = this->concretizeGenerators(mv_target_obj, simplify);
        }
    }

    std::vector<const ApiObject*> gen_concrete_params;
    for (const ApiObject* param : func_obj->getParams())
    {
        const FuncObject* func_obj_param = dynamic_cast<const FuncObject*>(param);
        const MetaVarObject* mv_obj_param = dynamic_cast<const MetaVarObject*>(param);
        if (func_obj_param)
        {
            gen_concrete_params.push_back(this->concretizeGenerators(func_obj_param, simplify));
        }
        else if (mv_obj_param && mv_obj_param->isGenerator())
        {
            gen_concrete_params.push_back(this->concretizeGenerators(mv_obj_param, simplify));
        }
        else
        {
            gen_concrete_params.push_back(param);
        }
    }

    FuncObject* res;
    res = new FuncObject(func_obj->getFunc(), target_obj, gen_concrete_params);


    return res;
}

/**
* @brief Creates a map which describes how to replace the abstract MetaVarObjs
* for a given relation
*
* Expects a list of objects from a FuncObj representing a concrete metamorphic
* relation after all generator comprehensions have been resolved. First filters
* all those objects which are not MetaVarObjs. Then, for each MetaVarObj
* remaining, looks for the corresponding concrete object in the symbol table,
* and adds the resulting pair as an entry to the concretizing map.
*
* @param mvo_list A list of ApiObjects of a concrete metamorphic relations
*
* @return A map representing pairs of abstract comprehensions and their
* respective concrete objects
*/
std::map<const MetaVarObject*, const ApiObject*>
ApiFuzzerNew::makeConcretizationMap(std::vector<const ApiObject*> obj_list)
{
    std::map<const MetaVarObject*, const ApiObject*> concrete_map;
    for (const ApiObject* obj : obj_list)
    {
        const MetaVarObject* mv_obj = dynamic_cast<const MetaVarObject*>(obj);
        if (!mv_obj)
        {
            continue;
        }
        const ApiObject* value = nullptr;
        CHECK_CONDITION(mv_obj->meta_relations.empty(),
            fmt::format("Found unrolled generator comprehension {}.",
                mv_obj->getIdentifier()));
        if (!mv_obj->getIdentifier().compare("<m_curr>"))
        {
            value = this->smt->getCurrentMetaVar();
        }
        else if (mv_obj->getIdentifier().front() == 'm')
        {
            assert(std::isdigit(mv_obj->getIdentifier()[1]));
            size_t meta_variant_id = stoi(mv_obj->getIdentifier().substr(1));
            value = this->getMetaVariant(meta_variant_id);
        }
        else if (std::isdigit(mv_obj->getIdentifier().front()))
        {
            size_t input_id = stoi(mv_obj->getIdentifier()) - 1;
            CHECK_CONDITION(input_id < this->meta_in_vars.size(),
                fmt::format("Expected input var id `{}`, but have only `{}`.",
                input_id, this->meta_in_vars.size()));
            value = this->meta_in_vars.at(input_id);
        }
        CHECK_CONDITION(value,
            fmt::format("Unable to resolve comprehension {}.",
                mv_obj->getIdentifier()));
        concrete_map.emplace(std::make_pair(mv_obj, value));
    }

    return concrete_map;
}

/**
 * @brief Creates a concrete application of a MetaRelation
 *
 * Takes a pointer to an abstract MetaRelation object (including
 * comprehensions and metamorphic generators) and yields a MetaRelation object
 * which references only existing objects in the current test generation.
 * Handles any additional object creation requirements
 *
 * @param abstract_rel Abstract metamorphic relation to concretize
 * @param curr_meta_variant Instance of currently generated metamorphic variant
 * @return Concrete instance of abstract relation, with all comprehensions
 * solved
 *
 * @remark Metamorphic testing generation function
 */

const MetaRelation*
ApiFuzzerNew::concretizeRelation(const MetaRelation* abstract_rel,
    const ApiObject* curr_meta_variant, bool first, bool simplify)
{
    const MetaVarObject* abstract_result_var =
        dynamic_cast<const MetaVarObject*>(abstract_rel->getStoreVar());

    const ApiObject* concrete_result_var = nullptr;
    if (abstract_result_var)
    {
        concrete_result_var =
            abstract_result_var->getConcreteVar(
                curr_meta_variant, this->meta_variants, this->meta_in_vars);
    }
    const FuncObject* unrolled_func_obj =
        this->concretizeGenerators(abstract_rel->getBaseFunc(), simplify);
    std::map<const MetaVarObject*, const ApiObject*> concretize_map
        = this->makeConcretizationMap(unrolled_func_obj->getAllObjs());
    // HACK should perform the initialization check better
    if (!first)
    {
        concretize_map.at(this->getMetaVar("1")) = curr_meta_variant;
        // TODO once unified maps, can do random input replacement
        /*
        do
        {
            try
            {
                MetaVarObject* mv_obj = this->getMetaVar(std::to_string(
                    getRandInt(1, this->meta_in_vars.size())));
                concretize_map.at(mv_obj) = curr_meta_variant;
                logDebug(fmt::format("Replaced meta var {} with curr_meta_var.",
                    mv_obj->getIdentifier()));
                break;
            }
            catch (const std::out_of_range e)
            {
                continue;
            }
        } while (true);
        */
    }
    const FuncObject* concrete_func_obj =
        this->concretizeFuncObject(unrolled_func_obj, concretize_map);

    return new MetaRelation(abstract_rel->getAbstractRelation(),
        concrete_func_obj, concrete_result_var);
}

/**
 * @brief Concretizes the comprehensions used in a FuncObject
 *
 * Goes over the target object and each parameter of the provided FuncObject; if
 * a MetaVarObject is detected, it is transformed into the corresponding
 * ApiObject; if a FuncObject is detected, the function is called recursively.
 *
 * @param func_obj The FuncObject which to make concrete
 * @return A FuncObject with all comprehensions replaced with explcit objects
 */
const FuncObject*
ApiFuzzerNew::concretizeFuncObject(const FuncObject* func_obj,
    std::map<const MetaVarObject*, const ApiObject*> concretize_map)
{
    /* Concretize class instance, if member function */
    const ApiObject* func_target = func_obj->getTarget();
    if (func_target != nullptr)
    {
        if (dynamic_cast<const MetaVarObject*>(func_target))
        {
            func_target = concretize_map.at(dynamic_cast<const MetaVarObject*>(func_target));
        }
        else if (dynamic_cast<const FuncObject*>(func_target))
        {
            func_target = this->concretizeFuncObject(
                dynamic_cast<const FuncObject*>(func_target), concretize_map);
        }
        else
        {
            CHECK_CONDITION(false, "Should not get here");
        }
    }

    /* Concretize parameters */
    std::vector<const ApiObject*> concrete_params;
    for (const ApiObject* param : func_obj->getParams())
    {
        if (dynamic_cast<const MetaVarObject*>(param))
        {
            concrete_params.push_back(
                concretize_map.at(dynamic_cast<const MetaVarObject*>(param)));
        }
        else if (dynamic_cast<const FuncObject*>(param))
        {
            concrete_params.push_back(this->concretizeFuncObject(
                dynamic_cast<const FuncObject*>(param), concretize_map));
        }
        else
        {
            concrete_params.push_back(param);
        }
    }
    return new FuncObject(func_obj->getFunc(), func_target,
        concrete_params);
}


/**
 * @brief Returns the corresponding ApiObject of a given MetaVarObject
 *
 * Parses the given MetaVarObject and returns the concrete reference to the
 * actual object as part of the symbol table. Handles the following situations:
 * * %m - represents the current metamorphic variant being generated
 * * %mn - where n is a number, represents the nth metamorphic variant
 * * %n - where n is a number, represents the nth metamorphic input
 * * %i - represents a random input, guaranteed to be the same across metamorphic
 * variants at the same call site
 * * other identifiers are permitted after a % symbol, as long as they are
 * declared as generators prior
 * An object might be created when concretizing certain MetaVarObjects
 *
 * @param mv_obj The MetaVarObject to be concretized
 *
 * @return A reference to the corresponding ApiObject
 */

//const ApiObject*
//ApiFuzzerNew::concretizeMetaVarObject(const MetaVarObject* mv_obj)
//{
    //if (mv_obj->meta_relations.empty())
    //{
        //if (!mv_obj->getIdentifier().compare("<m_curr>"))
        //{
            //return this->smt->getCurrentMetaVar();
        //}
        //else if (mv_obj->getIdentifier().front() == 'm')
        //{
            //assert(std::isdigit(mv_obj->getIdentifier()[1]));
            //size_t meta_variant_id = stoi(mv_obj->getIdentifier().substr(1));
            //for (const ApiObject* mv : meta_variants)
            //{
                //if (mv->getID() == meta_variant_id)
                //{
                    //return mv;
                //}
            //}
            //assert(false);
        //}
        //else if (std::isdigit(mv_obj->getIdentifier().front()))
        //{
            //size_t input_id = stoi(mv_obj->getIdentifier()) - 1;
            //CHECK_CONDITION(input_id < this->meta_in_vars.size(),
                //fmt::format("Expected input var id `{}`, but have only `{}`.",
                //input_id, this->meta_in_vars.size()));
            //return this->meta_in_vars.at(input_id);
        //}
        //assert(false);
    //}
    //return this->concretizeFuncObject(mv_obj->meta_relations.at(
        //(*this->rng)() % mv_obj->meta_relations.size())->getBaseFunc());
//}

#if 1
// Code Added by Pritam

bool ApiFuzzerNew::assertReduction(const ApiInstructionInterface* assert, std::vector<const ApiObject*> var)
{
	std::vector<const ApiObject*> params;
	const ApiInstruction* instr;

	instr = dynamic_cast<const ApiInstruction*>(assert);
	
        if (!strcmp(instr->getFunc()->getName().c_str(), "assert"))
	{
		params = (*(instr->getFuncParams().begin()))->getAllObjs();

		std::vector<const ApiObject*>::iterator it;	

		const ApiObject *first, *second;

		it = params.begin();

		first = *(it);

		it++;

		second = (*it);

		if(first == second)
		{
			return false;
		}

		for(it = params.begin(); it != params.end(); it++)
		{
			if(std::find(var.begin(), var.end(), *it) == var.end())
			{
				return false;
			}
		}

		return true;
	}
	
	return false;
}

std::vector<int> ApiFuzzerNew::MHReduceInstr(std::string compile_err, std::string exe_err, std::vector<const ApiObject*> var, std::vector<int> rel_indices, std::string output_file)
{
	std::pair<std::string, std::string> p1;

	std::string nc_err, n_exe_err;

	std::vector<int> new_indices, ip1, ip2, new_indices1, new_indices2;

	int size = rel_indices.size();
	int msize;

	if(size%2 == 0)
		msize = size/2;
	else
		msize = size/2 + 1;

	// Create two vectors each containing ip1 and ip2 half of the original input vector 

	for(int i = 0; i < msize; i++)
	{
		ip1.push_back(rel_indices.at(i));
	}

	ip2.push_back(rel_indices.at(0));

	for(int i = msize; i < size; i++)
	{
		ip2.push_back(rel_indices.at(i));
	}

	// Perform reduction on the first half

	p1 = createTestCaseMHReduce(exe_err, var, ip1, output_file);

	nc_err = p1.first;
	n_exe_err = p1.second;

	if(checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
	{
		if(ip1.size() > 1)
		{
			new_indices1 = MHReduceInstr(compile_err, exe_err, var, ip1, output_file);
		}
		else
		{
			new_indices1 = ip1;
		}

		new_indices = new_indices1;
	}
	else //Back Track
	{
		if(ip2 == rel_indices)
		{
			createTestCaseMHReduce(exe_err, var, ip2, output_file); //New Test case Generated with mvar meta variants
			return ip2;
		}

		new_indices1 = ip1;
	
		p1 = createTestCaseMHReduce(exe_err, var, ip2, output_file); //New Test case Generated with mvar meta variants
	
		nc_err = p1.first;
		n_exe_err = p1.second;

		if(checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
		{
			if(ip2.size() > 1)
			{
				new_indices2 = MHReduceInstr(compile_err, exe_err, var, ip2, output_file);
			}
			else
			{
				new_indices2 = ip2;
			}

			new_indices = new_indices2;
		}
		else
		{
			new_indices2 = ip2;

			new_indices = indexMerge(new_indices1, new_indices2);

			if(new_indices != rel_indices)
			{
				new_indices = MHReduceInstr(compile_err, exe_err, var, new_indices, output_file);
			}
		}
	}

	return new_indices;
}

std::vector<int> ApiFuzzerNew::indexMerge(std::vector<int> v1, std::vector<int> v2)
{
	for(std::vector<int>::iterator it = v2.begin(); it != v2.end(); it++)
	{
		if(find(v1.begin(), v1.end(), *it) == v1.end())
		{
			v1.push_back(*it);
		}
	}

	return v1;
}

std::pair<std::string, std::string> ApiFuzzerNew::createTestCaseMHReduce(std::string exe_err, std::vector<const ApiObject*> var, std::vector<int> rel_indices, std::string output_file)
{
	std::vector<const ApiInstructionInterface*> temp;
	std::stringstream new_ss_m;
	const ApiObject* mvar;
	std::pair<std::string,std::string> res;	

	std::vector<const ApiObject*> assert_vars;
	std::vector<std::string> assert_vars_names;

    	res = parseErrorMsg(exe_err);

	assert_vars_names.push_back(res.first);
	assert_vars_names.push_back(res.second);

	assert_vars = getApiObjects(assert_vars_names);

	for(std::vector<const ApiObject*>::iterator it = var.begin(); it != var.end(); it++)
	{
		mvar = *it;

		temp =  MetaVariant_Instr[mvar->getID()];

		writeLine(new_ss_m, temp.at(0)->toStr());
//		writeLine(new_ss_m, temp.at(1)->toStr());

		for(int i = 0; i < rel_indices.size(); i++)
		{
			writeLine(new_ss_m, temp.at(rel_indices.at(i))->toStr());
		}

		if(assertReduction(temp.at(meta_test_count+1), assert_vars))
		{
			writeLine(new_ss_m, temp.at(meta_test_count+1)->toStr());
		}
	}

	std::ofstream ofs;
	ofs.open(output_file);

	ofs << new_ss_i.str();
	ofs << new_ss_mi.str();
	ofs << new_ss_m.str();
	ofs << new_ss_p.str();
	ofs.close();

	std::string new_compile_err = "";	
	std::string new_exe_err = "";	

    	new_compile_err = Exec(command);
    	new_exe_err = Exec(exe_command);

	res = std::make_pair(new_compile_err, new_exe_err);

	return res;
}

std::vector<const ApiInstructionInterface*> ApiFuzzerNew::MHReduceInstrPrep(std::string compile_err, std::string exe_err, std::vector<const ApiObject*> var, std::string output_file)
{
	const ApiObject* mvar;
        std::vector<const ApiInstructionInterface*> res, temp;
	std::vector<int> indices;

	for(int i = 1; i <= meta_test_count; i++)
	{
		indices.push_back(i);
	}

	indices = MHReduceInstr(compile_err, exe_err, var, indices, output_file);

	std::pair<std::string,std::string> parsed_msg;	

	std::vector<const ApiObject*> assert_vars;
	std::vector<std::string> assert_vars_names;

    	parsed_msg = parseErrorMsg(exe_err);

	assert_vars_names.push_back(parsed_msg.first);
	assert_vars_names.push_back(parsed_msg.second);

	assert_vars = getApiObjects(assert_vars_names);

	
	for(std::vector<const ApiObject*>::iterator it = var.begin(); it != var.end(); it++)
	{
		mvar = *it;
		temp =  MetaVariant_Instr[mvar->getID()];

		res.push_back(temp.at(0));	

		for(int i = 0; i < indices.size(); i++)
		{
			res.push_back(temp.at(indices.at(i)));
		}
		
		if(assertReduction(temp.at(meta_test_count+1), assert_vars))
		{
			res.push_back(temp.at(meta_test_count+1));
		}
	}

	return res;
}


std::vector<const ApiObject*> ApiFuzzerNew::getApiObjects(std::vector<std::string> var)
{
        std::vector<const ApiObject*> all = meta_variants;

	std::vector<const ApiObject*> res;

	for(std::vector<const ApiObject*>::iterator it = all.begin(); it != all.end(); it++)
	{
		if(find(var.begin(), var.end(), (*it)->toStr()) != var.end())
		{
			res.push_back(*it);
		}
	}		

	return res;
}

bool ApiFuzzerNew::checkTestCase(std::string c_err, std::string n_c_err, std::string e_err, std::string n_e_err)
{
	std::pair<std::string,std::string> res1, res2;	
	
	if(c_err == n_c_err)
	{
		res1 = parseErrorMsg(e_err);
		res2 = parseErrorMsg(n_e_err);

		if(res1.first == res2.first && res1.second == res2.second)
		{
			#if 0 
			std::cout << "Error Did Not Change" << std::endl;
			std::cout << "Old Compile error: " << c_err << std::endl;
			std::cout << "New Compile error: " << n_c_err << std::endl;

			std::cout << "Old Execute error: " << e_err << std::endl;
			std::cout << "New Execute error: " << n_e_err << std::endl;
			#endif

			return true;
		}
	}

	#if 0 
	std::cout << "Error Changed" << std::endl;
	std::cout << "Old Compile error: " << c_err << std::endl;
	std::cout << "New Compile error: " << n_c_err << std::endl;

	std::cout << "Old Execute error: " << e_err << std::endl;
	std::cout << "New Execute error: " << n_e_err << std::endl;
	#endif

	return false;
}

std::pair<std::string, std::string> ApiFuzzerNew::createTestCase(std::vector<const ApiObject*> var, std::string output_file)
{
	std::vector<const ApiInstructionInterface*> list_inst;
	std::stringstream new_ss_m;

	list_inst = MetaVariantReduce(var);

	for(std::vector<const ApiInstructionInterface*>::iterator it = list_inst.begin(); it != list_inst.end(); it++)
	{
		writeLine(new_ss_m, (*it)->toStr());
	}

	std::ofstream ofs;
	ofs.open(output_file);

	ofs << new_ss_i.str();
	ofs << new_ss_mi.str();
	ofs << new_ss_m.str();
	ofs << new_ss_p.str();
	ofs.close();

	std::string new_compile_err = "";	
	std::string new_exe_err = "";	
	std::pair<std::string,std::string> res;	

 	new_compile_err = Exec(command);
	new_exe_err = Exec(exe_command);

	res = std::make_pair(new_compile_err, new_exe_err);

	return res;
}

std::vector<const ApiObject*> ApiFuzzerNew::verticalReductionMerge(std::vector<const ApiObject*> mvar1, std::vector<const ApiObject*> mvar2)
{
	for(std::vector<const ApiObject*>::iterator it = mvar2.begin(); it != mvar2.end(); it++)
	{
		if(find(mvar1.begin(), mvar1.end(), *it) == mvar1.end())
		{
			mvar1.push_back(*it);
		}
	}

	return mvar1;
}

std::vector<const ApiInstructionInterface*> ApiFuzzerNew::instructionMerge(std::vector<const ApiInstructionInterface*> mvar1, std::vector<const ApiInstructionInterface*> mvar2)
{
	for(std::vector<const ApiInstructionInterface*>::iterator it = mvar2.begin(); it != mvar2.end(); it++)
	{
		if(find(mvar1.begin(), mvar1.end(), *it) == mvar1.end())
		{
			mvar1.push_back(*it);
		}
	}

	return mvar1;
}

std::vector<const ApiObject*> ApiFuzzerNew::verticalReduction(std::string compile_err, std::string exe_err, std::vector<const ApiObject*> mvar, std::string output_file)
{
	std::vector<const ApiObject*> new_mvar1, new_mvar2, ip1, ip2, new_mvar;

    	logDebug(fmt::format("Inside verticalReduction"));
	printVectorApiObjects(mvar);

	//Find the mid of the input vector mvar

	int size = mvar.size();
	int msize;

	if(size%2 == 0)
		msize = size/2;
	else
		msize = size/2 + 1;

	// Create two vectors each containing ip1 and ip2 half of the original input vector 

	std::pair<std::string, std::string> p1;

	std::string nc_err, n_exe_err;

	for(int i = 0; i < msize; i++)
	{
		ip1.push_back(mvar.at(i));
	}

	ip2.push_back(mvar.at(0)); // Since the comparison often happens with the first meta variant and the vector is sorted

	for(int i = msize; i < size; i++)
	{
		ip2.push_back(mvar.at(i));
	}

	// Perform reduction on the first half

	p1 = createTestCase(ip1, output_file);	//New Test case Generated with mvar meta variants
	nc_err = p1.first;
	n_exe_err = p1.second;

	if(checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
	{
		if(ip1.size() > 2)
		{
			new_mvar1 = verticalReduction(compile_err, exe_err, ip1, output_file);
		}
		else
		{
			new_mvar1 = ip1;
		}

		new_mvar = new_mvar1;
	}
	else //Back Track
	{
		new_mvar1 = ip1;

		p1 = createTestCase(ip2, output_file);	//New Test case Generated with mvar meta variants
		nc_err = p1.first;
		n_exe_err = p1.second;

		if(checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
		{
			if(ip2.size() > 2)
			{
				new_mvar2 = verticalReduction(compile_err, exe_err, ip2, output_file);
			}
			else
			{
				new_mvar2 = ip2;
			}

			new_mvar = new_mvar2;
		}
		else
		{
			new_mvar2 = ip2;

			new_mvar = verticalReductionMerge(new_mvar1, new_mvar2);

			if(new_mvar != mvar)
			{
				logDebug(fmt::format("Merged output is not same as original"));
				printVectorApiObjects(new_mvar);

				new_mvar = verticalReduction(compile_err, exe_err, new_mvar, output_file);
			}
		}
	}

	logDebug(fmt::format("Inside verticalReduction-- Final Output"));
	printVectorApiObjects(new_mvar);

	return new_mvar;
}	

std::vector<const ApiInstructionInterface*> ApiFuzzerNew::MetaVariantReduce(std::vector<const ApiObject*>  var)
{
	int id;
	std::vector<const ApiInstructionInterface*> result, list_inst;

	for(std::vector<const ApiObject*>::iterator it = var.begin(); it != var.end(); it++)
	{
		id = (*it)->getID();

		list_inst = MetaVariant_Instr[id];

		for(std::vector<const ApiInstructionInterface*>::iterator iit = list_inst.begin(); iit != list_inst.end(); iit++)
		{
			result.push_back(*iit);
		}
	}		

	return result;
}

std::vector<const ApiInstructionInterface*> ApiFuzzerNew::fuzzerReduction(std::string compile_err, std::string exe_err, std::string output_file, std::vector<const ApiInstructionInterface*> red)
{
	std::vector<NodeT*> root = this->tree.getRoots();


	std::vector<const ApiInstructionInterface*> input, input_temp;

	for(std::vector<NodeT*>::iterator it = root.begin(); it != root.end(); it++)
	{
		nodeReduction(compile_err, exe_err, *it, output_file, red);
	}	

	return this->tree.traverse();
}

void ApiFuzzerNew::nodeReduction(std::string compile_err, std::string exe_err, NodeT* node, std::string output_file, std::vector<const ApiInstructionInterface*> red)
{
	std::vector<EdgeT*> child;
	EdgeT* edge;

	for(std::vector<EdgeT*>::iterator it = this->tree.edges.begin(); it != this->tree.edges.end(); it++)
	{
		edge = *it;

		if(edge->src == node)
		{
			child.push_back(edge);
		}
	}

	std::vector<ApiInstructionInterface*> inst;

	std::vector<EdgeT*> new_child = childReduction(compile_err, exe_err, node, child, output_file, red);

	std::vector<EdgeT*> rem_child;

	std::set_difference(child.begin(), child.end(), new_child.begin(), new_child.end(), std::inserter(rem_child, rem_child.begin()));

	this->tree.removeChildren(rem_child);

//	return this->tree.traverseSubTree(node);
//	printVectorApiInstructions(this->tree.traverse());
}

std::pair<std::string, std::string> ApiFuzzerNew::createTestCaseEdge(NodeT* node, std::vector<EdgeT*> new_child, std::string output_file, std::vector<const ApiInstructionInterface*> red)
{
//	std::cout << "CreateTestCase for Edges: " << new_child.size() << std::endl;

	DependenceTree new_tree = this->tree;
	
	EdgeT* edge;

	std::vector<EdgeT*> child;

	for(std::vector<EdgeT*>::iterator it = new_tree.edges.begin(); it != new_tree.edges.end(); it++)
	{
		edge = *it;

		if(edge->src == node)
		{
			child.push_back(edge);
		}
	}

	std::vector<EdgeT*> rem_child;

	std::set_difference(child.begin(), child.end(), new_child.begin(), new_child.end(), std::inserter(rem_child, rem_child.begin()));

	new_tree.removeChildren(rem_child);

	std::vector<const ApiInstructionInterface*> inst = new_tree.traverse();

	std::stringstream new_ss_m;

	for(std::vector<const ApiInstructionInterface*>::iterator it = inst.begin(); it != inst.end(); it++)
	{
		writeLine(new_ss_m, (*it)->toStr());
	}

	for(std::vector<const ApiInstructionInterface*>::iterator it = red.begin(); it != red.end(); it++)
	{
		writeLine(new_ss_m, (*it)->toStr());
	}

	std::ofstream ofs;
	ofs.open(output_file);

	ofs << new_ss_i.str();
	ofs << new_ss_m.str();
	ofs << new_ss_p.str();
	ofs.close();

	std::string new_compile_err = "";	
	std::string new_exe_err = "";	
	std::pair<std::string,std::string> res;	

 	new_compile_err = Exec(command);
	new_exe_err = Exec(exe_command);

	res = std::make_pair(new_compile_err, new_exe_err);

	return res;
}

std::vector<EdgeT*> ApiFuzzerNew::edgeMerge(std::vector<EdgeT*> mvar1, std::vector<EdgeT*> mvar2)
{
	for(std::vector<EdgeT*>::iterator it = mvar2.begin(); it != mvar2.end(); it++)
	{
		if(find(mvar1.begin(), mvar1.end(), *it) == mvar1.end())
		{
			mvar1.push_back(*it);
		}
	}

	return mvar1;
}

std::vector<EdgeT*> ApiFuzzerNew::childReduction(std::string compile_err, std::string exe_err, NodeT* node, std::vector<EdgeT*> child, std::string output_file, std::vector<const ApiInstructionInterface*> red)
{
	logDebug(fmt::format("Inside childReduction"));
//	std::cout << "Inside childReduction: " << node->var->toStr() << std::endl;
	printVectorEdges(child);

	std::vector<EdgeT*> new_child1, new_child2, ip1, ip2, new_child;

	//Find the mid of the input vector child

	int size = child.size();
	int msize;

	if(size%2 == 0)
		msize = ceil(size/2);
	else
		msize = ceil(size/2) + 1;

	// Create two vectors each containing ip1 and ip2 half of the original input vector 

	std::pair<std::string, std::string> p1;

	std::string nc_err, n_exe_err;

	for(int i = 0; i < msize; i++)
	{
		ip1.push_back(child.at(i));
	}

	ip2.push_back(child.at(0));

	for(int i = msize; i < size; i++)
	{
		ip2.push_back(child.at(i));
	}

	// Perform reduction on the first half

	p1 = createTestCaseEdge(node, ip1, output_file, red);	//New Test case Generated with mvar meta variants

//	std::cout << "Created test case with the first half" <<std::endl;

	nc_err = p1.first;
	n_exe_err = p1.second;

	if(checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
	{
//		std::cout << "Reduce test case with the first half" <<std::endl;

		if(ip1.size() > 1)
		{
			new_child1 = childReduction(compile_err, exe_err, node, ip1, output_file, red);
		}
		else
		{
			new_child1 = ip1;
		}

		new_child = new_child1;
	}
	else //Back Track
	{
//		std::cout << "Test case failed with the first half" <<std::endl;

		new_child1 = ip1;
	
		if(ip2 == child)
		{
			createTestCaseEdge(node, ip2, output_file, red);
			return child;
		}

		p1 = createTestCaseEdge(node, ip2, output_file, red);	//New Test case Generated with mvar meta variants
	
//		std::cout << "Created test case with the second half" <<std::endl;

		nc_err = p1.first;
		n_exe_err = p1.second;

		if(checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
		{
//			std::cout << "Reduce test case with the second half" <<std::endl;

			if(ip2.size() > 1)
			{
				new_child2 = childReduction(compile_err, exe_err, node, ip2, output_file, red);
			}
			else
			{
				new_child2 = ip2;
			}

			new_child = new_child2;
		}
		else
		{
			new_child2 = ip2;

//			std::cout << "Test case failed with the second half" <<std::endl;

			new_child = edgeMerge(new_child1, new_child2);

//			std::cout << "Merged Output" << std::endl;
			logDebug(fmt::format("Merged Output"));
			printVectorEdges(new_child);

			if(new_child != child)
			{
				new_child = childReduction(compile_err, exe_err, node, new_child, output_file, red);
			}
		}
	}

//	std::cout << "Final Output" << std::endl;
	logDebug(fmt::format("Final Output"));
	printVectorEdges(new_child);

	return new_child;
}

std::vector<const ApiInstructionInterface*> ApiFuzzerNew::reduceSubTree(std::string compile_err, std::string exe_err, std::string output_file, std::vector<const ApiInstructionInterface*> red)
{

//	std::cout << "Special Object: " << special_obj.first->toStr() << " : " << special_obj.second->toStr() << std::endl;

	if(!special_obj.first)
	{
		return this->tree.traverse();
	}

	std::vector<NodeT*> root = this->tree.getRoots();

	std::vector<NodeT*> nodes, temp;
	std::vector<EdgeT*> vec_edges, vec_edges_temp;

//	std::cout << "Inside reduceSubTree" << std::endl;

	for(std::vector<NodeT*>::iterator it = root.begin(); it != root.end(); it++)
	{
//		std::cout << "Root: " << (*it)->var->toStr() << std::endl;

		vec_edges = this->tree.getImmDescendants(*it);

		int max = 0;
		NodeT* node = NULL;

		for(std::vector<EdgeT*>::iterator vit = vec_edges.begin(); vit != vec_edges.end(); vit++)
		{
			temp = (*vit)->dests;	

			for(std::vector<NodeT*>::iterator nit = temp.begin(); nit != temp.end(); nit++)
			{
				if(find(nodes.begin(), nodes.end(), *nit) == nodes.end())
				{
					if(*nit != *it)
					{
						if((*nit)->var->getType()->isPrimitive())
						{
							continue;
						}

						subTreeReduction(compile_err, exe_err, *nit, output_file, red);

						#if 0
						vec_edges_temp = this->tree.getImmDescendants(*nit);

						if(vec_edges_temp.size() > max)
						{
							max = vec_edges.size();
							node = *nit;
						}

						nodes.push_back(*nit);
						#endif
					}
				}
			}
		}

		#if 0
		if(node != NULL)
		{
//			std::cout << "1 Node selected for reduction: " << node->var->toStr() << std::endl;
			subTreeReduction(compile_err, exe_err, node, output_file, red);
		}
		else if(!nodes.empty())
		{
//			std::cout << "2 Node selected for reduction: " << node->var->toStr() << std::endl;
			node = nodes.at(rand()%nodes.size());
			subTreeReduction(compile_err, exe_err, node, output_file, red);
		}
		#endif
	}

	return this->tree.traverse();
}

void ApiFuzzerNew::subTreeReduction(std::string compile_err, std::string exe_err, NodeT* node, std::string output_file, std::vector<const ApiInstructionInterface*> red)
{
	std::vector<NodeT*> temp;
	std::vector<EdgeT*> vec_edges, vec_edges_temp;
	NodeT* new_node;

        const ApiObject* obj;

	DependenceTree new_tree = this->tree;

	#if 0
	std::vector<const ApiObject*> candidate_params = this->filterObjs(&ApiObject::hasType, node->var->getType());

	if(!candidate_params.empty())
	{
                obj = getRandomVectorElem(candidate_params, this->getRNG());
	}
	else
	{
		obj = generateNewObject(node->var->getType());
	}
	#endif

//	obj = generateApiObjectDecl("new_matrix", node->var->getType(), true);
//	obj = getSingletonObject(node->var->getType());

	obj = special_obj.first;

	NodeT* new_node1 = new_tree.insertNode(obj);

	#if 0
	std::vector<NodeT*> dests;
	const ApiInstructionInterface* api_new_instr;
//	api_new_instr = this->instrs.at(this->instrs.size()-1);	
	api_new_instr = special_obj.second;

	new_tree.addEdge(new_node1, dests, api_new_instr);
	#endif

//	std::cout << "Inside subTreeReduction" << std::endl;

	new_tree = replaceSubTree(new_tree, node, new_node1); 

//	std::cout << "Inside subTreeReduction -- after replaceSubTree" << std::endl;

//	printVectorEdges(new_edges);

//	new_tree.traverse();
//	std::cout << "Traverse End" << std::endl;

	std::pair<std::string, std::string> res;

	res = createTestCaseTree(new_tree, output_file, red);

	std::string nc_err = res.first;
	std::string n_exe_err = res.second;

	NodeT* new_node2;

	if(checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
	{
//		std::cout << "Test Case Reduced retains the error" << std::endl;

		this->tree = new_tree;
	}
	else
	{
//		std::cout << "Choose some other node at the next level" << std::endl;

		vec_edges = this->tree.getImmDescendants(node);

		int max = 0;
		NodeT* enode = NULL;
		std::vector<NodeT*> nodes;

		for(std::vector<EdgeT*>::iterator it = vec_edges.begin(); it != vec_edges.end(); it++)
		{
			temp = (*it)->dests;	

			for(std::vector<NodeT*>::iterator nit = temp.begin(); nit != temp.end(); nit++)
			{
				if(find(nodes.begin(), nodes.end(), *nit) == nodes.end())
				{
					if(*nit != node)
					{
						if((*nit)->var->getType()->isPrimitive())
						{
							continue;
						}

						vec_edges_temp = this->tree.getImmDescendants(*nit);

						if(vec_edges_temp.size() > max)
						{
							max = vec_edges.size();
							enode = *nit;
						}

						nodes.push_back(*nit);
					}
				}
			}

		}

		if(enode != NULL)
		{
//			std::cout << "21 Node selected for reduction: " << node->var->toStr() << std::endl;
			subTreeReduction(compile_err, exe_err, enode, output_file, red);
		}
		else if(!nodes.empty())
		{
//			std::cout << "22 Node selected for reduction: " << node->var->toStr() << std::endl;
			enode = nodes.at(rand()%nodes.size());
			subTreeReduction(compile_err, exe_err, enode, output_file, red);
		}
	}
}

std::pair<std::string, std::string> ApiFuzzerNew::createTestCaseTree(DependenceTree tree, std::string output_file, std::vector<const ApiInstructionInterface*> red)
{
	std::vector<const ApiInstructionInterface*> inst;

	std::stringstream new_ss_m;

	inst = tree.traverse();

	for(std::vector<const ApiInstructionInterface*>::iterator it = inst.begin(); it != inst.end(); it++)
	{
		writeLine(new_ss_m, (*it)->toStr());
	}

	for(std::vector<const ApiInstructionInterface*>::iterator it = red.begin(); it != red.end(); it++)
	{
		writeLine(new_ss_m, (*it)->toStr());
	}

	std::ofstream ofs;
	ofs.open(output_file);

	ofs << new_ss_i.str();
	ofs << new_ss_m.str();
	ofs << new_ss_p.str();
	ofs.close();

	std::string new_compile_err = "";	
	std::string new_exe_err = "";	
	std::pair<std::string,std::string> res;	

 	new_compile_err = Exec(command);
	new_exe_err = Exec(exe_command);

	res = std::make_pair(new_compile_err, new_exe_err);

	return res;

}

const ApiInstructionInterface* isPresent(std::vector<const ApiInstructionInterface*> red, const ApiInstructionInterface* instr)
{
	const ApiInstructionInterface* r = NULL;

	for(std::vector<const ApiInstructionInterface*>::iterator it = red.begin(); it != red.end(); it++)
	{
		if((*it)->toStr() == instr->toStr())
		{
			return *it;
		}
	}

	return r;
}

std::vector<const ApiInstructionInterface*> ApiFuzzerNew::simplifyMetaRelationsPrep(std::string compile_err, std::string exe_err, std::vector<const ApiObject*> var, std::string output_file, std::vector<const ApiInstructionInterface*> input_insts, std::vector<const ApiInstructionInterface*> red)
{
//	std::cout << "Inside simplifyMetaRelations: " << mvar_relations.size() << std::endl;

	const ApiInstructionInterface* new_instr;
	const ApiInstructionInterface* instr1;
	const ApiInstructionInterface* instr2;

	std::vector<const MetaRelation*> rel1, rel2;
	const ApiObject* mvar;

	std::map<const ApiInstructionInterface*, const ApiInstructionInterface*> map_relations;
	
	for(std::map<const ApiObject*, original_simplified_mapping>::reverse_iterator it = mvar_relations.rbegin(); it != mvar_relations.rend(); it++)
	{
		mvar = it->first;

		if(find(var.begin(), var.end(), mvar) == var.end())
		{
			continue;
		}

//		std::cout << "Mvar: " << mvar->toStr() << std::endl;
	
		rel1 = it->second.first;
		rel2 = it->second.second;

//		std::cout << "Rel1 Size: " << rel1.size() << std::endl;
//		std::cout << "Rel2 Size: " << rel2.size() << std::endl;

		for(int i = rel1.size()-1; i >= 0; i--)
		{
//			std::cout << "i: " << i << std::endl;

//			std::cout << "Rel1: " << rel1.at(i)->toStr() << std::endl;
	
			if(rel1.at(i)->getAbstractRelation() == "check")
			{
				instr1 = rel1.at(i)->toApiInstruction();

				map_relations[instr1] = instr1;

				continue;
			}	

			mvar->setDeclared();
			instr1 = rel1.at(i)->toApiInstruction(); // Without Declaration
			mvar->resetDeclared();
			instr2 = rel1.at(i)->toApiInstruction();
		
			if(isPresent(red, instr1) != NULL)
			{
				mvar->setDeclared();

				new_instr = rel2.at(i)->toApiInstruction();
				map_relations[instr1] = new_instr;
			}	
			else if(isPresent(red, instr2) != NULL)
			{
				mvar->resetDeclared();

				new_instr = rel2.at(i)->toApiInstruction();
				map_relations[instr2] = new_instr;
			}	
		}

//		std::cout << "Outside Inner for loop" << std::endl;
	}

//	std::cout << "Outside Outer for loop" << std::endl;

	std::vector<const ApiInstructionInterface*> res;

//	printVectorApiInstructions(red);

	try_outs = 0;

	std::vector<const ApiInstructionInterface*> simp_red;

	for(int i = 0; i < red.size(); i++)
	{
		if(i%2 == 0)
		{
			simp_red.push_back(red.at(i));
		}
	}

	res = simplifyMetaRelations(compile_err, exe_err, output_file, input_insts, red, simp_red, map_relations);
	
 	std::string new_compile_err = Exec(command);
	std::string new_exe_err = Exec(exe_command);

	if(checkTestCase(compile_err, new_compile_err, exe_err, new_exe_err))
	{
		return res;
	}
	else
	{
		createTestCaseSimplify(input_insts, red, red, map_relations, output_file);
		return res;
	}
}

std::pair<std::string, std::string> ApiFuzzerNew::createTestCaseSimplify(std::vector<const ApiInstructionInterface*> input_insts, std::vector<const ApiInstructionInterface*> red, std::vector<const ApiInstructionInterface*> var, std::map<const ApiInstructionInterface*, const ApiInstructionInterface*> map_relations, std::string output_file)
{
	std::vector<const ApiInstructionInterface*> inst;

	std::stringstream new_ss_m;

	for(std::vector<const ApiInstructionInterface*>::iterator it = input_insts.begin(); it != input_insts.end(); it++)
	{
		writeLine(new_ss_m, (*it)->toStr());
	}

//	std::cout << "Create test case" << std::endl;
//	printVectorApiInstructions(var);

	for(std::vector<const ApiInstructionInterface*>::iterator it = red.begin(); it != red.end(); it++)
	{
//		inst.push_back(map_relations[*it]);

//		std::cout << "Instr in red: " << (*it)->toStr() << std::endl;

		if(isPresent(var, *it) != NULL)	
		{
			if(map_relations.find(*it) != map_relations.end())	
			{
//				std::cout << "Instr Found: " << std::endl;
//				std::cout << "New Instr: " << map_relations[*it]->toStr() << std::endl;
				writeLine(new_ss_m, map_relations[*it]->toStr());
			}
			else
			{
				writeLine(new_ss_m, (*it)->toStr());
			}
		}
		else
		{
//			std::cout << "Instr Not Found: " << std::endl;
			writeLine(new_ss_m, (*it)->toStr());
		}
	}	

	std::ofstream ofs;
	ofs.open(output_file);

	ofs << new_ss_i.str();
	ofs << new_ss_m.str();
	ofs << new_ss_p.str();
	ofs.close();

	std::string new_compile_err = "";	
	std::string new_exe_err = "";	
	std::pair<std::string,std::string> res;	

 	new_compile_err = Exec(command);
	new_exe_err = Exec(exe_command);

	res = std::make_pair(new_compile_err, new_exe_err);

	return res;
}

std::vector<const ApiInstructionInterface*> ApiFuzzerNew::simplifyMetaRelations(std::string compile_err, std::string exe_err, std::string output_file, std::vector<const ApiInstructionInterface*> input_insts, std::vector<const ApiInstructionInterface*> red, std::vector<const ApiInstructionInterface*> var, std::map<const ApiInstructionInterface*, const ApiInstructionInterface*> map_relations)
{
	if(try_outs > 10)
	{
		map_relations.clear();
		createTestCaseSimplify(input_insts, red, red, map_relations, output_file);
		return red;
	}

	try_outs++;

	std::pair<std::string, std::string> p1;

	std::string nc_err, n_exe_err;

	p1 = createTestCaseSimplify(input_insts, red, var, map_relations, output_file);

	nc_err = p1.first;
	n_exe_err = p1.second;

	if(checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
	{
		return red;
	}

	std::vector<const ApiInstructionInterface*> new_red, ip1, ip2, new_red1, new_red2;

	int size = red.size();
	int msize;

	if(size%2 == 0)
		msize = ceil(size/2);
	else
		msize = ceil(size/2) + 1;

	// Create two vectors each containing ip1 and ip2 half of the original input vector 

	for(int i = 0; i < msize; i++)
	{
		ip1.push_back(red.at(i));
	}

//	ip2.push_back(child.at(0));

	for(int i = msize; i < size; i++)
	{
		ip2.push_back(red.at(i));
	}

	// Perform reduction on the first half

	p1 = createTestCaseSimplify(input_insts, red, ip1, map_relations, output_file);

//	std::cout << "Created test case with the first half" <<std::endl;

	nc_err = p1.first;
	n_exe_err = p1.second;

	if(!checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
	{
//		std::cout << "Reduce test case with the first half" <<std::endl;

		if(ip1.size() > 1)
		{
			new_red1 = simplifyMetaRelations(compile_err, exe_err, output_file, input_insts, red, ip1, map_relations);
		}
		else
		{
			new_red1 = ip1;
		}

		new_red = new_red1;
	}
	else //Back Track
	{
//		std::cout << "Test case failed with the first half" <<std::endl;

		new_red1 = ip1;
	
		p1 = createTestCaseSimplify(input_insts, red, ip2, map_relations, output_file);	//New Test case Generated with mvar meta variants
	
//		std::cout << "Created test case with the second half" <<std::endl;

		nc_err = p1.first;
		n_exe_err = p1.second;

		if(!checkTestCase(compile_err, nc_err, exe_err, n_exe_err)) //Error Replicated, perform more reduction
		{
//			std::cout << "Reduce test case with the second half" <<std::endl;

			if(ip2.size() > 1)
			{
				new_red2 = simplifyMetaRelations(compile_err, exe_err, output_file, input_insts, red, ip2, map_relations);
			}
			else
			{
				new_red2 = ip2;
			}

			new_red = new_red2;
		}
		else
		{
			new_red2 = ip2;

//			std::cout << "Test case failed with the second half" <<std::endl;

			new_red = instructionMerge(new_red1, new_red2);

			if(new_red != red)
			{
				new_red = simplifyMetaRelations(compile_err, exe_err, output_file, input_insts, red, new_red, map_relations);
			}
		}
	}

	return new_red;
}

const ApiInstructionInterface* ApiFuzzerNew::getNewInstruction(const ApiInstruction* old_instr, NodeT* node, NodeT* new_node)
{
	const ApiObject* target;
	const ApiObject* obj;
	const ApiInstruction* new_instr;
	std::vector<const ApiObject*> params, new_params;

//	std::cout << "Hey Der" << std::endl;

	target = old_instr->getTargetObj();

	params = old_instr->getFuncParams();

//	printVectorApiObjects(params);

	if(target != NULL && target == node->var)
	{
//		std::cout << "Node is Target" << target->toStr() << std::endl;		
		target  = new_node->var;
		new_params = params;
	}
	else
	{
//		std::cout << "Node is Param: " << params.size() << std::endl;		
		for(std::vector<const ApiObject*>::iterator it = params.begin(); it != params.end(); it++)
		{
//			std::cout << "Par: " << (*it)->toStr() << std::endl;

			if(*it == node->var)
			{
//				std::cout << "Match" << std::endl;
//				std::cout << "New Node: "  << new_node->var->toStr() << std::endl;
				new_params.push_back(new_node->var);
			}
			else
			{
				new_params.push_back(*it);
			}
		}
	}

//	printVectorApiObjects(new_params);

	obj = old_instr->getResultObj();

//	std::cout << "Result: " << obj->toStr() << std::endl;

	const ApiInstruction* e_instr;
	e_instr = dynamic_cast<const ApiInstruction*>(old_instr);

        if(e_instr->isDeclInstr())
	{
		obj->resetDeclared();
	}

	new_instr = new ApiInstruction(old_instr->getFunc(), obj, target, new_params);

	return ((const ApiInstructionInterface*)(new_instr));
}

EdgeT* ApiFuzzerNew::getNewEdge(EdgeT* old_edge, NodeT* node, NodeT* new_node)
{
	const ApiInstruction* old_instr;
	const ApiInstructionInterface* new_instr;

	#if 0
	std::cout << "Inside New Edge" << std::endl;
	std::cout << "Node: " << node->var->toStr() << std::endl;
	std::cout << "New Node: " << new_node->var->toStr() << std::endl;
	#endif

	std::vector<NodeT*> new_dests, old_dests;
	NodeT* target_node;

	EdgeT* res = new EdgeT();

	res->src = old_edge->src;

	old_dests = old_edge->dests;

	for(std::vector<NodeT*>::iterator it = old_dests.begin(); it != old_dests.end(); it++)
	{
		if(*it == node)
		{
//			std::cout << "Match Found" << std::endl;
			new_dests.push_back(new_node);			
		}
		else
		{
			new_dests.push_back(*it);
		}
	}

	printVectorNodes(new_dests);

	res->dests = new_dests;

	old_instr = dynamic_cast<const ApiInstruction*>(old_edge->instr);

	new_instr = getNewInstruction(old_instr, node, new_node);
	
	res->instr = (const ApiInstructionInterface*)(new_instr);

	return res;
}

DependenceTree ApiFuzzerNew::replaceSubTree(DependenceTree tree, NodeT* node, NodeT* new_node)
{
//	std::cout << "Inside replaceSubTree" << std::endl;
//	std::cout <<  "New Node: " << new_node->var->toStr() << std::endl; 

	DependenceTree new_tree = tree;

	std::vector<EdgeT*> ancestors, descendants;
	std::vector<EdgeT*> res;

	ancestors = new_tree.getImmAncestors(node);
	descendants = new_tree.getImmDescendants(node);

	EdgeT* old_edge;
	EdgeT* new_edge;

	std::vector<EdgeT*> edges = new_tree.edges;

	for(std::vector<EdgeT*>::iterator it = edges.begin(); it != edges.end(); it++)
	{
		if(find(ancestors.begin(), ancestors.end(), *it) != ancestors.end())
		{
			old_edge = *it;

//			std::cout << "Ancestor Edge" << std::endl;

			new_edge = getNewEdge(old_edge, node, new_node);

			#if 0
			std::cout << "Old Edge" << std::endl;
			printEdge(old_edge);
			std::cout << "New Edge" << std::endl;
			printEdge(new_edge);
			#endif

			res.push_back(new_edge);
		}	
		else if(find(descendants.begin(), descendants.end(), *it) != descendants.end())
		{
//			std::cout << "Descendant Edge" << std::endl;
//			printEdge(*it);
		}
		else
		{
//			std::cout << "No Edge" << std::endl;

			res.push_back(*it);
		}
	}

//	std::cout << "Modified Edges" << std::endl;
//	printVectorEdges(res);

	new_tree.edges = res;

	return new_tree;
}

#endif
