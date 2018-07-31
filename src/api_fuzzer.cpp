#include "api_fuzzer.hpp"

/* TODO list:
 * - add a depth for expression generation
 */

std::map<std::string, PrimitiveTypeEnum> primitives_map = {
    { "string", STRING },
    { "unsigned int", UINT },
};

char delim_front = '<';
char delim_back = '>';
char delim_mid = '=';

//static const bool DEBUG = false;
static const bool DEBUG = true;

/*******************************************************************************
 * Helper functions
 ******************************************************************************/

void
logDebug(std::string message)
{
    if (DEBUG)
    {
        std::cerr << "DEBUG: " << message << std::endl;
    }
}

std::string
getStringWithDelims(std::vector<std::string> string_list, char delim)
{
    if (string_list.begin() == string_list.end())
    {
        return "";
    }
    std::string string_with_delim = "";
    std::vector<std::string>::iterator it = string_list.begin();
    for (int i = 1; i < string_list.size(); i++)
    {
        string_with_delim += *it + delim + " ";
        it++;
    }
    string_with_delim += *it;
    return string_with_delim;
}

template<typename T>
std::string
makeArgString(std::vector<T> func_args)
{
    std::vector<std::string> args_to_string;
    for (T& obj : func_args)
    {
        args_to_string.push_back(obj->toStr());
    }
    return getStringWithDelims(args_to_string, ',');
}

template<typename T>
T
getRandomVectorElem(std::vector<T>& vector_in)
{
    return vector_in.at(std::rand() % vector_in.size());
}

template<typename T>
T
getRandomSetElem(std::set<T>& set_in)
{
    typename std::set<T>::iterator it = set_in.begin();
    std::advance(it, std::rand() % set_in.size());
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
 * ApiType functions
 ******************************************************************************/

bool
ApiType::isType(const ApiType* other) const
{
    if (other->isExplicit())
    {
        other = dynamic_cast<const ExplicitType*>(other)->getUnderlyingType();
    }
    return !this->toStr().compare(other->toStr());
}

/*******************************************************************************
 * ExplicitFunc functions
 ******************************************************************************/

const ApiObject*
ExplicitType::retrieveObj() const
{
    assert(this->definition.front() == delim_front &&
        this->definition.back() == delim_back);
    logDebug("Definition to retrieve: " + this->getDefinition());
    if  (this->definition.find(fmt::format("string{}", delim_mid))
            != std::string::npos)
    {
        assert(this->getUnderlyingType()->isPrimitive());
        assert(((PrimitiveType *) this->getUnderlyingType())->getTypeEnum()
            == STRING);
        std::string data = this->definition.substr(this->definition.find(delim_mid) + 1,
            this->definition.find(delim_back) - this->definition.find(delim_mid) - 1);
        return new PrimitiveObject<std::string>(
            (PrimitiveType*) this->getUnderlyingType(), data);
    }
    else if (this->definition.find(fmt::format("input{}", delim_mid))
            != std::string::npos)
    {
        // TODO
        assert(false);
    }
    assert(false);
}

std::string
ExplicitType::extractExplicitTypeDecl(std::string type_str)
{
    assert(type_str.front() == delim_front && type_str.back() == delim_back);
    assert(type_str.find(delim_mid) != std::string::npos);
    unsigned int substr_start = type_str.find(delim_mid) + 1;
    unsigned int substr_length = type_str.find(delim_back) - type_str.find(delim_mid) - 1;
    return type_str.substr(substr_start, substr_length);
}

/*******************************************************************************
 * ApiFunc functions
 ******************************************************************************/

const ApiType*
ApiFunc::getParamType(const unsigned int index) const
{
    assert(index < this->getParamCount());
    return this->param_types.at(index);
}

bool
ApiFunc::hasParamTypes(std::vector<const ApiType*> param_types_check) const
{
    if (param_types_check.size() != this->getParamCount())
    {
        return false;
    }
    for (int i = 0; i < param_types_check.size(); i++)
    {
        if (!param_types_check.at(i)->isType(this->getParamType(i)))
        {
            return false;
        }
    }
    return true;
}

bool
ApiFunc::checkArgs(std::vector<const ApiObject*> args_check) const
{
    if (args_check.size() != this->getParamCount())
    {
        return false;
    }
    for (int i = 0; i < args_check.size(); i++)
    {
        if (!args_check.at(i)->getType()->isType(this->getParamType(i)))
        {
            return false;
        }
    }
    return true;
}

std::string
ApiFunc::printSignature() const
{
    std::stringstream print_ss;
    if (this->getMemberType()->toStr() != "")
    {
        print_ss << this->getMemberType()->toStr() << ".";
    }
    print_ss << this->getName() << "(";
    print_ss << makeArgString(this->getParamTypes()) << ")";
    return print_ss.str();
}

std::string
ApiFunc::printInvocation(std::vector<const ApiObject*> params) const
{
    assert(params.size() == this->getParamCount());
    std::stringstream print_inv_ss;
    print_inv_ss << this->getName() << "(";
    print_inv_ss << makeArgString(params) << ")";
    return print_inv_ss.str();
}

/*******************************************************************************
 * ApiFuzzer functions
 ******************************************************************************/

std::vector<std::string>
ApiFuzzer::getInstrList()
{
    return this->instrs;
}

std::vector<const ApiObject*>
ApiFuzzer::getObjList()
{
    return this->objs;
}

std::set<const ApiType*>
ApiFuzzer::getTypeList()
{
    return this->types;
}

std::set<const ApiFunc*>
ApiFuzzer::getFuncList()
{
    return this->funcs;
}

int
ApiFuzzer::getRandInt(int min, int max)
{
    std::uniform_int_distribution<int> uid(min, max);
    return uid(this->rng);
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
ApiFuzzer::addInstr(std::string instr)
{
    //static int counter = 0;
    this->instrs.push_back(instr);
    //this->instrs.push_back(fmt::format("fprintf(stderr, \"{}\n\");\n", ++counter));
}

void
ApiFuzzer::addObj(const ApiObject* obj)
{
    this->objs.push_back(obj);
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

const ApiType*
ApiFuzzer::getTypeByName(std::string type_check)
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
ApiFuzzer::filterObjs(bool (ApiObject::*filter_func)(T) const, T filter_check)
{
    return filterObjList(this->getObjList(), filter_func, filter_check);
}

template<typename T>
std::set<const ApiFunc*>
ApiFuzzer::filterFuncs(bool (ApiFunc::*filter_func)(T) const, T filter_check)
{
    return filterFuncList(this->getFuncList(), filter_func, filter_check);
}

const ApiFunc*
ApiFuzzer::getFuncByName(std::string name)
{
    std::set<const ApiFunc*> filtered_funcs = filterFuncs(&ApiFunc::hasName, name);
    assert(filtered_funcs.size() > 0);
    return getRandomSetElem(filtered_funcs);
}

unsigned int
ApiFuzzer::getNextID()
{
    this->next_obj_id++;
    return this->next_obj_id - 1;
}

const ApiObject*
ApiFuzzer::generateNamedObjectWithoutDecl(std::string name,
    const ApiType* type)
{
    const NamedObject* new_obj = new NamedObject(name, type);
    std::stringstream obj_init_ss;
    obj_init_ss << new_obj->toStrWithType() << ";";
    this->addObj(new_obj);
    this->addInstr(obj_init_ss.str());
    return new_obj;
}

const ApiObject*
ApiFuzzer::generateNamedObject(std::string name, const ApiType* type,
    const ApiFunc* init_func, const ApiObject* target_obj,
    std::vector<const ApiObject*> init_func_args)
{
    const NamedObject* new_obj = new NamedObject(name, type);
    this->applyFunc(init_func, target_obj, new_obj, init_func_args, true);
    this->addObj(new_obj);
    //std::stringstream obj_init_ss;
    //obj_init_ss << new_obj->toStrWithType() << " = ";
    //obj_init_ss << init_func->printInvocation(init_func_args, target_obj) << ";";
    //this->addObj(new_obj);
    //this->addInstr(obj_init_ss.str());
    return new_obj;
}

const ApiObject*
ApiFuzzer::generateApiObjectWithoutDecl(std::string name,
    const ApiType* type)
{
    const ApiObject* new_obj = new ApiObject(name, this->getNextID(), type);
    std::stringstream obj_init_ss;
    obj_init_ss << new_obj->toStrWithType() << ";";
    this->addObj(new_obj);
    this->addInstr(obj_init_ss.str());
    return new_obj;
}

const ApiObject*
ApiFuzzer::generateApiObject(std::string name, const ApiType* type,
    const ApiFunc* init_func, const ApiObject* target_obj,
    std::vector<const ApiObject*> init_func_args)
{
    const ApiObject* new_obj = new ApiObject(name, this->getNextID(), type);
    this->applyFunc(init_func, target_obj, new_obj, init_func_args, true);
    //std::stringstream obj_init_ss;
    //obj_init_ss << new_obj->toStrWithType() << " = ";
    //obj_init_ss << init_func->printInvocation(init_func_args, target_obj) << ";";
    this->addObj(new_obj);
    //this->addInstr(obj_init_ss.str());
    return new_obj;
}

void
ApiFuzzer::applyFunc(const ApiFunc* func, const ApiObject* target_obj,
    const ApiObject* result_obj, std::vector<const ApiObject*> func_args,
    bool is_ctor)
{
    std::stringstream apply_func_ss;
    if (!func->checkArgs(func_args))
    {
        std::cout << "Invalid arguments given for func " << func->getName();
        std::cout << std::endl << "\tExpected types: " << makeArgString(func->getParamTypes());
        std::vector<std::string> arg_strings;
        for (const ApiObject* arg : func_args)
        {
            arg_strings.push_back(arg->toStrWithType());
        }
        std::cout << std::endl << "\tGiven types: " << getStringWithDelims(arg_strings, ',');
        std::cout << std::endl;
        exit(1);
    }
    if (result_obj != nullptr)
    {
        if (is_ctor)
        {
            apply_func_ss << result_obj->toStrWithType();
        }
        else
        {
            apply_func_ss << result_obj->toStr();
        }
        apply_func_ss << " = ";
    }
    else
    {
        assert(!is_ctor);
    }
    if (!func->getConditions().empty())
    {
        apply_func_ss << this->emitFuncCond(func, target_obj, func_args);
    }
    if (!func->isStatic() && target_obj != nullptr)
    {
        apply_func_ss << target_obj->toStr() << ".";
    }
    else if (func->isStatic())
    {
        assert(target_obj == nullptr);
        apply_func_ss << func->getMemberType()->toStr() << "::";
    }
    apply_func_ss << func->printInvocation(func_args);
    if (!func->getConditions().empty())
    {
        apply_func_ss << " : " << this->generateObject(func->getReturnType())->toStr();
    }
    apply_func_ss << ";";
    this->addInstr(apply_func_ss.str());
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

void
ApiFuzzer::applyFunc(const ApiFunc* func, const ApiObject* target_obj,
    const ApiObject* result_obj, bool is_ctor)
{
    std::vector<const ApiObject*> func_args = getFuncArgs(func);
    applyFunc(func, target_obj, result_obj, func_args, is_ctor);
}

std::vector<const ApiObject*>
ApiFuzzer::getFuncArgs(const ApiFunc* func)
{
    logDebug(fmt::format("Curr depth is {}", this->depth));
    std::vector<const ApiType*> param_types = func->getParamTypes();
    std::vector<const ApiObject*> params;
    for (const ApiType* param_type : param_types)
    {
        if (this->getRandInt(0, 10) < this->depth || this->depth > this->max_depth)
        {
            std::vector<const ApiObject*> candidate_params =
                this->filterObjs(&ApiObject::hasType, param_type);
            if (!candidate_params.empty())
            {
                params.push_back(getRandomVectorElem(candidate_params));
                continue;
            }
        }
        this->depth++;
        params.push_back(this->generateObject(param_type));
        this->depth--;
    }
    return params;
}

/*******************************************************************************
 * ApiFuzzerNew functions
 ******************************************************************************/

ApiFuzzerNew::ApiFuzzerNew(std::string& config_file_path, std::mt19937 _rng) : ApiFuzzer(_rng)
{
    YAML::Node config_file = YAML::LoadFile(config_file_path);
    this->initPrimitiveTypes();
    this->initInputs(config_file["inputs"]);
    for (std::pair<std::string, const ApiObject*> pair_in : this->fuzzer_input)
    {
        const PrimitiveObject<unsigned int>* po =
            dynamic_cast<const PrimitiveObject<unsigned int>*>(pair_in.second);
        logDebug(fmt::format("{} = {}", pair_in.first, po->toStr()));
    }
    this->initTypes(config_file["types"]);
    this->initTypes(config_file["singleton_types"]);
    this->initFuncs(config_file["funcs"]);
    this->initFuncs(config_file["special_funcs"]);
    this->initConstructors(config_file["constructors"]);
    this->initGenConfig(config_file["set_gen"]);
    this->generateSet();
    for (std::string inst : this->getInstrList())
    {
        std::cout << inst << std::endl;
    }
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
    std::string member_str = func_yaml["member_type"].as<std::string>();
    const ApiType* member_type =
        member_str == "" ? nullptr :
        this->parseTypeStr(member_str);
    std::string return_str = func_yaml["return_type"].as<std::string>();
    const ApiType* return_type =
        return_str == "" ? nullptr :
        this->parseTypeStr(return_str);
    YAML::Node param_types_list_yaml = func_yaml["param_types"];
    std::vector<const ApiType*> param_type_list;
    for (YAML::Node param_type_yaml : param_types_list_yaml)
    {
        param_type_list.push_back(
            this->parseTypeStr(param_type_yaml.as<std::string>()));
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
        const ApiType* ctor_type = this->parseTypeStr(func_name);
        const ApiType* member_type = nullptr;
        const ApiType* return_type = ctor_type;
        YAML::Node param_types_list_yaml = ctor_yaml["param_types"];
        std::vector<const ApiType *> param_types_list;
        for (YAML::Node param_types_yaml : param_types_list_yaml)
            param_types_list.push_back(
                this->parseTypeStr(param_types_yaml.as<std::string>()));
        std::vector<std::string> cond_list;
        this->addFunc(new ApiFunc(func_name, member_type, return_type,
            param_types_list, cond_list));
    }
}

const ApiType*
ApiFuzzerNew::parseTypeStr(std::string type_str)
{
    if (type_str.front() == delim_front && type_str.back() == delim_back) {
        assert (type_str.find(delim_mid) != std::string::npos);
        // HACK: replace by type of input
        if (type_str.find(fmt::format("input{}", delim_mid)) != std::string::npos)
        {
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
        if (expl_type->isInput())
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
    assert(this->fuzzer_input.count(input_name) != 0);
    return this->fuzzer_input[input_name];
}


const ApiObject*
ApiFuzzerNew::generateNewObject(const ApiType* obj_type)
{
    if (obj_type->isPrimitive())
    {
        return generatePrimitiveObject(dynamic_cast<const PrimitiveType*> (obj_type));
    }
    std::set<const ApiFunc*> ctor_func_candidates = this->filterFuncs(
        &ApiFunc::hasReturnType, obj_type);
    ctor_func_candidates = filterFuncList(ctor_func_candidates,
        &ApiFunc::notIsSpecial);
    const ApiFunc* gen_func = getRandomSetElem(ctor_func_candidates);
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
            target_obj = getRandomVectorElem(target_obj_candidates);
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
        //var_name = o
        //var_name = "v";
    }
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
    std::vector<const ApiObject*> filtered_objs = this->filterObjs(
        &ApiObject::hasType, obj_type);
    assert (filtered_objs.size() <= 1);
    if (filtered_objs.size() == 0)
        return generateNewObject(obj_type);
    return filtered_objs.at(0);
}

const ApiObject*
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
        else
        {
            assert(false);
        }
    }
    return nullptr;
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
ApiFuzzerNew::generateConstructor(YAML::Node instr_config)
{
}

void
ApiFuzzerNew::generateFunc(YAML::Node instr_config, int loop_counter)
{
    std::string func_name = instr_config["func"].as<std::string>();
    const ApiFunc* func = this->getFuncByName(func_name);
    // Set target object, if any declared
    const ApiObject* target_obj = nullptr;
    if (instr_config["target"].IsDefined())
    {
        // TODO consider more possibilities
        assert(instr_config["target"].as<std::string>().find(
            fmt::format("var{}", delim_mid)));
        std::string obj_name = this->getGeneratorData(
            instr_config["target"].as<std::string>());
        std::vector<const ApiObject*> candidate_objs =
            this->filterObjs(&ApiObject::hasName, obj_name);
        assert(candidate_objs.size() == 1);
        target_obj = candidate_objs.at(0);
        assert(target_obj->getType()->isType(func->getMemberType()));
    }
    else if (func->getMemberType() && !func->isStatic())
    {
        target_obj = this->generateObject(func->getMemberType());
    }
    // Set return object, if any declared
    const ApiObject* return_obj = nullptr;
    // TODO should we automatically create a return object if not defined?
    if (instr_config["return"].IsDefined())
    {
        std::string obj_name = instr_config["return"].as<std::string>();
        if (obj_name.find(fmt::format("new{}", delim_mid)) !=
            std::string::npos)
        {
            const ApiType* return_type = this->parseTypeStr(obj_name);
            return_obj = this->generateApiObjectWithoutDecl("v", return_type);
        }
        else if (obj_name.find(fmt::format("var{}", delim_mid)) !=
            std::string::npos)
        {
            std::vector<const ApiObject*> candidate_objs =
                this->filterObjs(&ApiObject::hasName,
                this->getGeneratorData(instr_config["return"].as<std::string>()));
            assert(candidate_objs.size() <= 1);
            if (candidate_objs.size() == 1)
            {
                return_obj = candidate_objs.at(0);
            }
            else
            {
                // TODO check for singleton object? Pipe this through generateObject
                return_obj = this->generateNamedObjectWithoutDecl(
                    this->getGeneratorData(instr_config["return"].as<std::string>()),
                    func->getReturnType());
            }
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
                //func_params.push_back(
                    //std::unique_ptr<const PrimitiveObject<unsigned int>>(
                        //new PrimitiveObject<unsigned int>(
                        //dynamic_cast<const PrimitiveType*>(
                        //this->getTypeByName("unsigned int")), i)));
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
    this->applyFunc(this->getFuncByName(func_name), target_obj, return_obj,
        func_params);
}

std::string
ApiFuzzerNew::getGeneratorData(std::string gen_desc) const
{
    assert(gen_desc.front() == delim_front);
    assert(gen_desc.back() == delim_back);
    assert(gen_desc.find(delim_mid) != std::string::npos);
    return gen_desc.substr(gen_desc.find(delim_mid) + 1,
        gen_desc.find(delim_back) - gen_desc.find(delim_mid) - 1);
}

std::string expr_ops[5] = {"==", "<=", "=>", "<", ">"};

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

/*******************************************************************************
 * ApiFuzzerISL functions
 ******************************************************************************/

//std::vector<const ApiType*> isl_types = {
    //new ApiType("isl::val"),
    //new ApiType("isl::pw_aff"),
    //new ApiType("isl::set"),
    //new ApiType("isl::space"),
    //new ApiType("isl::local_space"),
    //new ApiType("isl::ctx"),
    //new ApiType("void"),
//};

//struct {
    //std::string func_name;
    //std::string member_type_name;
    //std::string return_type_name;
    //std::initializer_list<std::string> param_type_name;
    //std::initializer_list<std::string> conditions;
//} isl_funcs[] = {

////std::vector<ApiFunc> isl_funcs = {
    //// isl::val unary funcs
    //{"two_exp", "isl::val", "isl::val", {}, {}},
    //{"abs"    , "isl::val", "isl::val", {}, {}},
    //{"ceil"   , "isl::val", "isl::val", {}, {}},
    //{"floor"  , "isl::val", "isl::val", {}, {}},
    //{"inv"    , "isl::val", "isl::val", {}, {}},
    //{"neg"    , "isl::val", "isl::val", {}, {}},
    //{"trunc"  , "isl::val", "isl::val", {}, {}},
    //// isl::val binary funcs
    ////ApiFunc{"gcd", "isl::val", {"isl::val"}, {}},
    ////ApiFunc{"mod", "isl::val", {"isl::val"}, {}},
    //{"add"    , "isl::val", "isl::val", {"isl::val"}, {}},
    //{"div"    , "isl::val", "isl::val", {"isl::val"}, {}},
    //{"max"    , "isl::val", "isl::val", {"isl::val"}, {}},
    //{"min"    , "isl::val", "isl::val", {"isl::val"}, {}},
    //{"mul"    , "isl::val", "isl::val", {"isl::val"}, {}},
    //{"sub"    , "isl::val", "isl::val", {"isl::val"}, {}},
    //// isl::pw_aff unary funcs
    //{"ceil"   , "isl::pw_aff", "isl::pw_aff", {}, {}},
    //{"floor"  , "isl::pw_aff", "isl::pw_aff", {}, {}},
    //// isl::pw_aff binary funcs
    ////ApiFunc{"mod", "isl::pw_aff", {"isl::val"}, {}},
    ////ApiFunc{"scale", "isl::pw_aff", {"isl::val"}, {}},
    //{"add"    , "isl::pw_aff"  , "isl::pw_aff", {"isl::pw_aff"}, {}},
    //{"sub"    , "isl::pw_aff"  , "isl::pw_aff", {"isl::pw_aff"}, {}},
    //{"max"    , "isl::pw_aff"  , "isl::pw_aff", {"isl::pw_aff"}, {}},
    //{"min"    , "isl::pw_aff"  , "isl::pw_aff", {"isl::pw_aff"}, {}},
    //// isl::set generation funcs from isl::pw_aff
    //// Other useful funcs
    //{"dump"     , "isl::set"   , "void", {}, {}},
    //{"intersect", "isl::set"   , "isl::set", {"isl::set"}, {}},
    //{"le_set"   , "isl::pw_aff", "isl::set", {"isl::pw_aff"}, {}},
    //{"ge_set"   , "isl::pw_aff", "isl::set", {"isl::pw_aff"}, {}},
    //{"lt_set"   , "isl::pw_aff", "isl::set", {"isl::pw_aff"}, {}},
    //{"gt_set"   , "isl::pw_aff", "isl::set", {"isl::pw_aff"}, {}},
    //{"eq_set"   , "isl::pw_aff", "isl::set", {"isl::pw_aff"}, {}},
    //{"ne_set"   , "isl::pw_aff", "isl::set", {"isl::pw_aff"}, {}},
//};

////std::vector<ApiFunc> set_gen_funcs = {
    ////ApiFunc("le_set", "isl::pw_aff", {"isl::pw_aff"}, {}),
    ////ApiFunc("ge_set", "isl::pw_aff", {"isl::pw_aff"}, {}),
    ////ApiFunc("lt_set", "isl::pw_aff", {"isl::pw_aff"}, {}),
    ////ApiFunc("gt_set", "isl::pw_aff", {"isl::pw_aff"}, {}),
    ////ApiFunc("eq_set", "isl::pw_aff", {"isl::pw_aff"}, {}),
    ////ApiFunc("ne_set", "isl::pw_aff", {"isl::pw_aff"}, {}),
////};

//ApiFuzzerISL::ApiFuzzerISL(const unsigned int _max_dims,
    //const unsigned int _max_params, const unsigned int _max_constraints)
    //: ApiFuzzer(), dims(std::rand() % _max_dims + 1),
    //params(std::rand() % _max_params + 1),
    //constraints(std::rand() % _max_constraints + 1)
//{
    //this->dim_var_list = std::vector<const ApiObject*>();
    //this->initTypes();
    //this->initFuncs();
//}

//void
//ApiFuzzerISL::initFuncs()
//{
    //const unsigned int func_arr_size = sizeof(isl_funcs) / sizeof(isl_funcs[0]);
    //for (int i = 0; i < func_arr_size; i++) {
        //const ApiType* member_type =
            //isl_funcs[i].member_type_name == "" ?
            //nullptr :
            //this->getTypeByName(isl_funcs[i].member_type_name);
        //const ApiType* return_type =
            //this->getTypeByName(isl_funcs[i].return_type_name);
        //std::vector<const ApiType*> param_types;
        //for (std::string type_str : isl_funcs[i].param_type_name)
            //param_types.push_back(this->getTypeByName(type_str));
        //const ApiFunc* new_func = new ApiFunc(isl_funcs[i].func_name, member_type,
            //return_type, param_types,
            //std::vector<std::string>(isl_funcs[i].conditions));
        //this->addFunc(new_func);
    //}
//}

//void
//ApiFuzzerISL::initTypes()
//{
    //for (const ApiType* type : isl_types)
        //this->addType(type);
//}

//ApiFuzzerISL::~ApiFuzzerISL()
//{
    //this->clearObjs();
    //this->clearFuncs();
    //this->clearTypes();
//}

//void
//ApiFuzzerISL::clearObjs()
//{
    //for (const ApiObject* obj : this->getObjList())
        //delete(obj);
//}

//void
//ApiFuzzerISL::clearFuncs()
//{
    //for (const ApiFunc* func : this->getFuncList())
        //delete(func);
//}

//void
//ApiFuzzerISL::clearTypes()
//{
    //for (const ApiType* type : this->getTypeList())
        //delete(type);
//}

//void
//ApiFuzzerISL::addDimVar(const ApiObject* dim_var)
//{
    //this->dim_var_list.push_back(dim_var);
//}

//std::vector<const ApiObject*>
//ApiFuzzerISL::getDimVarList()
//{
    //return this->dim_var_list;
//}

//const ApiObject*
//ApiFuzzerISL::generateSet()
//{
    //const ApiObject* ctx = this->generateApiObjectAndDecl(
        //"ctx", "isl::ctx", "isl::ctx", { "ctx_ptr" });
    //const ApiObject* space = this->generateApiObjectAndDecl(
        //"space", "isl::space", "isl::space",
        //{ctx->toStr(), std::to_string(params), std::to_string(dims)});
    //const ApiObject* l_space = this->generateApiObjectAndDecl(
        //"local_space", "isl::local_space", "isl::local_space", {space->toStr()});
    //for (unsigned int i = 0; i < this->dims; i++) {
        //const ApiObject* dim_var = this->generateApiObjectAndDecl(
            //"v", "isl::pw_aff", "isl::pw_aff::var_on_domain",
            //{l_space->toStr(), "isl::dim::set", std::to_string(i)});
        //this->addDimVar(dim_var);
    //}
    //for (unsigned int i = 0; i < this->params; i++) {
        //const ApiObject* dim_var = this->generateApiObjectAndDecl(
            //"v", "isl::pw_aff", "isl::pw_aff::var_on_domain",
            //{l_space->toStr(), "isl::dim::param", std::to_string(i)});
        //this->addDimVar(dim_var);
    //}
    //const ApiObject* set = this->generateApiObjectAndDecl(
        //"set", "isl::set", "isl::set::universe", {space->toStr()});
    //for (int i = 0; i < constraints; i++) {
        //const ApiObject* cons1 = this->generatePWAff(ctx);
        //const ApiObject* cons2 = this->generatePWAff(ctx);
        //const ApiObject* cons_set = this->generateSetFromConstraints(cons1, cons2);
        //this->addConstraintFromSet(set, cons_set);
    //}
    //const ApiFunc* dump_func = getFuncByName("dump");
    //this->applyFunc(dump_func, set, nullptr, std::vector<const ApiObject*>());
    //return set;
//}

//const ApiObject*
//ApiFuzzerISL::generateObject(const ApiType* obj_type)
//{
    //if (obj_type->hasName("isl::val"))
        //return generateSimpleVal();
    //std::cout << "Missing object generation for type " << obj_type->toStr();
    //assert(false);
//}

//const ApiObject*
//ApiFuzzerISL::generateObject(std::string obj_name, std::string obj_type)
//{
    //const ApiObject* new_obj = new ApiObject(obj_name, this->getNextID(), this->getTypeByName(obj_type));
    //return new_obj;
//}


//const ApiObject*
//ApiFuzzerISL::getRandomDimVar()
//{
    //std::vector<const ApiObject*> dim_var_list = this->getDimVarList();
    //return dim_var_list.at(std::rand() % dim_var_list.size());
//}

//const ApiObject*
//ApiFuzzerISL::getCtx()
//{
    //const ApiType* ctx_type = this->getTypeByName("isl::ctx");
    //std::vector<const ApiObject*> ctx_list = this->filterObjs(&ApiObject::hasType, ctx_type);
    //assert (ctx_list.size() == 1);
    //return ctx_list.at(0);
//}

//const ApiObject*
//ApiFuzzerISL::generateVal()
//{
    //const ApiObject* val = this->generateSimpleVal();
    //this->augmentVal(val);
    //return val;
//}

//const ApiObject*
//ApiFuzzerISL::generateSimpleVal()
//{
    //const ApiObject* val = this->generateObject("val", "isl::val");
    //const ApiObject* ctx = this->getCtx();
    //this->addInstr(fmt::format("{} = isl::val({}, {});",
        //val->toStrWithType(), ctx->toStr(), (long) std::rand() % 10));
    //return val;
//}

//void
//ApiFuzzerISL::augmentVal(const ApiObject* val)
//{
    //const unsigned int val_augment_count = std::rand() % 5 + 1;
    //for (int i = 0; i < val_augment_count; i++) {
        //std::set<const ApiFunc*> valid_func_list = filterFuncs(
            //&ApiFunc::hasMemberType, this->getTypeByName("isl::val"));
        //const ApiFunc* augment_func = getRandomSetElem(valid_func_list);
        //this->applyFunc(augment_func, val, val);
    //}
//}

//const ApiObject*
//ApiFuzzerISL::getExistingVal()
//{
    //const ApiType* val_type = this->getTypeByName("isl::val");
    //std::vector<const ApiObject*> all_vals = this->filterObjs(&ApiObject::hasType, val_type);
    //if (all_vals.size() == 0) {
        //const ApiObject* ctx = this->getCtx();
        //return this->generateSimpleVal();
    //}
    //return getRandomVectorElem(all_vals);
//}

//const ApiObject*
//ApiFuzzerISL::generatePWAff(const ApiObject* ctx)
//{
    //const ApiObject* pw_aff = this->getRandomDimVar();
    //const ApiObject* cons_pwa = this->generateApiObjectAndDecl(
        //"pw_aff", "isl::pw_aff", "isl::pw_aff", {pw_aff->toStr()});
    //unsigned int op_count = 5;
    //while (op_count-- > 0) {
        //std::set<const ApiFunc*> valid_func_list = filterFuncs(
            //&ApiFunc::hasMemberType, this->getTypeByName("isl::pw_aff"));
        //valid_func_list = filterFuncList(valid_func_list,
            //&ApiFunc::hasReturnType, this->getTypeByName("isl::pw_aff"));
        //const ApiFunc* augment_func = getRandomSetElem(valid_func_list);
        //std::vector<const ApiType*> func_params = augment_func->getParamTypes();
        //// TODO still a hack actually
        //if (func_params.empty())
            //this->applyFunc(augment_func, cons_pwa, cons_pwa);
        //else if (func_params.at(0)->hasName("isl::val"))
            //this->applyFunc(augment_func, cons_pwa, cons_pwa,
                //std::vector<const ApiObject*>({ this->generateVal() }));
        //else if (func_params.at(0)->hasName("isl::pw_aff"))
            //this->applyFunc(augment_func, cons_pwa, cons_pwa,
                //std::vector<const ApiObject*>({ this->getRandomDimVar() }));
    //}
    //return cons_pwa;
//}

//const ApiObject*
//ApiFuzzerISL::generateSetFromConstraints(const ApiObject* cons1, const ApiObject* cons2)
//{
    //std::set<const ApiFunc*> set_gen_funcs = this->filterFuncs(
        //&ApiFunc::hasReturnType, getTypeByName("isl::set"));
    //set_gen_funcs = filterFuncList(set_gen_funcs,
        //&ApiFunc::hasMemberType, getTypeByName("isl::pw_aff"));
    //const ApiFunc* set_decl_func = getRandomSetElem(set_gen_funcs);
    //return this->generateApiObjectAndDecl("set", "isl::set",
        //fmt::format("{}.{}", cons1->toStr(), set_decl_func->getName()),
        //{ cons2->toStr() });
//}

//void
//ApiFuzzerISL::addConstraintFromSet(const ApiObject* set, const ApiObject* constraint)
//{
    //const ApiFunc* intersect_func = getFuncByName("intersect");
    //this->applyFunc(intersect_func, set, set, std::vector<const ApiObject*>({ constraint }));
//}
