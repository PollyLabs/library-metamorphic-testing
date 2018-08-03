#ifndef API_FUZZER_HPP
#define API_FUZZER_HPP

#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <random>
#include <string>
#include <sstream>
#include <set>
#include <tuple>
#include <vector>

#include "isl-noexceptions.h"
#include "fmt/format.h"
#include "yaml-cpp/yaml.h"

enum ApiTarget {
    ISL,
};

enum PrimitiveTypeEnum {
    STRING,
    UINT,
};

extern std::map<std::string, PrimitiveTypeEnum> primitives_map;
extern char delim_front, delim_back, delim_mid;

class ApiType;
class ExplicitType;
class ApiFunc;
class ApiObject;
class ApiInstruction;

std::string getStringWithDelims(std::vector<std::string>, char);
template<typename T> std::string makeArgString(std::vector<T>);
template<typename T> T getRandomVectorElem(std::vector<T>&);
template<typename T> T getRandomSetElem(std::set<T>&);
template<typename T> std::vector<const ApiObject*> filterObjList
    (std::vector<const ApiObject*>, bool (ApiObject::*)(T) const, T);
template<typename T> std::set<const ApiFunc*> filterFuncList(
    std::set<const ApiFunc*>, bool (ApiFunc::*)(T) const, T);

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

        std::string getDefinition() const { return this->definition; };
        const ApiType* getUnderlyingType() const {
            return this->underlying_type;
        };

        static std::string extractExplicitTypeDecl(std::string);
        const ApiObject* retrieveObj() const;
};

class ApiObject {
    protected:
        const unsigned int id;
        const std::string name;
        const ApiType* type;

    public:
        ApiObject(std::string _name, unsigned int _id, const ApiType* _type) :
            id(_id), name(_name), type(_type) {};

        const ApiType* getType() const { return this->type; };

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
            return fmt::format("{} {}", this->getType()->toStr(), this->toStr());
        };
};

template<class T>
class PrimitiveObject : public ApiObject {
    T data;

    public:
        PrimitiveObject(const PrimitiveType* _type, T _data) : ApiObject(_type->toStr(),
            -1, _type), data(_data) {};

        T getData() const { return this->data; };

        std::string toStr() const
        {
            return fmt::format("{}", this->data);
        };
        std::string toStrWithType() const
        {
            assert(false);
        };
};

class NamedObject : public ApiObject {
    public:
        NamedObject(std::string _name, const ApiType* _type) :
            ApiObject(_name, -1, _type) {};

        std::string toStr() const
        {
            return this->name;
        }
};

class ExprObject : public ApiObject {
    public:
        ExprObject(std::string _expr, const PrimitiveType* _type) :
            ApiObject(_expr, -1, _type) {};

        std::string toStr() const { return this->name; };
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

        bool checkArgs(std::vector<const ApiObject*>) const;
        std::string printSignature() const;
        std::string printInvocation(std::vector<const ApiObject*>) const;

        inline bool operator<(const ApiFunc& other) const {
            return this->printSignature() < other.printSignature();
        }
};

class ApiInstruction
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

class ApiFuzzer {
    private:
        std::set<const ApiType*> types;
        std::set<const ApiFunc*> funcs;
        std::vector<const ApiObject*> objs;
        std::vector<const ApiInstruction*> instrs;
        unsigned int next_obj_id;
        unsigned int depth;
        const unsigned int max_depth = 10;
        std::mt19937 rng;


        virtual const ApiObject* generateObject(const ApiType*) = 0;

    public:
        ApiFuzzer(std::mt19937 _rng): next_obj_id(0), depth(0),
            instrs(std::vector<const ApiInstruction*>()),
            objs(std::vector<const ApiObject*>()),
            types(std::set<const ApiType*>()),
            funcs(std::set<const ApiFunc*>()), rng(_rng) {};

        std::vector<const ApiInstruction*> getInstrList() const;
        std::vector<std::string> getInstrStrs() const;
        std::vector<const ApiObject*> getObjList() const;
        std::set<const ApiFunc*> getFuncList() const;
        std::set<const ApiType*> getTypeList() const;
        int getRandInt(int = 0, int = std::numeric_limits<int>::max());
        unsigned int getNextID();

        bool hasTypeName(std::string);
        bool hasFuncName(std::string);

        const ApiType* getTypeByName(std::string);
        template<typename T> std::vector<const ApiObject*> filterObjs(
            bool (ApiObject::*)(T) const, T);
        template<typename T> std::set<const ApiFunc*> filterFuncs(
            bool (ApiFunc::*)(T) const, T);
        const ApiFunc* getFuncByName(std::string);

        void addInstr(const ApiInstruction*);
        void addObj(const ApiObject*);
        void addType(const ApiType*);
        void addFunc(const ApiFunc*);

        virtual const ApiObject* generateSet() = 0;

    protected:
        ApiObject* generateApiObjectAndDecl(std::string, std::string,
            std::string, std::initializer_list<std::string>);
        const ApiObject* generateNamedObject(std::string, const ApiType*,
            const ApiFunc*, const ApiObject*, std::vector<const ApiObject*>);
        const ApiObject* generateApiObject(std::string, const ApiType*,
            const ApiFunc*, const ApiObject*, std::vector<const ApiObject*>);
        void applyFunc(const ApiFunc*, const ApiObject*, const ApiObject*);
        void applyFunc(const ApiFunc*, const ApiObject*, const ApiObject*,
            std::vector<const ApiObject*>);
        std::vector<const ApiObject*> getFuncArgs(const ApiFunc*);

    private:
        std::string emitFuncCond(const ApiFunc*, const ApiObject*,
            std::vector<const ApiObject*>);
        std::string parseCondition(std::string, const ApiObject*,
            std::vector<const ApiObject*>);
};

class ApiFuzzerNew : public ApiFuzzer {
    std::map<std::string, const ApiObject*> fuzzer_input;
    std::vector<YAML::Node> set_gen_instrs;

    public:
        ApiFuzzerNew(std::string&, std::mt19937);
        //~ApiFuzzerNew();

    private:
        void initPrimitiveTypes();
        void initInputs(YAML::Node);
        void initTypes(YAML::Node);
        void initFuncs(YAML::Node);
        ApiFunc* genNewApiFunc(YAML::Node);
        void initConstructors(YAML::Node);
        void initGenConfig(YAML::Node);
        void runGeneration(YAML::Node);
        void generateForLoop(YAML::Node);
        void generateConstructor(YAML::Node);
        void generateFunc(YAML::Node, int = -1);
        const ApiObject* getSingletonObject(const ApiType*);

        const ApiObject* generateObject(const ApiType*);
        const ApiObject* generateNewObject(const ApiType*);
        const ApiObject* generatePrimitiveObject(const PrimitiveType*);
        const ApiObject* generatePrimitiveObject(const PrimitiveType*, std::string);
        const ApiObject* generateSet();
        const ApiObject* getInputObject(std::string);

        std::pair<int, int> parseRange(std::string);
        int parseRangeSubstr(std::string);
        const ApiType* parseTypeStr(std::string);
        std::string getGeneratorData(std::string) const;
        std::string makeLinearExpr(std::vector<const ApiObject*>);
};

class ApiFuzzerISL : public ApiFuzzer {
    public:
        const unsigned int dims;
        const unsigned int params;
        const unsigned int constraints;

        ApiFuzzerISL(const unsigned int, const unsigned int, const unsigned int);
        ~ApiFuzzerISL();

        const ApiObject* generateSet();

    private:

        std::vector<const ApiObject*> dim_var_list;

        std::vector<const ApiObject*> getDimVarList();
        void addDimVar(const ApiObject*);

        void initFuncs();
        void initTypes();
        void clearObjs();
        void clearFuncs();
        void clearTypes();

        const ApiObject* generateObject(const ApiType*);
        const ApiObject* generateObject(std::string, std::string);
        const ApiObject* getCtx();
        const ApiObject* generateContext();
        const ApiObject* generateDimVar(const ApiObject*, std::string, const unsigned int);
        const ApiObject* getRandomDimVar();
        const ApiObject* getExistingVal();
        const ApiObject* generateVal();
        const ApiObject* generateSimpleVal();
        void augmentVal(const ApiObject*);
        void augmentConstraint(const ApiObject*);
        void addConstraintFromSet(const ApiObject*, const ApiObject*);
        const ApiObject* generatePWAff(const ApiObject*);
        void applyPWAFunc(const ApiObject*, std::string, std::initializer_list<std::string>);
        const ApiObject* generateSetFromConstraints(const ApiObject*, const ApiObject*);
};

#endif
