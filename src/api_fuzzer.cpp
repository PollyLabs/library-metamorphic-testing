#include "api_fuzzer.hpp"

/*******************************************************************************
 * Helper functions
 ******************************************************************************/

template<typename T>
const unsigned int
getRandomFuncID(T &func_struct)
{
    return (std::rand() % sizeof(func_struct) / sizeof(func_struct[0]));
}

std::string
getStringWithDelims(std::initializer_list<std::string> string_list, char delim)
{
    if (string_list.begin() == string_list.end())
        return "";
    std::string string_with_delim = "";
    std::initializer_list<std::string>::iterator it = string_list.begin();
    for (int i = 1; i < string_list.size(); i++) {
        string_with_delim += *it + delim + " ";
        it++;
    }
    string_with_delim += *it;
    return string_with_delim;
}

std::vector<ApiFunc>
filterFuncs(std::vector<ApiFunc> func_list, std::string member_check,
    std::initializer_list<std::string> params_check)
{
    std::vector<ApiFunc> filtered_funcs;
    for (ApiFunc fn : func_list) {
        if (fn.isMemberOf(member_check) && fn.hasParamTypes(params_check))
            filtered_funcs.push_back(fn);
    }
    return filtered_funcs;
}

std::vector<ApiFunc>
filterFuncsByMember(std::vector<ApiFunc> func_list, std::string member_check)
{
    std::vector<ApiFunc> filtered_funcs;
    for (ApiFunc fn : func_list) {
        if (fn.isMemberOf(member_check))
            filtered_funcs.push_back(fn);
    }
    return filtered_funcs;
}

template<typename T>
T
getRandomVectorElem(std::vector<T>& vector_in)
{
    return vector_in.at(std::rand() % vector_in.size());
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
    return fmt::format("{} {}", this->type, this->toStr());
}

std::string
ApiObject::getType()
{
    return this->type;
}

/*******************************************************************************
 * ApiFunc functions
 ******************************************************************************/

bool
ApiFunc::isMemberOf(std::string member)
{
    return !this->member_of.compare(member);
}

bool
ApiFunc::hasParamTypes(std::initializer_list<std::string> params_to_check)
{
    std::initializer_list<std::string>::iterator to_check =
        params_to_check.begin();
    for (std::string func_param : this->param_types) {
        if (func_param.compare(*to_check))
            return false;
        to_check++;
    }
    return true;
}

unsigned int
ApiFunc::getParamCount()
{
    return this->param_types.size();
}

std::string
ApiFunc::getName()
{
    return this->name;
}

std::vector<std::string>
ApiFunc::getParamTypes()
{
    return this->param_types;
}

/*******************************************************************************
 * ApiFuzzer functions
 ******************************************************************************/

std::vector<std::string>
ApiFuzzer::getInstrs()
{
    return this->instrs;
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

std::vector<ApiObject>
ApiFuzzer::getObjList()
{
    return this->objs;
}

std::vector<ApiObject>
ApiFuzzer::getObjByType(std::string obj_type)
{
    std::vector<ApiObject> objs_by_type;
    for (ApiObject obj : this->getObjList())
        if (!obj.getType().compare(obj_type))
            objs_by_type.push_back(obj);
    return objs_by_type;
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

void
ApiFuzzer::applyFunc(ApiObject& obj, std::string fn_name, bool store_result,
    std::initializer_list<std::string> fn_args)
{
    std::stringstream apply_func_ss;
    if (store_result)
        apply_func_ss << obj.toStr() << " = ";
    apply_func_ss << obj.toStr() << "." << fn_name;
    apply_func_ss << "(" << getStringWithDelims(fn_args, ',') << ");";
    this->addInstr(apply_func_ss.str());
}

/*******************************************************************************
 * ApiFuzzerISL functions
 ******************************************************************************/

std::vector<ApiFunc> isl_funcs = {
    // isl::val unary funcs
    ApiFunc("two_exp", "isl::val", {}, {}),
    ApiFunc("abs", "isl::val", {}, {}),
    ApiFunc("ceil", "isl::val", {}, {}),
    ApiFunc("floor", "isl::val", {}, {}),
    ApiFunc("inv", "isl::val", {}, {}),
    ApiFunc("neg", "isl::val", {}, {}),
    ApiFunc("trunc", "isl::val", {}, {}),
    // isl::val binary funcs
    ApiFunc("add", "isl::val", {"isl::val"}, {}),
    ApiFunc("div", "isl::val", {"isl::val"}, {}),
    //ApiFunc("gcd", "isl::val", {"isl::val"}, {}),
    ApiFunc("max", "isl::val", {"isl::val"}, {}),
    ApiFunc("min", "isl::val", {"isl::val"}, {}),
    //ApiFunc("mod", "isl::val", {"isl::val"}, {}),
    ApiFunc("mul", "isl::val", {"isl::val"}, {}),
    ApiFunc("sub", "isl::val", {"isl::val"}, {}),
    // isl::pw_aff unary funcs
    ApiFunc("ceil", "isl::pw_aff", {}, {}),
    ApiFunc("floor", "isl::pw_aff", {}, {}),
    // isl::pw_aff binary funcs
    //ApiFunc("mod", "isl::pw_aff", {"isl::val"}, {}},
    //ApiFunc("scale", "isl::pw_aff", {"isl::val"}, {}},
    ApiFunc("add", "isl::pw_aff", {"isl::pw_aff"}, {}),
    ApiFunc("sub", "isl::pw_aff", {"isl::pw_aff"}, {}),
    ApiFunc("max", "isl::pw_aff", {"isl::pw_aff"}, {}),
    ApiFunc("min", "isl::pw_aff", {"isl::pw_aff"}, {}),
};

ApiFuzzerISL::ApiFuzzerISL(const unsigned int _max_dims,
    const unsigned int _max_params, const unsigned int _max_constraints)
    : ApiFuzzer(), dims(std::rand() % _max_dims + 1),
    params(std::rand() % _max_params + 1),
    constraints(std::rand() % _max_constraints + 1)
{
    this->dim_var_list = std::vector<ApiObject>();
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
    this->applyFunc(set, "dump", false, {});
    return set;
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
    std::vector<ApiObject> ctx_list = this->getObjByType("isl::ctx");
    assert (ctx_list.size() == 1);
    return ctx_list.at(0);
}

ApiObject
ApiFuzzerISL::generateVal(ApiObject& ctx, bool simple = false)
{
    ApiObject val("val", this->getNextID(), "isl::val");
    this->addInstr(fmt::format("{} = isl::val({}, {});",
        val.toStrWithType(), ctx.toStr(), (long) std::rand() % 10));
    if (!simple)
        this->augmentVal(val);
    return val;
}

void
ApiFuzzerISL::augmentVal(ApiObject& val)
{
    const unsigned int val_augment_count = std::rand() % 5 + 1;
    for (int i = 0; i < val_augment_count; i++) {
        std::vector<ApiFunc> valid_func_list = filterFuncsByMember(isl_funcs, "isl::val");
        ApiFunc augment_func = getRandomVectorElem(valid_func_list);
        switch (augment_func.getParamCount()) {
            case 0: {
                this->applyFunc(val, augment_func.getName(), true, {});
                break;
            }
            case 1: {
                this->applyFunc(val, augment_func.getName(), true,
                    { this->getExistingVal().toStr() });
                break;
            }
        }
    }
}

ApiObject
ApiFuzzerISL::getExistingVal()
{
    std::vector<ApiObject> all_vals = this->getObjByType("isl::val");
    if (all_vals.size() == 0) {
        ApiObject ctx = this->getCtx();
        return this->generateVal(ctx, true);
    }
    return this->getObjByType("isl::val")[std::rand() % all_vals.size()];
}

ApiObject
ApiFuzzerISL::generatePWAff(ApiObject& ctx)
{
    ApiObject pw_aff = this->getRandomDimVar();
    ApiObject cons_pwa = this->generateApiObjectAndDecl(
        "pw_aff", "isl::pw_aff", "isl::pw_aff", {pw_aff.toStr()});
    unsigned int op_count = 5;
    while (op_count-- > 0) {
        std::vector<ApiFunc> valid_func_list =
            filterFuncsByMember(isl_funcs, "isl::pw_aff");
        ApiFunc augment_func = getRandomVectorElem(valid_func_list);
        std::vector<std::string> func_params = augment_func.getParamTypes();
        if (func_params.empty())
            this->applyFunc(cons_pwa, augment_func.getName(), true, {});
        // TODO some hacks, will fix once implementing more ApiFunc stuff
        else if (!func_params.at(0).compare("isl::val"))
            this->applyFunc(cons_pwa, augment_func.getName(), true,
                { this->generateVal(ctx, false).toStr() });
        else if (!func_params.at(0).compare("isl::pw_aff"))
            this->applyFunc(cons_pwa, augment_func.getName(), true,
                { this->getRandomDimVar().toStr() });
    }
    return cons_pwa;
}

struct {
    std::string fn_name;
} set_gen_fns[] = {
    { "isl::pw_aff::le_set" },
    { "isl::pw_aff::ge_set" },
    { "isl::pw_aff::lt_set" },
    { "isl::pw_aff::gt_set" },
    { "isl::pw_aff::eq_set" },
    { "isl::pw_aff::ne_set" },
};

ApiObject
ApiFuzzerISL::generateSetFromConstraints(ApiObject& cons1, ApiObject& cons2)
{
    std::string set_decl_fn_name =
        set_gen_fns[getRandomFuncID(set_gen_fns)].fn_name;
    return this->generateApiObjectAndDecl("set", "isl::set",
        fmt::format("{}.{}", cons1.toStr(), set_decl_fn_name),
        { cons2.toStr() });
}

void
ApiFuzzerISL::addConstraintFromSet(ApiObject& set, ApiObject& constraint)
{
    this->applyFunc(set, "intersect", true, { constraint.toStr() });
}
