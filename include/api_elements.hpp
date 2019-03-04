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
enum ApiTypeFlag {
    POINTER,
    SINGLETON,
};

enum ApiFuncFlags {
};

enum PrimitiveTypeEnum {
    CHAR,
    CHAR_SYMBOL,
    NQSTRING,
    STRING,
    UINT,
    INT,
    BOOL,
};

extern std::map<std::string, std::vector<char>> char_set;
extern std::map<std::string, PrimitiveTypeEnum> primitives_map;
extern char delim_front, delim_back, delim_mid;
extern const bool DEBUG;

void logDebug(std::string);
void CHECK_CONDITION(bool, std::string);
std::string getStringWithDelims(std::vector<std::string>, char);
template<typename T> std::string makeArgString(std::vector<T>);

class ApiObject;
class ApiFunc;
class MetaRelation;
class MetaVarObject;

class ApiType {
    const std::string name;
    std::map<std::string, bool> flags;

    public:
        ApiType(std::string _name) : ApiType(_name, false, false) {};
        ApiType(std::string _name, bool _pointer, bool _singleton) : name(_name)
            {
                assert (!_name.empty());
                this->flags.insert({"pointer", _pointer});
                this->flags.insert({"singleton", _singleton});
            };
        virtual ~ApiType() = default;

        bool hasName(std::string name_check) const {
            return !this->toStr().compare(name_check);
        };
        virtual bool isType(const ApiType*) const;

        bool checkFlag(std::string flag) const
        {
            bool negative = flag.front() == '!';
            if (negative)
            {
                flag.erase(flag.begin());
            }
            assert(flags.count(flag) != 0);
            return negative ^ this->flags.find(flag)->second;
        }

        //virtual bool isSingleton() const { return false; };
        virtual bool isPrimitive() const { return false; };
        virtual bool isExplicit() const { return false; };

        virtual const ApiType* getUnderlyingType() const { return this; };
        virtual const PrimitiveTypeEnum getTypeEnum() const { assert(false); };

        inline bool operator<(const ApiType* other) const {
            return this->toStr() < other->toStr();
        };

        std::string toStr() const { return this->name; };
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
    std::string gen_type;
    std::string gen_method;
    std::string descriptor;

    public:
        ExplicitType(std::string, const ApiType*);

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
        std::string getGenType() const { return this->gen_type; };
        std::string getGenMethod() const { return this->gen_method; };
        std::string getDescriptor() const { return this->descriptor; };
        const ApiType* getUnderlyingType() const {
            return this->underlying_type;
        };

        static std::string extractExplicitTypeDecl(std::string);
        //const ApiObject* retrieveObj() const;
};

class ApiObject {
    protected:
        const size_t id;
        const std::string name;
        const ApiType* type;

    public:
        mutable bool declared;

        ApiObject(std::string _name, size_t _id, const ApiType* _type) :
            id(_id), name(_name), type(_type), declared(false) {};
        virtual ~ApiObject() = default;

        inline const ApiType* getType() const { return this->type; };
        inline size_t getID() const { return this->id; };

        inline void setDeclared() const { this->declared = true; };

        inline bool isPrimitive() const { return this->getType()->isPrimitive(); };
        inline bool notIsPrimitive() const { return !this->getType()->isPrimitive(); };
        inline bool isDeclared() const { return this->declared; };
        inline bool hasName(std::string name_check) const {
            return !this->name.compare(name_check);
        };
        inline bool hasType(const ApiType* type_check) const {
            return this->getType()->isType(type_check);
        };
        inline bool hasID(size_t id_check) const {
            return this->getID() == id_check;
        };

        inline virtual std::string toStr() const {
            return fmt::format("{}_{}", this->name, std::to_string(this->id));
        };
        inline virtual std::string toStrWithType() const {
            return fmt::format("{} {}", this->getType()->toStr(),
                this->toStr());
        };
};

template<typename T>
class PrimitiveObject : public ApiObject {
    T data;

    public:
        PrimitiveObject(const PrimitiveType* _type, T _data, std::string _name,
            size_t _id) :
            ApiObject(_name, _id, _type), data(_data) {};
        PrimitiveObject(const PrimitiveType* _type, T _data, size_t _id) :
            PrimitiveObject(_type, _data, _type->toStr(), _id) {};

        T getData() const { return this->data; };

        std::string toStr() const;
        std::string toStrWithType() const { assert(false); };
};

class NamedObject : public ApiObject {
    public:
        NamedObject(std::string _name, size_t _id, const ApiType* _type) :
            ApiObject(_name, _id, _type) {};

        std::string toStr() const { return this->name; };
        std::string toStrWithType() const { return fmt::format("{} {}",
            this->getType()->getUnderlyingType()->toStr(), this->toStr()); };
};

class ExprObject : public ApiObject {
    public:
        ExprObject(std::string _expr, size_t _id, const PrimitiveType* _type) :
            ApiObject(_expr, _id, _type) {};

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
    std::map<std::string, bool> flags;

    public:
        ApiFunc(std::string _name, const ApiType* _member_type,
            const ApiType* _return_type,
            std::vector<const ApiType*> _param_types,
            std::vector<std::string> _conditions, bool _special = false,
            bool _statik = false, bool _ctor = false, bool _max_depth = false) :
            name(_name), member_type(_member_type), return_type(_return_type),
            param_types(_param_types), conditions(_conditions)
            {
                this->flags.insert({"special", _special});
                this->flags.insert({"statik", _statik});
                this->flags.insert({"ctor", _ctor});
                this->flags.insert({"max_depth", _max_depth});
            }
        virtual ~ApiFunc() = default;

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
        bool checkFlag(std::string flag) const
        {
            bool negative = flag.front() == '!';
            if (negative)
            {
                flag.erase(flag.begin());
            }
            assert(flags.count(flag) != 0);
            return negative ^ this->flags.find(flag)->second;
        }

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
        virtual ~ApiInstructionInterface() = default;
};

class ApiInstruction : public ApiInstructionInterface
{
    const ApiFunc* func;
    const ApiObject* target_obj;
    const ApiObject* result_obj;
    std::vector<const ApiObject*> func_params;

    public:
        ApiInstruction(const ApiFunc* _func, const ApiObject* _result = nullptr,
            const ApiObject* _target = nullptr,
            std::vector<const ApiObject*> _params =
            std::vector<const ApiObject*>()) :
            func(_func), target_obj(_target), result_obj(_result),
            func_params(_params) {};

        const ApiFunc* getFunc() const { return this->func; }
        const ApiObject* getTargetObj() const { return this->target_obj; };
        const ApiObject* getResultObj() const { return this->result_obj; };
        std::vector<const ApiObject*> getFuncParams() const
        {
            return this->func_params;
        };

        std::string toStr() const;
};

class ObjectDeclInstruction : public ApiInstructionInterface
{
    private:
        const ApiObject* obj;

    public:
        ObjectDeclInstruction(const ApiObject* _obj) : obj(_obj) {};

        const ApiObject* getObject() const { return this->obj; };
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
        const ApiInstruction* toApiInstruction() const;

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

#include "api_elements.tpp"

#endif
