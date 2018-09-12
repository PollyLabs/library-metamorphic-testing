#include "api_elements.hpp"

std::map<std::string, PrimitiveTypeEnum> primitives_map = {
    { "string", STRING },
    { "unsigned int", UINT },
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
 * ExplicitType functions
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
MetaRelation::toApiInstruction(bool new_obj_decl) const
{
    const ApiFunc* func = this->getBaseFunc()->getFunc();
    const std::vector<const ApiObject*> params = this->getBaseFunc()->getParams();
    const ApiObject* target_obj = this->getBaseFunc()->getTarget();
    const ApiObject* result_obj = this->getStoreVar();
    return new ApiInstruction(func, result_obj, target_obj, params, new_obj_decl);
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
            arg_strings.push_back(func_param->toStrWithType());
        }
        std::cout << std::endl << "\tGiven types: ";
        std::cout << getStringWithDelims(arg_strings, ',') << std::endl;
        exit(1);
    }
    if (this->getResultObj() != nullptr)
    {
        if (this->isNewObjDecl() || this->getFunc()->isCtor())
        {
            instr_ss << result_obj->toStrWithType();
        }
        else
        {
            instr_ss << result_obj->toStr();
        }
        instr_ss << " = ";
    }
    else
    {
        assert(!this->getFunc()->isCtor());
    }
    if (!this->getFunc()->getConditions().empty())
    {
        assert(false);
        //instr_ss << this->emitFuncCond(this->getFunc, this->getTargetObj,
            //this->getFuncParams());
    }
    if (!this->getFunc()->isStatic() && target_obj != nullptr)
    {
        // TODO consider pointer objects
        std::string invocation_string =
            this->getTargetObj()->getType()->toStr().back() == '*'
            ? "->"
            : ".";
        instr_ss << this->getTargetObj()->toStr() << invocation_string;
    }
    else if (this->getFunc()->isStatic())
    {
        assert(this->getTargetObj() == nullptr);
        instr_ss << this->getFunc()->getMemberType()->toStr() << "::";
    }
    instr_ss << this->getFunc()->printInvocation(this->getFuncParams());
    //if (!this->getFunc()->getConditions().empty())
    //{
        //apply_func_ss << " : " << this->generateObject(func->getReturnType())->toStr();
    //}
    instr_ss << ";";
    return instr_ss.str();
}

