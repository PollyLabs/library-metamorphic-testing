#include "api_fuzzer.hpp"

std::map<std::string, PrimitiveTypeEnum> primitives_map = {
    { "string", STRING },
    { "unsigned int", UINT },
};

/*******************************************************************************
 * Helper functions
 ******************************************************************************/

std::string
getStringWithDelims(std::vector<std::string> string_list, char delim)
{
    if (string_list.begin() == string_list.end())
        return "";
    std::string string_with_delim = "";
    std::vector<std::string>::iterator it = string_list.begin();
    for (int i = 1; i < string_list.size(); i++) {
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
        args_to_string.push_back(obj->toStr());
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

std::pair<int, int>
parseRange(std::string range_str)
{
    if (range_str.find(",") == std::string::npos)
        return std::pair<int, int>(0, atoi(range_str.c_str()));
    std::string from_str = range_str.substr(0, range_str.find(","));
    std::string to_str = range_str.substr(range_str.find(",") + 1, std::string::npos);
    int from = atoi(from_str.substr(1, std::string::npos).c_str());
    int to = atoi(to_str.c_str());
    if (from_str[0] == '(')
        from++;
    if (to_str[to_str.length() - 1] == ')')
        to--;
    return std::pair<int, int>(from, to);
}

template<typename T>
std::vector<const ApiObject*>
filterObjList(std::vector<const ApiObject*> obj_list,
    bool (ApiObject::*filter_func)(T) const, T filter_check)
{
    std::vector<const ApiObject*> filtered_objs;
    for (const ApiObject* obj : obj_list)
        if ((obj->*filter_func)(filter_check))
            filtered_objs.push_back(obj);
    return filtered_objs;
}

template<typename T>
std::set<const ApiFunc*>
filterFuncList(std::set<const ApiFunc*> func_list,
    bool (ApiFunc::*filter_func)(T) const, T filter_check)
{
    std::set<const ApiFunc*> filtered_funcs;
    for (const ApiFunc* fn : func_list)
        if ((fn->*filter_func)(filter_check))
            filtered_funcs.insert(fn);
    return filtered_funcs;
}

/*******************************************************************************
 * ApiType functions
 ******************************************************************************/

std::string
ApiType::getTypeStr() const
{
    return this->name;
}

bool
ApiType::isType(const ApiType* other) const
{
    return !other->getTypeStr().compare(this->getTypeStr());
}

bool
ApiType::isSingleton() const
{
    return this->singleton;
}

bool
ApiType::isPrimitive() const
{
    return false;
}

bool
ApiType::hasName(std::string name_check) const
{
    return !this->getTypeStr().compare(name_check);
}

ApiType
ApiType::getVoid()
{
    return ApiType("void");
}

std::string
ApiType::toStr() const
{
    return this->name;
}

/*******************************************************************************
 * PrimitiveType functions
 ******************************************************************************/

bool
PrimitiveType::isPrimitive() const
{
    return true;
}

const PrimitiveTypeEnum
PrimitiveType::getTypeEnum() const
{
    return this->type_enum;
}

/*******************************************************************************
 * ApiObject functions
 ******************************************************************************/

bool
ApiObject::isPrimitive() const
{
    return this->getType()->isPrimitive();
}

std::string
ApiObject::toStr() const
{
    return fmt::format("{}_{}", this->name, std::to_string(this->id));
}

std::string
ApiObject::toStrWithType() const
{
    return fmt::format("{} {}", this->getType()->getTypeStr(), this->toStr());
}

const ApiType*
ApiObject::getType() const
{
    return this->type;
}

bool
ApiObject::hasType(const ApiType* type_check) const
{
    return this->getType()->isType(type_check);
}

/*******************************************************************************
 * PrimitiveObject functions
 ******************************************************************************/

template<typename T>
T
PrimitiveObject<T>::getData() const
{
    return this->data;
}

/*******************************************************************************
 * ApiFunc functions
 ******************************************************************************/

std::string
ApiFunc::getName() const
{
    return this->name;
}

std::vector<const ApiType*>
ApiFunc::getParamTypes() const
{
    return this->param_types;
}

const ApiType*
ApiFunc::getParamType(const unsigned int index) const
{
    assert(index < this->getParamCount());
    return this->param_types.at(index);
}

unsigned int
ApiFunc::getParamCount() const
{
    return this->param_types.size();
}

const ApiType*
ApiFunc::getMemberType() const
{
    return this->member_type;
}

const ApiType*
ApiFunc::getReturnType() const
{
    return this->return_type;
}

bool
ApiFunc::hasMemberType(const ApiType* type_check) const
{
    return this->getMemberType()->isType(type_check);
}

bool
ApiFunc::hasReturnType(const ApiType* return_check) const
{
    return this->getReturnType()->isType(return_check);
}

bool
ApiFunc::hasName(std::string name_check) const
{
    return !this->getName().compare(name_check);
}

bool
ApiFunc::hasParamTypes(std::vector<const ApiType*> param_types_check) const
{
    if (param_types_check.size() != this->getParamCount())
        return false;
    for (int i = 0; i < param_types_check.size(); i++)
        if (!param_types_check.at(i)->isType(this->getParamType(i)))
            return false;
    return true;
}

bool
ApiFunc::checkArgs(std::vector<const ApiObject*> args_check) const
{
    if (args_check.size() != this->getParamCount())
        return false;
    for (int i = 0; i < args_check.size(); i++)
        if (!args_check.at(i)->getType()->isType(this->getParamType(i)))
            return false;
    return true;
}

std::string
ApiFunc::printSignature() const
{
    std::stringstream print_ss;
    if (this->getMemberType()->getTypeStr() != "")
        print_ss << this->getMemberType()->getTypeStr() << ".";
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

bool
ApiFuzzer::hasTypeName(std::string type_check)
{
    for (const ApiType* type : this->getTypeList())
        if (type->hasName(type_check))
            return true;
    return false;
}

bool
ApiFuzzer::hasFuncName(std::string func_check)
{
    for (const ApiFunc* func : this->getFuncList())
        if (func->hasName(func_check))
            return true;
    return false;
}

void
ApiFuzzer::addInstr(std::string instr)
{
    this->instrs.push_back(instr);
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
        if (type->hasName(type_check))
            return type;
    std::cout << "Could not find type " << type_check << std::endl;
    std::cout << "List of types:" << std::endl;
    for (const ApiType* type : this->getTypeList())
        std::cout << "\t" << type->getTypeStr() << std::endl;
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
    return getRandomSetElem(filtered_funcs);
}

unsigned int
ApiFuzzer::getNextID()
{
    this->next_obj_id++;
    return this->next_obj_id - 1;
}

ApiObject*
ApiFuzzer::generateApiObjectAndDecl(std::string name, std::string type,
    std::string init_func, std::initializer_list<std::string> init_func_args)
{
    ApiObject* new_obj = new ApiObject(name, this->getNextID(), this->getTypeByName(type));
    this->addObj(new_obj);
    std::stringstream obj_init_ss;
    obj_init_ss << new_obj->toStrWithType() << " = " << init_func;
    obj_init_ss << "(" << getStringWithDelims(init_func_args, ',') << ");";
    this->addInstr(obj_init_ss.str());
    return new_obj;
}

const ApiObject*
ApiFuzzer::generateApiObject(std::string name, const ApiType* type,
    std::vector<const ApiObject*> constructor_args)
{
    std::vector<const ApiType*> constructor_param_types;
    for (const ApiObject* obj : constructor_args)
        constructor_param_types.push_back(obj->getType());
    const ApiFunc constructor_func = ApiFunc(type->toStr(), nullptr, type,
        constructor_param_types, std::vector<std::string>());
    return this->generateApiObject(name, type, &constructor_func,
        constructor_args);
}

const ApiObject*
ApiFuzzer::generateApiObject(std::string name, const ApiType* type,
    const ApiFunc* init_func, std::vector<const ApiObject*> init_func_args)
{
    const ApiObject* new_obj = new ApiObject(name, this->getNextID(), type);
    std::stringstream obj_init_ss;
    obj_init_ss << new_obj->toStrWithType() << " = ";
    obj_init_ss << init_func->printInvocation(init_func_args) << ";";
    this->addObj(new_obj);
    return new_obj;
}

void
ApiFuzzer::applyFunc(const ApiFunc* func, const ApiObject* target_obj,
    const ApiObject* result_obj, std::vector<const ApiObject*> func_args)
{
    std::stringstream apply_func_ss;
    if (!func->checkArgs(func_args)) {
        std::cout << "Invalid arguments given for func " << func->getName();
        std::cout << std::endl << "\tExpected types: " << makeArgString(func->getParamTypes());
        std::vector<std::string> arg_strings;
        for (const ApiObject* arg : func_args)
            arg_strings.push_back(arg->toStrWithType());
        std::cout << std::endl << "\tGiven types: " << getStringWithDelims(arg_strings, ',');
        std::cout << std::endl;
        exit(1);
    }
    if (result_obj != nullptr)
        apply_func_ss << result_obj->toStr() << " = ";
    apply_func_ss << target_obj->toStr() << ".";
    apply_func_ss << func->printInvocation(func_args) << ";";
    this->addInstr(apply_func_ss.str());
}

void
ApiFuzzer::applyFunc(const ApiFunc* func, const ApiObject* target_obj,
    const ApiObject* result_obj)
{
    std::vector<const ApiObject*> func_args = getFuncArgs(func);
    applyFunc(func, target_obj, result_obj, func_args);
}

std::vector<const ApiObject*>
ApiFuzzer::getFuncArgs(const ApiFunc* func)
{
    std::vector<const ApiType*> param_types = func->getParamTypes();
    std::vector<const ApiObject*> params;
    for (const ApiType* param_type : param_types) {
        std::vector<const ApiObject*> candidate_params =
            this->filterObjs(&ApiObject::hasType, param_type);
        if (candidate_params.empty())
            params.push_back(this->generateObject(param_type));
        else
            params.push_back(getRandomVectorElem(candidate_params));
    }
    return params;
}

/*******************************************************************************
 * ApiFuzzerNew functions
 ******************************************************************************/

ApiFuzzerNew::ApiFuzzerNew(std::string& config_file_path) : ApiFuzzer()
{
    YAML::Node config_file = YAML::LoadFile(config_file_path);
    this->initPrimitiveTypes();
    this->initInputs(config_file["inputs"]);
    this->initTypes(config_file["types"]);
    this->initTypes(config_file["singleton_types"]);
    this->initFuncs(config_file["funcs"]);
    this->initFuncs(config_file["special_funcs"]);
    this->initConstructors(config_file["constructors"]);
    this->initGenConfig(config_file["set_gen"]);
    //this->generateSet();
    this->generateObject(this->getTypeByName("isl::set"));
    for (std::string inst : this->getInstrList())
        std::cout << inst << std::endl;
}

void
ApiFuzzerNew::initPrimitiveTypes()
{
    for (std::pair<std::string, PrimitiveTypeEnum> primitive_type_decl :
            primitives_map)
        this->addType(new PrimitiveType(primitive_type_decl.first));
}

void
ApiFuzzerNew::initInputs(YAML::Node inputs_config)
{
    for (YAML::Node input_yaml : inputs_config) {
        std::string name = input_yaml["name"].as<std::string>();
        const ApiType* obj_type = this->getTypeByName(
            input_yaml["type"].as<std::string>());
        const ApiObject* obj;
        if (input_yaml["range"].IsDefined())
            obj = this->generatePrimitiveObject( (PrimitiveType*) obj_type,
                input_yaml["range"].as<std::string>());
        else
            obj = this->generateObject(obj_type);
        this->fuzzer_input.insert(std::pair<std::string, const ApiObject*>(name, obj));
    }
}

void
ApiFuzzerNew::initTypes(YAML::Node types_config)
{
    for (YAML::Node type_yaml : types_config) {
        std::string type_name = type_yaml[0].as<std::string>();
        bool singleton = false;
        if (type_yaml["singleton"].IsDefined())
            singleton = type_yaml["singleton"].as<bool>();
        this->addType(new ApiType(type_name, singleton));
    }
}

void
ApiFuzzerNew::initFuncs(YAML::Node funcs_config)
{
    for (YAML::Node func_yaml : funcs_config) {
        this->addFunc(this->genNewApiFunc(func_yaml));
    }
}

ApiFunc*
ApiFuzzerNew::genNewApiFunc(YAML::Node func_yaml)
{
    std::string func_name = func_yaml["name"].as<std::string>();
    const ApiType* member_type = this->getTypeByName(
        func_yaml["member_type"].as<std::string>());
    const ApiType* return_type = this->getTypeByName(
        func_yaml["return_type"].as<std::string>());
    YAML::Node param_types_list_yaml = func_yaml["param_types"];
    std::vector<const ApiType*> param_type_list;
    for (YAML::Node param_type_yaml : param_types_list_yaml)
        param_type_list.push_back(
            this->getTypeByName(param_type_yaml.as<std::string>()));
    YAML::Node cond_list_yaml = func_yaml["conditions"];
    std::vector<std::string> cond_list;
    for (YAML::Node cond_yaml : cond_list_yaml)
        cond_list.push_back(cond_yaml.as<std::string>());
    std::string hint = "";
    if (func_yaml["hint"].IsDefined())
        hint = func_yaml["hint"].as<std::string>();
    return new ApiFunc(func_name, member_type, return_type, param_type_list,
        cond_list, hint);
}

void
ApiFuzzerNew::initConstructors(YAML::Node ctors_yaml)
{
    for (YAML::Node ctor_yaml : ctors_yaml) {
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
    if (type_str[0] == '<' && type_str[type_str.length() - 1] == '>' &&
            type_str.find("input:") != std::string::npos) {
        unsigned int substr_length = type_str.find('>') - type_str.find(':') - 1;
        std::string param_name = type_str.substr(type_str.find(":") + 1,
            substr_length);
        return this->fuzzer_input[param_name]->getType();
    }
    return this->getTypeByName(type_str);
}

void
ApiFuzzerNew::initGenConfig(YAML::Node gen_config_yaml)
{
    for (YAML::Node gen_config_instr : gen_config_yaml)
        this->set_gen_instrs.push_back(gen_config_instr);
}

const ApiObject*
ApiFuzzerNew::generateObject(const ApiType* obj_type)
{
    if (obj_type->isSingleton())
        return this->getSingletonObject(obj_type);
    else if (obj_type->isPrimitive())
        return this->generatePrimitiveObject((PrimitiveType*) obj_type);
    else
        return this->generateNewObject(obj_type);
}

const ApiObject*
ApiFuzzerNew::generateNewObject(const ApiType* obj_type)
{
    if (obj_type->isPrimitive())
        return generatePrimitiveObject((const PrimitiveType*) obj_type);
    std::set<const ApiFunc*> ctor_func_candidates = this->filterFuncs(
        &ApiFunc::hasReturnType, obj_type);
    const ApiFunc* gen_func = getRandomSetElem(ctor_func_candidates);
    std::vector<const ApiObject*> ctor_args = this->getFuncArgs(gen_func);
    return generateApiObject("v" + std::to_string(this->getNextID()),
        obj_type, gen_func, ctor_args);
}

const ApiObject*
ApiFuzzerNew::generatePrimitiveObject(const PrimitiveType* obj_type)
{
    assert(obj_type->isPrimitive());
    switch(obj_type->getTypeEnum()) {
        case UINT:
            // HACKS
            return new PrimitiveObject<unsigned int>(obj_type, std::rand() % 10);
    }
    assert(false);
}

const ApiObject*
ApiFuzzerNew::generatePrimitiveObject(const PrimitiveType* obj_type,
    std::string range)
{
    assert(obj_type->hasName("unsigned int"));
    switch(obj_type->getTypeEnum()) {
        case UINT: {
            std::pair<int, int> int_range = parseRange(range);
            return new PrimitiveObject<unsigned int>(obj_type,
                std::experimental::randint(int_range.first, int_range.second));
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
    for (YAML::Node gen_instr_yaml : this->set_gen_instrs) {
        std::string gen_instr_type = gen_instr_yaml[0].as<std::string>();
        if (!gen_instr_type.compare("for"))
            this->generateForLoop(gen_instr_yaml);
        else if (this->hasTypeName(gen_instr_type))
            this->generateConstructor(gen_instr_yaml);
        else if (this->hasFuncName(gen_instr_type))
            this->generateFunc(gen_instr_yaml);
        assert(false);
    }
}

void
ApiFuzzerNew::generateForLoop(YAML::Node instr_config)
{
    //std::string count_hint = instr_config[1].as<std::string>();
    //std::pair<int, int> count = parseCountHint(count_hint);
    //for (int i = count.first; i <= count.second; i++)
        //this->

}

void
ApiFuzzerNew::generateConstructor(YAML::Node instr_config)
{
}

void
ApiFuzzerNew::generateFunc(YAML::Node instr_config)
{
}

/*******************************************************************************
 * ApiFuzzerISL functions
 ******************************************************************************/

std::vector<const ApiType*> isl_types = {
    new ApiType("isl::val"),
    new ApiType("isl::pw_aff"),
    new ApiType("isl::set"),
    new ApiType("isl::space"),
    new ApiType("isl::local_space"),
    new ApiType("isl::ctx"),
    new ApiType("void"),
};

struct {
    std::string func_name;
    std::string member_type_name;
    std::string return_type_name;
    std::initializer_list<std::string> param_type_name;
    std::initializer_list<std::string> conditions;
} isl_funcs[] = {

//std::vector<ApiFunc> isl_funcs = {
    // isl::val unary funcs
    {"two_exp", "isl::val", "isl::val", {}, {}},
    {"abs"    , "isl::val", "isl::val", {}, {}},
    {"ceil"   , "isl::val", "isl::val", {}, {}},
    {"floor"  , "isl::val", "isl::val", {}, {}},
    {"inv"    , "isl::val", "isl::val", {}, {}},
    {"neg"    , "isl::val", "isl::val", {}, {}},
    {"trunc"  , "isl::val", "isl::val", {}, {}},
    // isl::val binary funcs
    //ApiFunc{"gcd", "isl::val", {"isl::val"}, {}},
    //ApiFunc{"mod", "isl::val", {"isl::val"}, {}},
    {"add"    , "isl::val", "isl::val", {"isl::val"}, {}},
    {"div"    , "isl::val", "isl::val", {"isl::val"}, {}},
    {"max"    , "isl::val", "isl::val", {"isl::val"}, {}},
    {"min"    , "isl::val", "isl::val", {"isl::val"}, {}},
    {"mul"    , "isl::val", "isl::val", {"isl::val"}, {}},
    {"sub"    , "isl::val", "isl::val", {"isl::val"}, {}},
    // isl::pw_aff unary funcs
    {"ceil"   , "isl::pw_aff", "isl::pw_aff", {}, {}},
    {"floor"  , "isl::pw_aff", "isl::pw_aff", {}, {}},
    // isl::pw_aff binary funcs
    //ApiFunc{"mod", "isl::pw_aff", {"isl::val"}, {}},
    //ApiFunc{"scale", "isl::pw_aff", {"isl::val"}, {}},
    {"add"    , "isl::pw_aff"  , "isl::pw_aff", {"isl::pw_aff"}, {}},
    {"sub"    , "isl::pw_aff"  , "isl::pw_aff", {"isl::pw_aff"}, {}},
    {"max"    , "isl::pw_aff"  , "isl::pw_aff", {"isl::pw_aff"}, {}},
    {"min"    , "isl::pw_aff"  , "isl::pw_aff", {"isl::pw_aff"}, {}},
    // isl::set generation funcs from isl::pw_aff
    // Other useful funcs
    {"dump"     , "isl::set"   , "void", {}, {}},
    {"intersect", "isl::set"   , "isl::set", {"isl::set"}, {}},
    {"le_set"   , "isl::pw_aff", "isl::set", {"isl::pw_aff"}, {}},
    {"ge_set"   , "isl::pw_aff", "isl::set", {"isl::pw_aff"}, {}},
    {"lt_set"   , "isl::pw_aff", "isl::set", {"isl::pw_aff"}, {}},
    {"gt_set"   , "isl::pw_aff", "isl::set", {"isl::pw_aff"}, {}},
    {"eq_set"   , "isl::pw_aff", "isl::set", {"isl::pw_aff"}, {}},
    {"ne_set"   , "isl::pw_aff", "isl::set", {"isl::pw_aff"}, {}},
};

//std::vector<ApiFunc> set_gen_funcs = {
    //ApiFunc("le_set", "isl::pw_aff", {"isl::pw_aff"}, {}),
    //ApiFunc("ge_set", "isl::pw_aff", {"isl::pw_aff"}, {}),
    //ApiFunc("lt_set", "isl::pw_aff", {"isl::pw_aff"}, {}),
    //ApiFunc("gt_set", "isl::pw_aff", {"isl::pw_aff"}, {}),
    //ApiFunc("eq_set", "isl::pw_aff", {"isl::pw_aff"}, {}),
    //ApiFunc("ne_set", "isl::pw_aff", {"isl::pw_aff"}, {}),
//};

ApiFuzzerISL::ApiFuzzerISL(const unsigned int _max_dims,
    const unsigned int _max_params, const unsigned int _max_constraints)
    : ApiFuzzer(), dims(std::rand() % _max_dims + 1),
    params(std::rand() % _max_params + 1),
    constraints(std::rand() % _max_constraints + 1)
{
    this->dim_var_list = std::vector<const ApiObject*>();
    this->initTypes();
    this->initFuncs();
}

void
ApiFuzzerISL::initFuncs()
{
    const unsigned int func_arr_size = sizeof(isl_funcs) / sizeof(isl_funcs[0]);
    for (int i = 0; i < func_arr_size; i++) {
        const ApiType* member_type =
            isl_funcs[i].member_type_name == "" ?
            nullptr :
            this->getTypeByName(isl_funcs[i].member_type_name);
        const ApiType* return_type =
            this->getTypeByName(isl_funcs[i].return_type_name);
        std::vector<const ApiType*> param_types;
        for (std::string type_str : isl_funcs[i].param_type_name)
            param_types.push_back(this->getTypeByName(type_str));
        const ApiFunc* new_func = new ApiFunc(isl_funcs[i].func_name, member_type,
            return_type, param_types,
            std::vector<std::string>(isl_funcs[i].conditions));
        this->addFunc(new_func);
    }
}

void
ApiFuzzerISL::initTypes()
{
    for (const ApiType* type : isl_types)
        this->addType(type);
}

ApiFuzzerISL::~ApiFuzzerISL()
{
    this->clearObjs();
    this->clearFuncs();
    this->clearTypes();
}

void
ApiFuzzerISL::clearObjs()
{
    for (const ApiObject* obj : this->getObjList())
        delete(obj);
}

void
ApiFuzzerISL::clearFuncs()
{
    for (const ApiFunc* func : this->getFuncList())
        delete(func);
}

void
ApiFuzzerISL::clearTypes()
{
    for (const ApiType* type : this->getTypeList())
        delete(type);
}

void
ApiFuzzerISL::addDimVar(const ApiObject* dim_var)
{
    this->dim_var_list.push_back(dim_var);
}

std::vector<const ApiObject*>
ApiFuzzerISL::getDimVarList()
{
    return this->dim_var_list;
}

const ApiObject*
ApiFuzzerISL::generateSet()
{
    const ApiObject* ctx = this->generateApiObjectAndDecl(
        "ctx", "isl::ctx", "isl::ctx", { "ctx_ptr" });
    const ApiObject* space = this->generateApiObjectAndDecl(
        "space", "isl::space", "isl::space",
        {ctx->toStr(), std::to_string(params), std::to_string(dims)});
    const ApiObject* l_space = this->generateApiObjectAndDecl(
        "local_space", "isl::local_space", "isl::local_space", {space->toStr()});
    for (unsigned int i = 0; i < this->dims; i++) {
        const ApiObject* dim_var = this->generateApiObjectAndDecl(
            "v", "isl::pw_aff", "isl::pw_aff::var_on_domain",
            {l_space->toStr(), "isl::dim::set", std::to_string(i)});
        this->addDimVar(dim_var);
    }
    for (unsigned int i = 0; i < this->params; i++) {
        const ApiObject* dim_var = this->generateApiObjectAndDecl(
            "v", "isl::pw_aff", "isl::pw_aff::var_on_domain",
            {l_space->toStr(), "isl::dim::param", std::to_string(i)});
        this->addDimVar(dim_var);
    }
    const ApiObject* set = this->generateApiObjectAndDecl(
        "set", "isl::set", "isl::set::universe", {space->toStr()});
    for (int i = 0; i < constraints; i++) {
        const ApiObject* cons1 = this->generatePWAff(ctx);
        const ApiObject* cons2 = this->generatePWAff(ctx);
        const ApiObject* cons_set = this->generateSetFromConstraints(cons1, cons2);
        this->addConstraintFromSet(set, cons_set);
    }
    const ApiFunc* dump_func = getFuncByName("dump");
    this->applyFunc(dump_func, set, nullptr, std::vector<const ApiObject*>());
    return set;
}

const ApiObject*
ApiFuzzerISL::generateObject(const ApiType* obj_type)
{
    if (obj_type->hasName("isl::val"))
        return generateSimpleVal();
    std::cout << "Missing object generation for type " << obj_type->getTypeStr();
    assert(false);
}

const ApiObject*
ApiFuzzerISL::generateObject(std::string obj_name, std::string obj_type)
{
    const ApiObject* new_obj = new ApiObject(obj_name, this->getNextID(), this->getTypeByName(obj_type));
    return new_obj;
}


const ApiObject*
ApiFuzzerISL::getRandomDimVar()
{
    std::vector<const ApiObject*> dim_var_list = this->getDimVarList();
    return dim_var_list.at(std::rand() % dim_var_list.size());
}

const ApiObject*
ApiFuzzerISL::getCtx()
{
    const ApiType* ctx_type = this->getTypeByName("isl::ctx");
    std::vector<const ApiObject*> ctx_list = this->filterObjs(&ApiObject::hasType, ctx_type);
    assert (ctx_list.size() == 1);
    return ctx_list.at(0);
}

const ApiObject*
ApiFuzzerISL::generateVal()
{
    const ApiObject* val = this->generateSimpleVal();
    this->augmentVal(val);
    return val;
}

const ApiObject*
ApiFuzzerISL::generateSimpleVal()
{
    const ApiObject* val = this->generateObject("val", "isl::val");
    const ApiObject* ctx = this->getCtx();
    this->addInstr(fmt::format("{} = isl::val({}, {});",
        val->toStrWithType(), ctx->toStr(), (long) std::rand() % 10));
    return val;
}

void
ApiFuzzerISL::augmentVal(const ApiObject* val)
{
    const unsigned int val_augment_count = std::rand() % 5 + 1;
    for (int i = 0; i < val_augment_count; i++) {
        std::set<const ApiFunc*> valid_func_list = filterFuncs(
            &ApiFunc::hasMemberType, this->getTypeByName("isl::val"));
        const ApiFunc* augment_func = getRandomSetElem(valid_func_list);
        this->applyFunc(augment_func, val, val);
    }
}

const ApiObject*
ApiFuzzerISL::getExistingVal()
{
    const ApiType* val_type = this->getTypeByName("isl::val");
    std::vector<const ApiObject*> all_vals = this->filterObjs(&ApiObject::hasType, val_type);
    if (all_vals.size() == 0) {
        const ApiObject* ctx = this->getCtx();
        return this->generateSimpleVal();
    }
    return getRandomVectorElem(all_vals);
}

const ApiObject*
ApiFuzzerISL::generatePWAff(const ApiObject* ctx)
{
    const ApiObject* pw_aff = this->getRandomDimVar();
    const ApiObject* cons_pwa = this->generateApiObjectAndDecl(
        "pw_aff", "isl::pw_aff", "isl::pw_aff", {pw_aff->toStr()});
    unsigned int op_count = 5;
    while (op_count-- > 0) {
        std::set<const ApiFunc*> valid_func_list = filterFuncs(
            &ApiFunc::hasMemberType, this->getTypeByName("isl::pw_aff"));
        valid_func_list = filterFuncList(valid_func_list,
            &ApiFunc::hasReturnType, this->getTypeByName("isl::pw_aff"));
        const ApiFunc* augment_func = getRandomSetElem(valid_func_list);
        std::vector<const ApiType*> func_params = augment_func->getParamTypes();
        // TODO still a hack actually
        if (func_params.empty())
            this->applyFunc(augment_func, cons_pwa, cons_pwa);
        else if (func_params.at(0)->hasName("isl::val"))
            this->applyFunc(augment_func, cons_pwa, cons_pwa,
                std::vector<const ApiObject*>({ this->generateVal() }));
        else if (func_params.at(0)->hasName("isl::pw_aff"))
            this->applyFunc(augment_func, cons_pwa, cons_pwa,
                std::vector<const ApiObject*>({ this->getRandomDimVar() }));
    }
    return cons_pwa;
}

const ApiObject*
ApiFuzzerISL::generateSetFromConstraints(const ApiObject* cons1, const ApiObject* cons2)
{
    std::set<const ApiFunc*> set_gen_funcs = this->filterFuncs(
        &ApiFunc::hasReturnType, getTypeByName("isl::set"));
    set_gen_funcs = filterFuncList(set_gen_funcs,
        &ApiFunc::hasMemberType, getTypeByName("isl::pw_aff"));
    const ApiFunc* set_decl_func = getRandomSetElem(set_gen_funcs);
    return this->generateApiObjectAndDecl("set", "isl::set",
        fmt::format("{}.{}", cons1->toStr(), set_decl_func->getName()),
        { cons2->toStr() });
}

void
ApiFuzzerISL::addConstraintFromSet(const ApiObject* set, const ApiObject* constraint)
{
    const ApiFunc* intersect_func = getFuncByName("intersect");
    this->applyFunc(intersect_func, set, set, std::vector<const ApiObject*>({ constraint }));
}
