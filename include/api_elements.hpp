#ifndef API_ELEMENTS_HPP
#define API_ELEMENTS_HPP

#include <string>
#include <cassert>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <vector>

#include "fmt/format.h"


// TODO move to api_elements
enum PrimitiveTypeEnum {
    STRING,
    UINT,
    BOOL,
};

extern std::map<std::string, PrimitiveTypeEnum> primitives_map;
extern char delim_front, delim_back, delim_mid;
extern const bool DEBUG;

void logDebug(std::string);
std::string getStringWithDelims(std::vector<std::string>, char);
template<typename T> std::string makeArgString(std::vector<T>);

class ApiObject;
class ApiFunc;
class MetaRelation;
class MetaVarObject;

class ApiType {
    const std::string name;

    public:
        ApiType(std::string _name) : name(_name) { assert (!_name.empty()); };

        std::string toStr() const { return this->name; };

        bool hasName(std::string name_check) const {
            return !this->toStr().compare(name_check);
        };
        virtual bool isType(const ApiType*) const;

        virtual bool isSingleton() const { return false; };
        virtual bool isPrimitive() const { return false; };
        virtual bool isExplicit() const { return false; };

        inline bool operator<(const ApiType* other) const {
            return this->toStr() < other->toStr();
        };
};

class SingletonType : public ApiType {
    public:
        SingletonType(std::string _name) : ApiType(_name) {};

        bool isSingleton() const { return true; };
};

class PrimitiveType : public ApiType {
    const PrimitiveTypeEnum type_enum;
    std::string range;

    public:
        PrimitiveType(std::string _name) : ApiType(_name),
            type_enum(primitives_map[_name]), range("") {};

        bool isPrimitive() const { return true; };
        const PrimitiveTypeEnum getTypeEnum() const { return this->type_enum; };
};

class ExplicitType : public ApiType {
    const ApiType* underlying_type;
    const std::string definition;

    public:
        ExplicitType(std::string _definition, const ApiType* _underlying_type) :
            ApiType(_definition), definition(_definition),
            underlying_type(_underlying_type) {};

        bool isType(const ApiType* other) const {
            return !this->underlying_type->toStr().compare(other->toStr());
        }
        bool isExplicit() const { return true; };
        bool isInput() const {
            return this->definition.find(fmt::format("input{}", delim_mid))
                != std::string::npos;
        };
        bool isExpr() const {
            return this->definition.find(fmt::format("expr{}", delim_mid))
                != std::string::npos;
        };
        bool isRange() const {
            return this->definition.find(fmt::format("range{}", delim_mid))
                != std::string::npos;
        };

        std::string getDefinition() const { return this->definition; };
        const ApiType* getUnderlyingType() const {
            return this->underlying_type;
        };

        static std::string extractExplicitTypeDecl(std::string);
        const ApiObject* retrieveObj() const;
};

class ApiObject {
    protected:
        const size_t id;
        const std::string name;
        const ApiType* type;

    public:
        ApiObject(std::string _name, unsigned int _id, const ApiType* _type) :
            id(_id), name(_name), type(_type) {};

        const ApiType* getType() const { return this->type; };
        size_t getID() const { return this->id; };

        bool isPrimitive() const { return this->getType()->isPrimitive(); };
        bool hasName(std::string name_check) const {
            return !this->name.compare(name_check);
        };
        bool hasType(const ApiType* type_check) const {
            return this->getType()->isType(type_check);
        };

        virtual std::string toStr() const {
            return fmt::format("{}_{}", this->name, std::to_string(this->id));
        };
        virtual std::string toStrWithType() const {
            return fmt::format("{} {}", this->getType()->toStr(),
                this->toStr());
        };
};

template<class T>
class PrimitiveObject : public ApiObject {
    T data;

    public:
        PrimitiveObject(const PrimitiveType* _type, T _data) : ApiObject(_type->toStr(),
            -1, _type), data(_data) {};

        T getData() const { return this->data; };

        std::string toStr() const { return fmt::format("{}", this->data); };
        std::string toStrWithType() const { assert(false); };
};

class NamedObject : public ApiObject {
    public:
        NamedObject(std::string _name, const ApiType* _type) :
            ApiObject(_name, -1, _type) {};

        std::string toStr() const { return this->name; }
};

class ExprObject : public ApiObject {
    public:
        ExprObject(std::string _expr, const PrimitiveType* _type) :
            ApiObject(_expr, -1, _type) {};

        std::string toStr() const { return this->name; };
        std::string toStrWithType() const { assert(false); };
};

class FuncObject : public ApiObject {
    const ApiFunc* func;
    const ApiObject* target = nullptr;
    std::vector<const ApiObject*> params;

    public:
        FuncObject(const ApiFunc*, const ApiObject*, std::vector<const ApiObject*>);

        FuncObject* concretizeVars(const ApiObject*,
            const std::vector<const ApiObject*>&,
            const std::vector<const ApiObject*>&) const;

        const ApiFunc* getFunc() const { return this->func; };
        std::vector<const ApiObject*> getParams() const { return this->params; };
        const ApiObject* getTarget() const { return this->target; };

        std::string toStr() const;
        std::string toStrWithType() const { assert(false); };
};


class ApiFunc {
    const std::string name;
    const ApiType* member_type;
    const ApiType* return_type;
    const std::vector<const ApiType*> param_types;
    const std::vector<std::string> conditions;
    const bool special;
    const bool statik;
    const bool ctor;

    public:
        ApiFunc(std::string _name, const ApiType* _member_type, const ApiType* _return_type,
            std::vector<const ApiType*> _param_types,
            std::vector<std::string> _conditions, bool _special = false,
            bool _statik = false, bool _ctor = false) :
            name(_name), member_type(_member_type), return_type(_return_type),
            param_types(_param_types), conditions(_conditions),
            special(_special), statik(_statik), ctor(_ctor) {};

        std::string getName() const { return this->name; };
        std::vector<const ApiType*> getParamTypes() const {
            return this->param_types;
        };
        const ApiType* getParamType(const unsigned int) const;
        unsigned int getParamCount() const { return this->param_types.size(); };
        const ApiType* getMemberType() const { return this->member_type; };
        const ApiType* getReturnType() const { return this->return_type; };
        std::vector<std::string> getConditions() const
        {
            return this->conditions;
        };

        bool hasMemberType(const ApiType* member_check) const {
            return this->getMemberType() != nullptr &&
                this->getMemberType()->isType(member_check);
        };
        bool hasReturnType(const ApiType* return_check) const {
            return this->getReturnType() != nullptr &&
                this->getReturnType()->isType(return_check);
        };
        bool hasName(std::string name_check) const {
            return !this->getName().compare(name_check);
        };
        bool hasParamTypes(std::vector<const ApiType*>) const;
        bool isSpecial() const { return this->special; };
        bool notIsSpecial() const { return !this->special; };
        bool isStatic() const { return this->statik; };
        bool isCtor() const { return this->ctor; };
        bool notIsCtor() const { return !this->ctor; };

        bool checkArgs(std::vector<const ApiObject*>) const;
        std::string printSignature() const;
        std::string printInvocation(std::vector<const ApiObject*>) const;

        inline bool operator<(const ApiFunc& other) const {
            return this->printSignature() < other.printSignature();
        }
};

class ApiExpr
{
    public:
        virtual std::string toStr() const = 0;
};

class UnaryExpr : ApiExpr
{
    const ApiObject* obj;

    public:
        std::string toStr() const;
};

class BinaryExpr : ApiExpr
{
    const ApiExpr* lhs;
    const ApiExpr* rhs;
    std::string op_name;

    public:
        std::string toStr() const;
};

class ApiInstructionInterface
{
    public:
        virtual std::string toStr() const = 0;
};

class ApiInstruction : public ApiInstructionInterface
{
    const ApiFunc* func;
    const ApiObject* target_obj;
    const ApiObject* result_obj;
    std::vector<const ApiObject*> func_params;
    const bool new_obj_decl;

    public:
        ApiInstruction(const ApiFunc* _func, const ApiObject* _result = nullptr,
            const ApiObject* _target = nullptr,
            std::vector<const ApiObject*> _params =
            std::vector<const ApiObject*>(), bool _new_obj_decl = false) :
            func(_func), target_obj(_target), result_obj(_result),
            func_params(_params), new_obj_decl(_new_obj_decl) {};

        const ApiFunc* getFunc() const { return this->func; }
        const ApiObject* getTargetObj() const { return this->target_obj; };
        const ApiObject* getResultObj() const { return this->result_obj; };
        std::vector<const ApiObject*> getFuncParams() const
        {
            return this->func_params;
        };

        bool isNewObjDecl() const { return this->new_obj_decl; };

        std::string toStr() const;
};

class ApiComment : public ApiInstructionInterface
{
    private:
        const std::string comment_string;

    public:
        ApiComment(std::string _comment_string) :
            comment_string(_comment_string) {};

        std::string toStr() const { return fmt::format("// {}", this->comment_string); };
};

class MetaRelation {
    protected:
        const std::string abstract_relation;
        const FuncObject* base_func;
        const ApiObject* store_var;

    public:
        MetaRelation(std::string _abstract_relation, const FuncObject* _base_func,
            const ApiObject* _store_var):
            abstract_relation(_abstract_relation), base_func(_base_func),
            store_var(_store_var) {};

        const MetaRelation* concretizeVars(const ApiObject*,
            const std::vector<const ApiObject*>&,
            const std::vector<const ApiObject*>&) const;
        const ApiInstruction* toApiInstruction(bool = false) const;

        std::string getAbstractRelation() const { return this->abstract_relation; }
        const FuncObject* getBaseFunc() const { return this->base_func; }
        const ApiObject* getStoreVar() const { return this->store_var; }

        std::string toStr() const;
};

class MetaVarObject : public ApiObject {
    const std::string identifier;
    const ApiType* type;
    std::vector<const MetaRelation*> meta_relations;
    std::mt19937* rng;

    public:
        MetaVarObject(std::string _identifier, const ApiType* _type,
            std::mt19937* _rng) :
            ApiObject(_identifier, -1, _type), identifier(_identifier),
            rng(_rng) {};

        void addRelation(const MetaRelation* rel)
        {
            this->meta_relations.push_back(rel);
        };

        const ApiObject* getConcreteVar(const ApiObject*,
            const std::vector<const ApiObject*>&,
            const std::vector<const ApiObject*>&) const;

        std::string getIdentifier() const { return this->identifier; };
        std::string toStr() const { assert(false); };
        std::string toStrWithType() const { assert(false); };
};

#endif
