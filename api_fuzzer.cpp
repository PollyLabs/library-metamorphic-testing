#include "api_fuzzer.hpp"

std::string
ApiObject::to_str()
{
    return fmt::format("{}_{}", this->name, std::to_string(this->id));
}

std::string
ApiObject::to_str_with_type()
{
    return fmt::format("{} {}", this->type, this->to_str());
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

unsigned int
ApiFuzzer::getNextID()
{
    this->next_obj_id++;
    return this->next_obj_id - 1;
}

ApiFuzzerISL::ApiFuzzerISL() : ApiFuzzer()
{
    this->inputs = std::vector<ApiObject>();
}

std::vector<ApiObject>
ApiFuzzerISL::getDimVarList()
{
    return this->inputs;
}

ApiObject
ApiFuzzerISL::generateApiObjectAndDecl(std::string name, std::string type,
    std::string init_func, std::initializer_list<std::string> init_func_args)
{
    ApiObject new_obj (name, this->getNextID(), type);
    this->addObj(new_obj);
    std::stringstream obj_init_ss;
    obj_init_ss << new_obj.to_str_with_type() << " = " << init_func;
    obj_init_ss << "(";

    std::initializer_list<std::string>::iterator it = init_func_args.begin();
    for (int i = 0; i < init_func_args.size() - 1; i++) {
        obj_init_ss << *it << ", ";
        it++;
    }
    obj_init_ss << *it << ");";
    this->addInstr(obj_init_ss.str());
    return new_obj;
}

void
ApiFuzzerISL::generateSet()
{
    const unsigned int dims = 5;
    const unsigned int params = 5;
    ApiObject ctx = this->generateContext();
    ApiObject space = this->generateSpace(ctx, 5, 5);
    ApiObject l_space = this->generateLocalSpace(space);
    for (unsigned int i = 0; i < dims; i++)
        this->generateDimVar(l_space, "isl::dim::set", i);
    for (unsigned int i = 0; i < params; i++)
        this->generateDimVar(l_space, "isl::dim::param", i);
    ApiObject constraint = this->generateConstraint(ctx);
    ApiObject set = this->generateSetDecl(space);
    this->addConstraintToSet(set, constraint);
    //std::string new_set_decl = "isl::set s" + set_id + " = isl::set::universe(space)";
    //this->addInstr();
}

ApiObject
ApiFuzzerISL::generateSetDecl(ApiObject space)
{
    return this->generateApiObjectAndDecl("set", "isl::set", "isl::set::universe", {space.to_str()});
    ApiObject set ("set", this->getNextID(), "isl::set");
    this->addInstr(fmt::format(
        "{} = isl::set::universe({});",
        set.to_str_with_type(), space.to_str()));
    return set;
}

ApiObject
ApiFuzzerISL::generateContext()
{
    ApiObject ctx("ctx", this->getNextID(), "isl::ctx");
    this->addInstr("isl_ctx *ctx_ptr = isl_ctx_alloc();");
    this->addInstr(fmt::format("{} (ctx_ptr);", ctx.to_str_with_type()));
    return ctx;
}

ApiObject
ApiFuzzerISL::generateSpace(ApiObject ctx, const unsigned int params, const unsigned int dims)
{
    ApiObject space("space", this->getNextID(), "isl::space");
    this->addInstr(fmt::format(
        "{} ({}, {}, {});",
        space.to_str_with_type(), ctx.to_str(), params, dims));
    return space;
}

ApiObject
ApiFuzzerISL::generateLocalSpace(ApiObject space)
{
    ApiObject l_space("local_space", this->getNextID(), "isl::local_space");
    this->addInstr(fmt::format(
        "{} = isl::local_space({});",
        l_space.to_str_with_type(), space.to_str()));
    return l_space;
}

ApiObject
ApiFuzzerISL::generateDimVar(ApiObject l_space, std::string dim_type, const unsigned int i)
{
    ApiObject dim_var("v", this->getNextID(), "isl::pw_aff");
    this->addInstr(fmt::format(
        "{} = isl::pw_aff::var_on_domain({}, {}, {});",
        dim_var.to_str_with_type(), l_space.to_str(), dim_type, i));
    this->inputs.push_back(dim_var);
    return dim_var;
}

ApiObject
ApiFuzzerISL::getRandomDimVar()
{
    std::vector<ApiObject> dim_var_list = this->getDimVarList();
    return dim_var_list.at(std::rand() % dim_var_list.size());
}


ApiObject
ApiFuzzerISL::generateVal(ApiObject ctx)
{
    ApiObject val("val", this->getNextID(), "isl::val");
    this->addInstr(fmt::format("{} = isl::val({}, {});",
        val.to_str_with_type(), ctx.to_str(), (long) std::rand() % 10));
    return val;
}

ApiObject
ApiFuzzerISL::getVal(ApiObject ctx)
{
    //if (std::rand() % 3 < 1) // 33% chance to select an existing var
        return this->getRandomDimVar();
    //return this->generateVal(ctx);
}

ApiObject
ApiFuzzerISL::generateConstraint(ApiObject ctx)
{
    ApiObject cons("constraint", this->getNextID(), "isl::constraint");
    this->addInstr(fmt::format("{} = {};",
        cons.to_str_with_type(), this->getVal(ctx).to_str()));
    return cons;
}

void
ApiFuzzerISL::addConstraintToSet(ApiObject set, ApiObject constraint)
{
    this->addInstr(fmt::format("{}.add_constraint({});",
        set.to_str(), constraint.to_str()));
}
