#include "api_elements.hpp"

std::map<std::string, std::vector<char>> char_set =
    {
     {"numeric", {'0','1','2','3','4','5','6','7','8','9'}},
     {"low_alpha", {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
                    'p','q','r','s','t','u','v','w','x','y','z'}},
     {"up_alpha", {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
                   'P','Q','R','S','T','U','V','W','X','Y','Z'}},
     //{"symbol", {'`','!','$','%','^','&','*','(',')','_','+','{','}','[',']',
                 //';',':','\'','@','#','~',',','<','.','>','/','?','|'}}
    };


std::map<std::string, PrimitiveTypeEnum> primitives_map = {
    { "char", CHAR },
    { "string", STRING },
    { "std::string", STRING },
    { "nqstring", NQSTRING },
    { "unsigned int", UINT },
    { "int", INT },
    { "int32_t", LONG },
    { "uint32_t", UINT },
    { "size_t", UINT },
    { "int64_t", DOUBLE },
    { "long", LONG },
    { "double", DOUBLE },
    { "float", FLOAT },
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
    ApiType(_definition), underlying_type(_underlying_type),
    definition(_definition)
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
 * TemplateType functions
 ******************************************************************************/

TemplateType::TemplateType(std::string _name,
    size_t _template_count) : ApiType(_name), template_count(_template_count)
{
    //std::string template_str = std::accumulate(std::begin(_template_list),
        //std::end(_template_list),
        //std::string(), [](std::string acc, const ApiType* template_type)
        //{
            //return acc + ',' + template_type->toStr();
        //});
    //this->name = fmt::format("{}<{}>", _base_type->toStr(), template_str);
}

/*******************************************************************************
 * ApiObject functions
 ******************************************************************************/

ApiObject::ApiObject(std::string _name, size_t _id, const ApiType* _type,
    bool _initialize) :
    id(_id), name(_name), type(_type), initialize(_initialize), declared(false)
{
    if (_type && _type->isEnum())
    {
        CHECK_CONDITION(dynamic_cast<const EnumType*>(_type)->checkValue(_name),
            fmt::format("Invalid enum value {} for type {}.",
                _name, _type->toStr()));
    }
}

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

/**
* @brief Returns all ApiObject references from current FuncObject
*
* Iterates over all object fields of current FuncObject and gathers valid
* ApiObject pointers; if any such field points to another FuncObject,
* recursively calls `getAllObjs` over the respective pointers.
*
* @return A vector containing all ApiObject pointers of this and any children
* FuncObjects
*/

std::vector<const ApiObject*>
FuncObject::getAllObjs(void) const
{
    std::vector<const ApiObject*> all_objs;
    if (this->target)
    {
        std::vector<const ApiObject*> target_objs = target->getAllObjs();
        all_objs.insert(all_objs.end(), target_objs.begin(), target_objs.end());
    }
    std::for_each(this->params.begin(), this->params.end(),
        [&all_objs](const ApiObject* param)
        {
            std::vector<const ApiObject*> param_objs = param->getAllObjs();
            all_objs.insert(all_objs.end(), param_objs.begin(), param_objs.end());
        });
    return all_objs;
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

/*******************************************************************************
 * TemplateObject functions
 ******************************************************************************/

TemplateObject::TemplateObject(std::string _name, size_t _id,
    const TemplateInstance _ti): ApiObject(_name, _id, _ti.getBaseType()),
        template_instance(_ti) {};

//std::string
//templateObject::toStr() const
//{
    //std::string type_name = fmt::format("{}<{}>", this->getType()->toStr(),
        //std::accumulate(std::begin(this->template_types),
            //std::end(this->template_list), std::string(),
            //[](std::string acc, const ApiType* template_type)
            //{
                //return acc + ',' + template_type->toStr();
            //}));
    //return fmt::format("{} {}", type_name, this->name);
//}

/*******************************************************************************
 * MetaVarObject functions
 ******************************************************************************/

bool
MetaVarObject::isInput(void) const
{
    size_t pos;
    std::stoul(this->getIdentifier(), &pos);
    return pos == this->getIdentifier().size();
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

/**
* @brief Checks whether the given arguments are compatible with the declared
* function signature
*
* Ensures that the given arguments match with those in the function declaration,
* both in terms of number of provided arguments and in terms of individual types
* for each argument. Checks that given arguments are not null pointers, which
* might happend due to issues further above the generation chain.
*
* @param args_check A vector containing ApiObjects to be checked against the
* provided function signature
*
* @return `True` is all arguments given are compatible; `False` otherwise
*/
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

/**
* @brief Returns the value for the given flag string
*
* Looks inside the `flags` map and returns the appropriate bool value for the
* corresponding `flag` key. The given parameter can be transformed by the
* following operators:
* * '!' - negates the returned value of the flag
*
* @param flag A string representing a flag value to be returned
*
* @return The corresponding value from the `flags` map
*/
bool
ApiFunc::checkFlag(std::string flag) const
{
    bool negative = flag.front() == '!';
    if (negative)
    {
        flag.erase(flag.begin());
    }
    assert(flags.count(flag) != 0);
    //CHECK_CONDITION(flags.count(flag) != 0,
        //fmt::format("Could not find flag {} for ApiFunc {}.",
            //flag, this->name));
    return negative ^ this->flags.find(flag)->second;
}

/**
 * @brief Checks whether a function is callable given existing ApiObjects
 *
 * In order for a function to be callable, there must exist objects of required
 * types to pass as parameters or as a required object instance. If such objects
 * do exist in the provided `obj_list`, then this function yields `true`.
 *
 * @param obj_list List of available objects to use by a potential function call
 *
 * @return Whether there are sufficient objects to construct a function call
 */
bool
ApiFunc::isCallable(std::pair<ApiObject_c, ApiFunc_c> gen_data) const
{
    logDebug(fmt::format("Checking if {} is callable.", this->name));
    ApiObject_c obj_list = gen_data.first;
    ApiFunc_c func_list = gen_data.second;
    if (obj_list.empty() && func_list.empty())
    {
        return false;
    }

    if (this->enclosing_class)
    {
        ApiObject_c::iterator obj_it =
            std::find_if(obj_list.begin(), obj_list.end(),
            [this](const ApiObject* obj)
            {
                return obj->hasType(this->getClassType());
            });
        ApiFunc_c::iterator func_it =
            std::find_if(func_list.begin(), func_list.end(),
            [this](const ApiFunc* func)
            {
                return func->checkFlag("ctor") &&
                    func->hasReturnType(this->getClassType());
            });
        if ((obj_list.empty() || obj_it == obj_list.end()) &&
                (func_list.empty() || func_it == func_list.end()))
        {
            logDebug(fmt::format("Could not find existing object or constructor "
                "of type {} for instance of func {}.",
                this->enclosing_class->toStr(), this->name));
            return false;
        }
    }
    for (const ApiType* param_type : this->param_types)
    {
        ApiObject_c::iterator obj_it =
            std::find_if(obj_list.begin(), obj_list.end(),
            [param_type](const ApiObject* obj)
            {
                return obj->hasType(param_type);
            });
        ApiFunc_c::iterator func_it =
            std::find_if(func_list.begin(), func_list.end(),
            [param_type](const ApiFunc* func)
            {
                return func->checkFlag("ctor") &&
                    func->hasReturnType(param_type);
            });
        if ((obj_list.empty() || obj_it == obj_list.end()) &&
                (func_list.empty() || func_it == func_list.end()))
        {
            logDebug(fmt::format("Could not find existing object or "
                " constructor of type {} for param of func {}.",
                param_type->toStr(), this->name));
            return false;
        }
    }
    logDebug(fmt::format("{} found callable.", this->name));
    return true;
}

//bool
//ApiFunc::isConstructorCallable(
    //std::set<const ApiFunc*, decltype(&ApiFunc::pointerCmp)> func_list) const
//{
    //logDebug(fmt::format("Checking if {} is constructor callable.", this->name));
    //if (func_list.empty())
    //{
        //return false;
    //}
    //if (this->enclosing_class)
    //{
        //std::set<const ApiFunc*, decltype(&ApiType::pointerCmp)>::iterator it =
            //std::find_if(func_list.begin(), func_list.end(),
            //[this](const ApiFunc* af)
            //{
                //return af->checkFlag("ctor") &&
                    //af->hasReturnType(this->getClassType());
            //});
        //if (it == func_list.end())
        //{
            //logDebug(fmt::format("Could not find existing ctor func of type {} "
                //"for instance of func {}.", this->enclosing_class->toStr(),
                //this->name));
            //return false;
        //}
    //}
    //for (const ApiType* param_type : this->param_types)
    //{
        //std::set<const ApiFunc*, decltype(&ApiType::pointerCmp)>::iterator it =
            //std::find_if(func_list.begin(), func_list.end(),
            //[&param_type](const ApiFunc* af)
            //{
                //return af->checkFlag("ctor") && af->hasReturnType(param_type);
            //});
        //if (it == func_list.end())
        //{
            //logDebug(fmt::format("Could not find existing ctor func of type {} "
                //"for param of func {}.", param_type->toStr(), this->name));
            //return false;
        //}
    //}
    //logDebug(fmt::format("{} found callable.", this->name));
    //return true;
//}

std::string
ApiFunc::printSignature() const
{
    std::stringstream print_ss;
    //if (this->getClassType()->toStr() != "")
    if (this->getClassType())
    {
        print_ss << this->getClassType()->toStr() << ".";
    }
    print_ss << this->getName() << "(";
    print_ss << makeArgString(this->getParamTypes()) << ")";
    return print_ss.str();
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
ApiInstruction::ApiInstruction(const ApiFunc* _func, const ApiObject* _result,
    const ApiObject* _target, std::vector<const ApiObject*> _params,
    bool _decl_instr) : func(_func), func_elems{_result, _target, _params}
{
    if (func_elems.result)
    {
        decl_instr = !func_elems.result->isDeclared();
        func_elems.result->setDeclared();
    }
}

/**
* @brief Returns the string representation of an ApiInstruction
*
* Appropriately transforms each member of an ApiInstruction via `toStr()` calls
* to each object, and collates them together in a C++ syntax. In essence, the
* `target` and `result` fields are directly translated into strings, while an
* invocation of `func` is constructed by applying it to `func_params`.
*
* @return A string representation of all internal constructs.
*/
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
        if (this->isDeclInstr())
        {
            instr_ss << this->getResultObj()->toStrWithType();
        }
        else
        {
            instr_ss << this->getResultObj()->toStr();
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
    if (!this->getFunc()->checkFlag("statik") && this->getTargetObj() != nullptr)
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
    instr_ss << this->printFuncInvocation(this->getFunc(), this->getFuncParams());
    instr_ss << ";";
    return instr_ss.str();
}

/**
* @brief Helper function to print the invocation of a ApiFunc
*
* Fully prints an ApiFunc invocation for the given parameters, in the form
* `name(params...). Appropriate checks are performed by the
* ApiInstruction::toStr() caller.
*
* @param func A pointer to the ApiFunc to be invoked
* @param params A vector containing the parameter to pass to the function
* invocation
*
* @return A string representing the invocation in plain text.
*/
std::string
ApiInstruction::printFuncInvocation(const ApiFunc* func,
    std::vector<const ApiObject*> params) const
{
    std::stringstream print_inv_ss;
    print_inv_ss << func->getName() << "(";
    print_inv_ss << makeArgString(params) << ")";
    return print_inv_ss.str();
}

std::string
ObjectDeclInstruction::toStr() const
{
    return this->getObject()->toStrWithType() + ";";
}

std::string
ObjectConstructionInstruction::toStr() const
{
    // TODO move this check somewhere else
    CHECK_CONDITION(this->ctor->checkArgs(this->params),
        fmt::format("Invalid arguments given to constructor `{}` for object `{}`.",
            this->ctor->getName(), this->getObject()->toStr()));
    std::stringstream obj_ctor_instr_ss;
    obj_ctor_instr_ss << this->getObject()->toStrWithType();
    obj_ctor_instr_ss << this->ctor->getName() << "(";
    obj_ctor_instr_ss << makeArgString(this->params) << ");";
    return obj_ctor_instr_ss.str();
}
