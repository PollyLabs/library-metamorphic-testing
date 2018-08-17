#include "api_elements.hpp"

std::map<std::string, PrimitiveTypeEnum> primitives_map = {
    { "string", STRING },
    { "unsigned int", UINT },
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
        instr_ss << this->getTargetObj()->toStr() << ".";
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

