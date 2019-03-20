#include "api_elements.hpp"

std::map<std::string, std::vector<char>> char_set =
    {
     {"numeric", {'0','1','2','3','4','5','6','7','8','9'}},
     {"low_alpha", {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
                    'p','q','r','s','t','u','v','w','x','y','z'}},
     {"up_alpha", {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
                   'P','Q','R','S','T','U','V','W','X','Y','Z'}},
     {"symbol", {'`','!','$','%','^','&','*','(',')','_','+','{','}','[',']',
                 ';',':','\'','@','#','~',',','<','.','>','/','?','|'}}
    };


std::map<std::string, PrimitiveTypeEnum> primitives_map = {
    { "char", CHAR },
    { "string", STRING },
    { "nqstring", NQSTRING },
    { "unsigned int", UINT },
    { "int", INT},
    { "bool", BOOL },
};

std::map<std::string, std::string> api_ops_map = {
    { "ApiOp_Add", "+" },
    { "ApiOp_Sub", "-" },
    { "ApiOp_Div", "/" },
    { "ApiOp_Mul", "*" },
    { "ApiOp_Equals", "==" },
    { "ApiOp_LessThan", "<" },
    { "ApiOp_LessThanEquals", "<=" },
    { "ApiOp_GreaterThan", ">" },
    { "ApiOp_GreaterThanEquals", ">=" },
};

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

void
CHECK_CONDITION(bool condition, std::string message)
{
    if (!condition)
    {
        std::cout << "CHECK FAILED: " << message << std::endl;
        if (DEBUG)
        {
            assert(false);
        }
        exit(1);
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
        CHECK_CONDITION(obj != nullptr,
            fmt::format("Got null pointer argument when making argument string"));
        args_to_string.push_back(obj->toStr());
    }
    return getStringWithDelims(args_to_string, ',');
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

std::ostream&
operator<<(std::ostream& os, ApiType* type)
{
    os << type->toStr();
    return os;
}

/*******************************************************************************
 * ExplicitType functions
 ******************************************************************************/

ExplicitType::ExplicitType(std::string _definition, const ApiType* _underlying_type) :
    ApiType(_definition), definition(_definition), underlying_type(_underlying_type)
{
    size_t mid_one = _definition.find(delim_mid);
    size_t mid_two = _definition.find(delim_mid, mid_one + 1);
    CHECK_CONDITION(mid_two != std::string::npos,
        fmt::format("Expected second middle delimiter in comprehension `{}`.",
            _definition));
    size_t end = _definition.rfind(delim_back);
    this->gen_type = _definition.substr(1, mid_one - 1);
    this->gen_method = _definition.substr(mid_one + 1, mid_two - mid_one - 1);
    this->descriptor = _definition.substr(mid_two + 1, end - mid_two - 1);
}


//const ApiObject*
//ExplicitType::retrieveObj() const
//{
    //assert(this->definition.front() == delim_front &&
        //this->definition.back() == delim_back);
    //logDebug("Definition to retrieve: " + this->getDefinition());
    //if  (this->definition.find(fmt::format("string{}", delim_mid))
            //!= std::string::npos)
    //{
        //assert(this->getUnderlyingType()->isPrimitive());
        //assert(((PrimitiveType *) this->getUnderlyingType())->getTypeEnum()
            //== STRING);
        //std::string data = this->definition.substr(this->definition.find(delim_mid) + 1,
            //this->definition.find(delim_back) - this->definition.find(delim_mid) - 1);
        //return new PrimitiveObject<std::string>(
            //(PrimitiveType*) this->getUnderlyingType(), data);
    //}
    //else if (this->definition.find(fmt::format("uint{}", delim_mid))
            //!= std::string::npos)
    //{
        //assert(this->getUnderlyingType()->isPrimitive());
        //assert(((PrimitiveType *) this->getUnderlyingType())->getTypeEnum()
            //== UINT);
        //std::string data = this->definition.substr(this->definition.find(delim_mid) + 1,
            //this->definition.find(delim_back) - this->definition.find(delim_mid) - 1);
        //return new PrimitiveObject<unsigned int>(
            //(PrimitiveType*) this->getUnderlyingType(), std::stoi(data));
    //}
    //else if (this->definition.find(fmt::format("input{}", delim_mid))
            //!= std::string::npos)
    //{
        //// TODO
        //assert(false);
    //}
    //assert(false);
//}

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
 * ApiObject functions
 ******************************************************************************/

FuncObject::FuncObject(const ApiFunc* _func, const ApiObject* _target,
    std::vector<const ApiObject*> _params) :
    ApiObject(_func->getName(), -1, _func->getReturnType()), func(_func),
    target(_target), params(_params) {};

FuncObject*
FuncObject::concretizeVars(const ApiObject* curr_meta_variant,
    const std::vector<const ApiObject*>& meta_variant_vars,
    const std::vector<const ApiObject*>& input_vars) const
{
    // TODO add a makeConcrete function to ApiObject
    std::vector<const ApiObject*> concrete_params;
    std::transform(this->params.begin(), this->params.end(),
        std::back_inserter(concrete_params),
        [&](const ApiObject* param)
        {
            if (dynamic_cast<const MetaVarObject*>(param))
            {
                return dynamic_cast<const ApiObject*>(
                    dynamic_cast<const MetaVarObject*>(param)->
                        getConcreteVar(curr_meta_variant, meta_variant_vars,
                            input_vars));
            }
            if (dynamic_cast<const FuncObject*>(param))
            {
                return dynamic_cast<const ApiObject*>(
                    dynamic_cast<const FuncObject*>(param)->
                        concretizeVars(curr_meta_variant, meta_variant_vars,
                            input_vars));
            }
            return param;
        });
    const ApiObject* concrete_target = nullptr;
    if (target)
    {
        if (dynamic_cast<const MetaVarObject*>(target))
        {
            concrete_target = dynamic_cast<const MetaVarObject*>(target)->
                getConcreteVar(curr_meta_variant, meta_variant_vars,
                            input_vars);
        }
        else if (dynamic_cast<const FuncObject*>(target))
        {
            concrete_target =  dynamic_cast<const FuncObject*>(target)->
                concretizeVars(curr_meta_variant, meta_variant_vars,
                            input_vars);
        }
        else
        {
            concrete_target = target;
        }
    }
    return new FuncObject(this->func, concrete_target, concrete_params);
}

std::string
FuncObject::toStr() const
{
    std::vector<std::string> param_names;
    //for (const ApiObject* param : this->params)
    //{
        //param_names.push_back(param->toStr());
    //}
    std::transform(this->params.begin(), this->params.end(),
        std::back_inserter(param_names),
        [](const ApiObject* param) { return param->toStr(); });
    if (target)
    {
        return fmt::format("{}.{}({})", this->target->toStr(), this->name,
            getStringWithDelims(param_names, ','));
    }
    return fmt::format("{}({})", this->name, getStringWithDelims(param_names, ','));
}

const ApiObject*
MetaVarObject::getConcreteVar(const ApiObject* curr_meta_variant,
    const std::vector<const ApiObject*>& meta_variants,
    const std::vector<const ApiObject*>& input_vars) const
{
    if (this->meta_relations.empty())
    {
        if (!this->getIdentifier().compare("<m_curr>"))
        {
            return curr_meta_variant;
        }
        else if (this->getIdentifier().front() == 'm')
        {
            assert(std::isdigit(this->getIdentifier()[1]));
            size_t meta_variant_id = stoi(this->getIdentifier().substr(1));
            for (const ApiObject* mv : meta_variants)
            {
                if (mv->getID() == meta_variant_id)
                {
                    return mv;
                }
            }
            assert(false);
        }
        else if (std::isdigit(this->getIdentifier().front()))
        {
            size_t input_id = stoi(this->getIdentifier()) - 1;
            assert (input_id < input_vars.size());
            return input_vars.at(input_id);
        }
        assert(false);
    }
    return this->meta_relations.at(
        (*this->rng)() % this->meta_relations.size())
            ->getBaseFunc()->concretizeVars(curr_meta_variant, meta_variants,
                input_vars);
}

const MetaRelation*
MetaRelation::concretizeVars(const ApiObject* curr_meta_variant,
    const std::vector<const ApiObject*>& meta_variant_vars,
    const std::vector<const ApiObject*>& input_vars
    ) const
{
    const MetaVarObject* meta_store_var =
        dynamic_cast<const MetaVarObject*>(this->store_var);
    const ApiObject* concrete_store_var = nullptr;
    if (meta_store_var)
    {
        concrete_store_var =
            meta_store_var->getConcreteVar(curr_meta_variant, meta_variant_vars, input_vars);
    }
    const FuncObject* concrete_func_obj =
        this->getBaseFunc()->concretizeVars(curr_meta_variant, meta_variant_vars, input_vars);
    return new MetaRelation(this->getAbstractRelation(), concrete_func_obj,
        concrete_store_var);
}

const ApiInstruction*
MetaRelation::toApiInstruction() const
{
    const ApiFunc* func = this->getBaseFunc()->getFunc();
    const std::vector<const ApiObject*> params = this->getBaseFunc()->getParams();
    const ApiObject* target_obj = this->getBaseFunc()->getTarget();
    const ApiObject* result_obj = this->getStoreVar();
    return new ApiInstruction(func, result_obj, target_obj, params);
}

std::string
MetaRelation::toStr() const
{
    std::stringstream mr_ss;
    if (store_var)
    {
        mr_ss << store_var->toStr() << " = ";
    }
    mr_ss << base_func->toStr() << ";";
    return mr_ss.str();
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
        if (param_types_check.at(i)->isExplicit())
        {
            if (dynamic_cast<const ExplicitType*>(param_types_check.at(i))->
                    getUnderlyingType()->isType(this->getParamType(i)))
            {
                return false;
            }
        }
        else if (!param_types_check.at(i)->isType(this->getParamType(i)))
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
        CHECK_CONDITION(args_check.at(i) != nullptr,
            fmt::format("Given argument {} to check for function `{}` is null.",
                i, this->name));
        CHECK_CONDITION(args_check.at(i)->getType() != nullptr,
            fmt::format("Given argument named `{}` to check for function `{}` "
                        "has null type.",
                args_check.at(i)->toStr(), this->name));
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
    if (this->getClassType()->toStr() != "")
    {
        print_ss << this->getClassType()->toStr() << ".";
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
 * ApiExpr functions
 ******************************************************************************/

std::string
BinaryExpr::toStr() const
{
    return fmt::format("{} {} {}", this->lhs->toStr(),
        api_ops_map[this->op_name], this->rhs->toStr());
}

std::string
UnaryExpr::toStr() const
{
    return this->obj->toStr();
}


/*******************************************************************************
 * ApiInstruction functions
 ******************************************************************************/

std::string
ApiInstruction::toStr() const
{
    std::stringstream instr_ss;
    if (!this->getFunc()->checkArgs(this->getFuncParams()))
    {
        const ApiFunc* func = this->getFunc();
        std::cout << "Invalid arguments given for func " << func->getName();
        std::cout << std::endl << "\tExpected types: " << makeArgString(
            func->getParamTypes());
        std::vector<std::string> arg_strings;
        for (const ApiObject* func_param : this->getFuncParams())
        {
            //arg_strings.push_back(func_param->toStrWithType());
            arg_strings.push_back(func_param->toStr());
        }
        std::cout << std::endl << "\tGiven types: ";
        std::cout << getStringWithDelims(arg_strings, ',') << std::endl;
        assert(false);
    }
    if (this->getResultObj() != nullptr)
    {
        if (!this->getResultObj()->isDeclared())
        {
            instr_ss << result_obj->toStrWithType();
            result_obj->declared = true;
        }
        else
        {
            instr_ss << result_obj->toStr();
        }
        instr_ss << " = ";
    }
    else
    {
        assert(!this->getFunc()->checkFlag("ctor"));
    }
    if (!this->getFunc()->getConditions().empty())
    {
        assert(false);
        //instr_ss << this->emitFuncCond(this->getFunc, this->getTargetObj,
            //this->getFuncParams());
    }
    if (!this->getFunc()->checkFlag("statik") && target_obj != nullptr)
    {
        // TODO consider pointer objects
        std::string invocation_string =
            this->getTargetObj()->getType()->checkFlag("pointer")
            ? "->"
            : ".";
        instr_ss << this->getTargetObj()->toStr() << invocation_string;
    }
    else if (this->getFunc()->checkFlag("statik"))
    {
        CHECK_CONDITION(this->getTargetObj() == nullptr,
            fmt::format("Called static function `{}` on enclosing class instance.",
                this->getFunc()->getName()));
        instr_ss << this->getFunc()->getClassType()->toStr() << "::";
    }
    instr_ss << this->getFunc()->printInvocation(this->getFuncParams());
    //if (!this->getFunc()->getConditions().empty())
    //{
        //apply_func_ss << " : " << this->generateObject(func->getReturnType())->toStr();
    //}
    instr_ss << ";";
    return instr_ss.str();
}

std::string
ObjectDeclInstruction::toStr() const
{
    return this->getObject()->toStrWithType() + ";";
}
