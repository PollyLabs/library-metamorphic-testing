#ifndef API_FUZZER_HPP
#define API_FUZZER_HPP

#include <experimental/random>
#include <functional>
#include <iostream>
#include <iterator>
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

class ApiType;
class ApiFunc;
class ApiObject;

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
        ApiType(std::string _name) : name(_name) {};

        std::string toStr() const { return this->name; };

        bool hasName(std::string name_check) const {
            return !this->toStr().compare(name_check);
        };
        bool isType(const ApiType* other) const  {
            return !this->toStr().compare(other->toStr());
        };

        virtual bool isSingleton() const { return false; };
        virtual bool isPrimitive() const { return false; };

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

        const ApiType* getUnderlyingType() { return this->underlying_type; };
        const ApiObject* retrieveObj();
};

class ApiObject {
    const unsigned int id;
    const std::string name;
    const ApiType* type;

    public:
        ApiObject(std::string _name, unsigned int _id, const ApiType* _type) :
            id(_id), name(_name), type(_type) {};

        const ApiType* getType() const { return this->type; };

        bool isPrimitive() const { return this->getType()->isPrimitive(); };
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

        std::string toStr() const { return fmt::format("{}", this->data); };

        std::string toStrWithType() const { assert(false); };
};

class ApiFunc {
    const std::string name;
    const ApiType* member_type;
    const ApiType* return_type;
    const std::vector<const ApiType*> param_types;
    const std::vector<std::string> conditions;
    const bool special;

    public:
        ApiFunc(std::string _name, const ApiType* _member_type, const ApiType* _return_type,
            std::vector<const ApiType*> _param_types,
            std::vector<std::string> _conditions, bool _special = false) :
            name(_name), member_type(_member_type), return_type(_return_type),
            param_types(_param_types), conditions(_conditions),
            special(_special) {};

        std::string getName() const { return this->name; };
        std::vector<const ApiType*> getParamTypes() const {
            return this->param_types;
        };
        const ApiType* getParamType(const unsigned int) const;
        unsigned int getParamCount() const { return this->param_types.size(); };
        const ApiType* getMemberType() const { return this->member_type; };
        const ApiType* getReturnType() const { return this->return_type; };

        bool hasMemberType(const ApiType* member_check) const {
            return this->getMemberType()->isType(member_check);
        };
        bool hasReturnType(const ApiType* return_check) const {
            return this->getReturnType()->isType(return_check);
        };
        bool hasName(std::string name_check) const {
            return !this->getName().compare(name_check);
        };
        bool hasParamTypes(std::vector<const ApiType*>) const;
        bool isSpecial() const { return this->special; };
        bool notIsSpecial() const { return !this->special; };

        bool checkArgs(std::vector<const ApiObject*>) const;
        std::string printSignature() const;
        std::string printInvocation(std::vector<const ApiObject*>,
            const ApiObject*) const;

        inline bool operator<(const ApiFunc& other) const {
            return this->printSignature() < other.printSignature();
        }
};

class ApiFuzzer {
    private:
        std::set<const ApiType*> types;
        std::set<const ApiFunc*> funcs;
        std::vector<const ApiObject*> objs;
        std::vector<std::string> instrs;
        unsigned int next_obj_id;

        virtual const ApiObject* generateObject(const ApiType*) = 0;

    public:
        ApiFuzzer(): next_obj_id(0), instrs(std::vector<std::string>()),
            objs(std::vector<const ApiObject*>()), types(std::set<const ApiType*>()),
            funcs(std::set<const ApiFunc*>()) {};

        std::vector<std::string> getInstrList();
        std::vector<const ApiObject*> getObjList();
        std::set<const ApiFunc*> getFuncList();
        std::set<const ApiType*> getTypeList();
        unsigned int getNextID();

        bool hasTypeName(std::string);
        bool hasFuncName(std::string);

        const ApiType* getTypeByName(std::string);
        template<typename T> std::vector<const ApiObject*> filterObjs(
            bool (ApiObject::*)(T) const, T);
        template<typename T> std::set<const ApiFunc*> filterFuncs(
            bool (ApiFunc::*)(T) const, T);
        const ApiFunc* getFuncByName(std::string);

        void addInstr(std::string);
        void addObj(const ApiObject*);
        void addType(const ApiType*);
        void addFunc(const ApiFunc*);

        virtual const ApiObject* generateSet() = 0;

    protected:
        ApiObject* generateApiObjectAndDecl(std::string, std::string,
            std::string, std::initializer_list<std::string>);
        //const ApiObject* generateApiObject(std::string, const ApiType*,
            //std::vector<const ApiObject*>);
        const ApiObject* generateApiObject(std::string, const ApiType*,
            const ApiFunc*, const ApiObject*, std::vector<const ApiObject*>);
        void applyFunc(const ApiFunc*, const ApiObject*, const ApiObject*,
            std::vector<const ApiObject*>);
        void applyFunc(const ApiFunc*, const ApiObject*, const ApiObject*);
        std::vector<const ApiObject*> getFuncArgs(const ApiFunc*);
};

class ApiFuzzerNew : public ApiFuzzer {
    std::map<std::string, const ApiObject*> fuzzer_input;
    std::vector<YAML::Node> set_gen_instrs;

    public:
        ApiFuzzerNew(std::string& config_file_path);
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
        void generateFunc(YAML::Node);
        const ApiObject* getSingletonObject(const ApiType*);

        const ApiObject* generateObject(const ApiType*);
        const ApiObject* generateNewObject(const ApiType*);
        const ApiObject* generatePrimitiveObject(const PrimitiveType*);
        const ApiObject* generatePrimitiveObject(const PrimitiveType*, std::string);
        const ApiObject* generateSet();
        const ApiType* parseTypeStr(std::string);
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
