#include "api_fuzzer.hpp"

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
        args_to_string.push_back(obj.toStr());
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

/*******************************************************************************
 * ApiType functions
 ******************************************************************************/

std::string
ApiType::getTypeStr() const
{
    return this->name;
}

bool
ApiType::isType(ApiType other)
{
    return !other.getTypeStr().compare(this->getTypeStr());
}

bool
ApiType::hasName(std::string name_check)
{
    return !this->getTypeStr().compare(name_check);
}

ApiType
ApiType::getVoid()
{
    return ApiType("void");
}

std::string
ApiType::toStr()
{
    return this->name;
}

/*******************************************************************************
 * ApiObject functions
 ******************************************************************************/

std::string
ApiObject::toStr()
{
    return fmt::format("{}_{}", this->name, std::to_string(this->id));
}

std::string
ApiObject::toStrWithType()
{
    return fmt::format("{} {}", this->getType().getTypeStr(), this->toStr());
}

ApiType
ApiObject::getType()
{
    return this->type;
}

bool
ApiObject::hasType(ApiType type_check)
{
    return this->getType().isType(type_check);
}

/*******************************************************************************
 * ApiFunc functions
 ******************************************************************************/

std::string
ApiFunc::getName() const
{
    return this->name;
}

std::vector<ApiType>
ApiFunc::getParamTypes() const
{
    return this->param_types;
}

unsigned int
ApiFunc::getParamCount() const
{
    return this->param_types.size();
}

ApiType
ApiFunc::getMemberType() const
{
    return this->member_type;
}

ApiType
ApiFunc::getReturnType() const
{
    return this->return_type;
}

bool
ApiFunc::hasMemberType(ApiType type_check) const
{
    return this->getMemberType().isType(type_check);
}

bool
ApiFunc::hasReturnType(ApiType return_check) const
{
    return this->getReturnType().isType(return_check);
}

bool
ApiFunc::hasName(std::string name_check) const
{
    return !this->getName().compare(name_check);
}

bool
ApiFunc::hasParamTypes(std::vector<ApiType> param_types_check) const
{
    if (param_types_check.size() != this->param_types.size())
        return false;
    std::vector<ApiType>::iterator to_check =
        param_types_check.begin();
    for (ApiType param_type : this->param_types) {
        if (!param_type.isType(*to_check))
            return false;
        to_check++;
    }
    return true;
}

std::string
ApiFunc::print() const
{
    std::stringstream print_ss;
    if (this->getMemberType().getTypeStr() != "")
        print_ss << this->getMemberType().getTypeStr() << ".";
    print_ss << this->getName() << "(";
    print_ss << makeArgString(this->getParamTypes()) << ")";
    return print_ss.str();
}

/*******************************************************************************
 * ApiFuzzer functions
 ******************************************************************************/

std::vector<std::string>
ApiFuzzer::getInstrList()
{
    return this->instrs;
}

std::vector<ApiObject>
ApiFuzzer::getObjList()
{
    return this->objs;
}

std::set<ApiType>
ApiFuzzer::getTypeList()
{
    return this->types;
}

std::set<ApiFunc>
ApiFuzzer::getFuncList()
{
    return this->funcs;
}

void
ApiFuzzer::addInstr(std::string instr)
{
    this->instrs.push_back(instr);
}

void
ApiFuzzer::addObj(ApiObject obj)
{
    this->objs.push_back(obj);
}

void
ApiFuzzer::addType(ApiType type)
{
    this->types.insert(type);
}

void
ApiFuzzer::addFunc(ApiFunc func)
{
    this->funcs.insert(func);
}

std::vector<ApiObject>
ApiFuzzer::filterObjByType(ApiType obj_type)
{
    std::vector<ApiObject> objs_by_type;
    for (ApiObject obj : this->getObjList())
        if (obj.getType().isType(obj_type))
            objs_by_type.push_back(obj);
    return objs_by_type;
}

std::vector<ApiObject>
ApiFuzzer::filterObjByType(std::string type_name)
{
    ApiType type = this->getTypeByName(type_name);
    return filterObjByType(type);
}

ApiType
ApiFuzzer::getTypeByName(std::string type_check)
{
    for (ApiType type : this->getTypeList())
        if (type.isType(type_check))
            return type;
    std::cout << "Could not find type " << type_check << std::endl;
    std::cout << "List of types:" << std::endl;
    for (ApiType type : this->getTypeList())
        std::cout << "\t" << type.getTypeStr() << std::endl;
    assert(false);
}

template<typename T>
std::set<ApiFunc>
ApiFuzzer::filterFuncs(bool (ApiFunc::*filter_func)(T) const, T filter_check)
{
    return filterFuncs(this->getFuncList(), filter_func, filter_check);
}

template<typename T>
std::set<ApiFunc>
ApiFuzzer::filterFuncs(std::set<ApiFunc> func_list,
    bool (ApiFunc::*filter_func)(T) const, T filter_check)
{
    std::set<ApiFunc> filtered_funcs;
    for (ApiFunc fn : func_list)
        if ((fn.*filter_func)(filter_check))
            filtered_funcs.insert(fn);
    return filtered_funcs;
}


ApiFunc
ApiFuzzer::getFuncByName(std::string name)
{
    std::set<ApiFunc> filtered_funcs = filterFuncs(&ApiFunc::hasName, name);
    return getRandomSetElem(filtered_funcs);
}


unsigned int
ApiFuzzer::getNextID()
{
    this->next_obj_id++;
    return this->next_obj_id - 1;
}

ApiObject
ApiFuzzer::generateApiObjectAndDecl(std::string name, std::string type,
    std::string init_func, std::initializer_list<std::string> init_func_args)
{
    ApiObject new_obj (name, this->getNextID(), type);
    this->addObj(new_obj);
    std::stringstream obj_init_ss;
    obj_init_ss << new_obj.toStrWithType() << " = " << init_func;
    obj_init_ss << "(" << getStringWithDelims(init_func_args, ',') << ");";
    this->addInstr(obj_init_ss.str());
    return new_obj;
}

//ApiObject
//ApiFuzzer::generateApiObject(std::string name, ApiType type)
//{
//}

//ApiObject
//Apifuzzer::generateApiObject(std::string name, ApiType type,
    //ApiFunc init_func, std::vector<ApiObject> init_func_args)
//{
//}

void
ApiFuzzer::applyFunc(ApiFunc& func, ApiObject& target_obj, bool store_result)
{
    std::stringstream apply_func_ss;
    if (store_result)
        apply_func_ss << target_obj.toStr() << " = ";
    apply_func_ss << target_obj.toStr() << "." << func.getName();
    std::vector<ApiObject> func_args = getFuncArgs(func);
    apply_func_ss << "(" << makeArgString(func_args) << ");";
    this->addInstr(apply_func_ss.str());
}

void
ApiFuzzer::applyFunc(ApiFunc& func, ApiObject& target_obj, bool store_result,
    std::vector<ApiObject> func_args)
{
    std::stringstream apply_func_ss;
    if (store_result)
        apply_func_ss << target_obj.toStr() << " = ";
    apply_func_ss << target_obj.toStr() << "." << func.getName();
    apply_func_ss << "(" << makeArgString(func_args) << ");";
    this->addInstr(apply_func_ss.str());
}

void
ApiFuzzer::applyFuncAndStore(ApiFunc& func, ApiObject& target_obj,
    ApiObject& store_obj, std::vector<ApiObject> func_args)
{
    std::stringstream apply_func_ss;
    apply_func_ss << store_obj.toStr() << " = ";
    apply_func_ss << target_obj.toStr() << "." << func.getName();
    apply_func_ss << "(" << makeArgString(func_args) << ");";
    this->addInstr(apply_func_ss.str());
}

std::vector<ApiObject>
ApiFuzzer::getFuncArgs(ApiFunc func/*, ApiObject (*fallbackFunc)(void)*/)
{
    std::vector<ApiType> param_types = func.getParamTypes();
    std::vector<ApiObject> params;
    for (ApiType param_type : param_types) {
        std::vector<ApiObject> candidate_params =
            this->filterObjByType(param_type);
        if (candidate_params.empty())
            params.push_back(this->generateObject(param_type));
        else
            params.push_back(getRandomVectorElem(candidate_params));
    }
    return params;
}

/*******************************************************************************
 * ApiFuzzerISL functions
 ******************************************************************************/

std::vector<ApiType> isl_types = {
    ApiType("isl::val"),
    ApiType("isl::pw_aff"),
    ApiType("isl::set"),
    ApiType("isl::space"),
    ApiType("isl::local_space"),
    ApiType("isl::ctx"),
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
    {"intersect", "isl::set"   , "isl::set", {}, {}},
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
    this->dim_var_list = std::vector<ApiObject>();
    this->initTypes();
    this->initFuncs();
}

void
ApiFuzzerISL::initFuncs()
{
    const unsigned int func_arr_size = sizeof(isl_funcs) / sizeof(isl_funcs[0]);
    for (int i = 0; i < func_arr_size; i++) {
        ApiType member_type =
            isl_funcs[i].member_type_name == "" ?
            (ApiType) NULL :
            this->getTypeByName(isl_funcs[i].member_type_name);
        ApiType return_type =
            isl_funcs[i].return_type_name == "void" ?
            ApiType::getVoid() :
            this->getTypeByName(isl_funcs[i].return_type_name);
        std::vector<ApiType> param_types;
        for (std::string type_str : isl_funcs[i].param_type_name)
            param_types.push_back(this->getTypeByName(type_str));
        ApiFunc new_func(isl_funcs[i].func_name, member_type, return_type,
            param_types, std::vector<std::string>(isl_funcs[i].conditions));
        this->addFunc(new_func);
    }
}

void
ApiFuzzerISL::initTypes()
{
    for (ApiType type : isl_types)
        this->addType(type);
}

void
ApiFuzzerISL::addDimVar(ApiObject dim_var)
{
    this->dim_var_list.push_back(dim_var);
}

std::vector<ApiObject>
ApiFuzzerISL::getDimVarList()
{
    return this->dim_var_list;
}

ApiObject
ApiFuzzerISL::generateSet()
{
    ApiObject ctx = this->generateApiObjectAndDecl(
        "ctx", "isl::ctx", "isl::ctx", { "ctx_ptr" });
    ApiObject space = this->generateApiObjectAndDecl(
        "space", "isl::space", "isl::space",
        {ctx.toStr(), std::to_string(params), std::to_string(dims)});
    ApiObject l_space = this->generateApiObjectAndDecl(
        "local_space", "isl::local_space", "isl::local_space", {space.toStr()});
    for (unsigned int i = 0; i < this->dims; i++) {
        ApiObject dim_var = this->generateApiObjectAndDecl(
            "v", "isl::pw_aff", "isl::pw_aff::var_on_domain",
            {l_space.toStr(), "isl::dim::set", std::to_string(i)});
        this->addDimVar(dim_var);
    }
    for (unsigned int i = 0; i < this->params; i++) {
        ApiObject dim_var = this->generateApiObjectAndDecl(
            "v", "isl::pw_aff", "isl::pw_aff::var_on_domain",
            {l_space.toStr(), "isl::dim::param", std::to_string(i)});
        this->addDimVar(dim_var);
    }
    ApiObject set = this->generateApiObjectAndDecl(
        "set", "isl::set", "isl::set::universe", {space.toStr()});
    for (int i = 0; i < constraints; i++) {
        ApiObject cons1 = this->generatePWAff(ctx);
        ApiObject cons2 = this->generatePWAff(ctx);
        ApiObject cons_set = this->generateSetFromConstraints(cons1, cons2);
        this->addConstraintFromSet(set, cons_set);
    }
    ApiFunc dump_func = getFuncByName("dump");
    this->applyFunc(dump_func, set, false, std::vector<ApiObject>());
    return set;
}

ApiObject
ApiFuzzerISL::generateObject(ApiType obj_type)
{
    if (obj_type.hasName("isl::val"))
        return generateSimpleVal();
    std::cout << "Missing object generation for type " << obj_type.getTypeStr();
    assert(false);
}

ApiObject
ApiFuzzerISL::generateObject(std::string obj_name, std::string obj_type)
{
    ApiObject new_obj(obj_name, this->getNextID(), this->getTypeByName(obj_type));
    return new_obj;
}


ApiObject
ApiFuzzerISL::getRandomDimVar()
{
    std::vector<ApiObject> dim_var_list = this->getDimVarList();
    return dim_var_list.at(std::rand() % dim_var_list.size());
}

ApiObject
ApiFuzzerISL::getCtx()
{
    std::vector<ApiObject> ctx_list = this->filterObjByType("isl::ctx");
    assert (ctx_list.size() == 1);
    return ctx_list.at(0);
}

ApiObject
ApiFuzzerISL::generateVal()
{
    ApiObject val = this->generateSimpleVal();
    this->augmentVal(val);
    return val;
}

ApiObject
ApiFuzzerISL::generateSimpleVal()
{
    ApiObject val = this->generateObject("val", "isl::val");
    //ApiObject val("val", this->getNextID(), "isl::val");
    ApiObject ctx = this->getCtx();
    this->addInstr(fmt::format("{} = isl::val({}, {});",
        val.toStrWithType(), ctx.toStr(), (long) std::rand() % 10));
    return val;
}

void
ApiFuzzerISL::augmentVal(ApiObject& val)
{
    const unsigned int val_augment_count = std::rand() % 5 + 1;
    for (int i = 0; i < val_augment_count; i++) {
        std::set<ApiFunc> valid_func_list = filterFuncs(&ApiFunc::hasMemberType, this->getTypeByName("isl::val"));
        ApiFunc augment_func = getRandomSetElem(valid_func_list);
        this->applyFunc(augment_func, val, true);
    }
}

ApiObject
ApiFuzzerISL::getExistingVal()
{
    std::vector<ApiObject> all_vals = this->filterObjByType("isl::val");
    if (all_vals.size() == 0) {
        ApiObject ctx = this->getCtx();
        return this->generateSimpleVal();
    }
    return this->filterObjByType("isl::val")[std::rand() % all_vals.size()];
}

ApiObject
ApiFuzzerISL::generatePWAff(ApiObject& ctx)
{
    ApiObject pw_aff = this->getRandomDimVar();
    ApiObject cons_pwa = this->generateApiObjectAndDecl(
        "pw_aff", "isl::pw_aff", "isl::pw_aff", {pw_aff.toStr()});
    unsigned int op_count = 5;
    while (op_count-- > 0) {
        std::set<ApiFunc> valid_func_list = filterFuncs(
            &ApiFunc::hasMemberType, this->getTypeByName("isl::pw_aff"));
        valid_func_list = filterFuncs(valid_func_list,
            &ApiFunc::hasReturnType, this->getTypeByName("isl::pw_aff"));
        ApiFunc augment_func = getRandomSetElem(valid_func_list);
        std::vector<ApiType> func_params = augment_func.getParamTypes();
        // TODO still a hack actually
        if (func_params.empty())
            this->applyFunc(augment_func, cons_pwa, true);
        else if (func_params.at(0).hasName("isl::val"))
            this->applyFunc(augment_func, cons_pwa, true,
                std::vector<ApiObject>({ this->generateVal() }));
        else if (func_params.at(0).hasName("isl::pw_aff"))
            this->applyFunc(augment_func, cons_pwa, true,
                std::vector<ApiObject>({ this->getRandomDimVar() }));
    }
    return cons_pwa;
}

ApiObject
ApiFuzzerISL::generateSetFromConstraints(ApiObject& cons1, ApiObject& cons2)
{
    std::set<ApiFunc> set_gen_funcs = this->filterFuncs(
        &ApiFunc::hasReturnType, getTypeByName("isl::set"));
    set_gen_funcs = this->filterFuncs(set_gen_funcs,
        &ApiFunc::hasMemberType, getTypeByName("isl::pw_aff"));
    ApiFunc set_decl_func = getRandomSetElem(set_gen_funcs);
    return this->generateApiObjectAndDecl("set", "isl::set",
        fmt::format("{}.{}", cons1.toStr(), set_decl_func.getName()),
        { cons2.toStr() });
}

void
ApiFuzzerISL::addConstraintFromSet(ApiObject& set, ApiObject& constraint)
{
    ApiFunc intersect_func = getFuncByName("intersect");
    this->applyFunc(intersect_func, set, true, std::vector<ApiObject>({ constraint }));
}
