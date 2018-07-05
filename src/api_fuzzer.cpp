#include "api_fuzzer.hpp"

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

void
ApiFuzzerISL::generateSet()
{
    this->addInstr("isl_ctx *ctx_ptr = isl_ctx_alloc();");
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
    this->applyFunc(set, "to_str", false, {});
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



struct {
    std::string fn_name;
} val_unary_funcs[] = {
    { "two_exp" },
    { "abs" },
    { "ceil" },
    { "floor" },
    { "inv" },
    { "neg" },
    { "sgn" },
    { "trunc" },
};

struct {
    std::string fn_name;
} val_binary_funcs[] = {
    { "add" },
    { "div" },
    { "gcd" },
    { "max" },
    { "min" },
    { "mod" },
    { "mul" },
    { "sub" },
};

void
ApiFuzzerISL::augmentVal(ApiObject& val)
{
    const unsigned int val_augment_count = std::rand() % 5 + 1;
    for (int i = 0; i < val_augment_count; i++) {
        switch (std::rand() % 2) {
            case 0: {
                std::string fn_name =
                    val_unary_funcs[getRandomFuncID(val_unary_funcs)].fn_name;
                this->applyFunc(val, fn_name, true, {});
                break;
            }
            case 1: {
                std::string fn_name =
                    val_binary_funcs[getRandomFuncID(val_binary_funcs)].fn_name;
                this->applyFunc(val, fn_name, true, { this->getExistingVal().toStr() });
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

struct {
    std::string fn_name;
} unary_pw_aff_ops[] = {
    { "ceil" },
    { "floor" },
};

struct {
    std::string fn_name;
} binary_val_pw_aff_ops[] = {
    { "mod" },
    { "scale" },
};

struct {
    std::string fn_name;
} binary_pw_aff_ops[] = {
    { "add" },
    { "sub" },
    { "max" },
    { "min" },
};

ApiObject
ApiFuzzerISL::generatePWAff(ApiObject& ctx)
{
    ApiObject pw_aff = this->getRandomDimVar();
    ApiObject cons_pwa = this->generateApiObjectAndDecl(
        "pw_aff", "isl::pw_aff", "isl::pw_aff", {pw_aff.toStr()});
    unsigned int op_count = 5;
    while (op_count-- > 0) {
        switch (std::rand() % 3) {
            case 0: {
                std::string func_name =
                    unary_pw_aff_ops[getRandomFuncID(unary_pw_aff_ops)].fn_name;
                std::initializer_list<std::string> func_params = {};
                this->applyFunc(cons_pwa, func_name, true, func_params);
                break;
            }
            case 1: {
                std::string func_name =
                    binary_val_pw_aff_ops[
                        getRandomFuncID(binary_val_pw_aff_ops)].fn_name;
                std::initializer_list<std::string> func_params =
                    { this->generateVal(ctx, false).toStr() };
                this->applyFunc(cons_pwa, func_name, true, func_params);
                break;
            }
            case 2: {
                std::string func_name =
                    binary_pw_aff_ops[getRandomFuncID(binary_pw_aff_ops)].fn_name;
                std::initializer_list<std::string> func_params =
                    { this->getRandomDimVar().toStr() };
                this->applyFunc(cons_pwa, func_name, true, func_params);
                break;
            }
        }
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
