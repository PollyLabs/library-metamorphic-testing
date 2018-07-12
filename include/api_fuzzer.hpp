#ifndef API_FUZZER_HPP
#define API_FUZZER_HPP

#include <functional>
#include <iostream>
#include <iterator>
#include <string>
#include <sstream>
#include <set>
#include <vector>

#include "isl-noexceptions.h"
#include "fmt/format.h"

enum ApiTarget {
    ISL,
};

class ApiType;
class ApiFunc;
class ApiObject;

std::string getStringWithDelims(std::vector<std::string>, char);
template<typename T> std::string makeArgString(std::vector<T>);
template<typename T> T getRandomVectorElem(std::vector<T>&);

class ApiType {
    const std::string name;

    public:
        ApiType(std::string _name) : name(_name) {};

        std::string getTypeStr() const;
        bool isType(ApiType*) const;
        bool hasName(std::string) const;
        static ApiType getVoid();

        std::string toStr();

        inline bool operator<(const ApiType* other) const {
            return this->getTypeStr() < other->getTypeStr();
        }
};

class ApiObject {
    const unsigned int id;
    const std::string name;
    const ApiType* type;

    public:
        ApiObject(std::string _name, unsigned int _id, ApiType* _type) :
            id(_id), name(_name), type(_type) {};
        std::string toStr() const;
        std::string toStrWithType() const;
        const ApiType* getType() const;
        bool hasType(ApiType*);
};

class ApiFunc {
    const std::string name;
    const ApiType* member_type;
    const ApiType* return_type;
    const std::vector<ApiType*> param_types;
    const std::vector<std::string> conditions;

    public:
        ApiFunc(std::string _name, ApiType* _member_type, ApiType* _return_type,
            std::vector<ApiType*> _param_types,
            std::vector<std::string> _conditions) :
            name(_name), member_type(_member_type), return_type(_return_type),
            param_types(_param_types), conditions(_conditions) {};

        std::string getName() const;
        std::vector<ApiType*> getParamTypes() const;
        unsigned int getParamCount() const;
        const ApiType* getMemberType() const;
        const ApiType* getReturnType() const;

        bool hasMemberType(ApiType*) const;
        bool hasReturnType(ApiType*) const;
        bool hasName(std::string) const;
        bool hasParamTypes(std::vector<ApiType*>) const;

        std::string print() const;

        inline bool operator<(const ApiFunc& other) const {
            return this->print() < other.print();
        }
};

class ApiFuzzer {
    private:
        std::set<ApiType*> types;
        std::set<ApiFunc*> funcs;
        std::vector<ApiObject*> objs;
        std::vector<std::string> instrs;
        unsigned int next_obj_id;

        virtual ApiObject* generateObject(ApiType*) = 0;

    public:
        ApiFuzzer(): next_obj_id(0), instrs(std::vector<std::string>()),
            objs(std::vector<ApiObject*>()), types(std::set<ApiType*>()),
            funcs(std::set<ApiFunc*>()) {};

        std::vector<std::string> getInstrList();
        std::vector<ApiObject*> getObjList();
        std::set<ApiFunc*> getFuncList();
        std::set<ApiType*> getTypeList();
        unsigned int getNextID();

        std::vector<ApiObject*> filterObjByType(ApiType*);
        std::vector<ApiObject*> filterObjByType(std::string);
        ApiType* getTypeByName(std::string);
        template<typename T> std::set<ApiFunc*> filterFuncs(
            bool (ApiFunc::*)(T) const, T);
        template<typename T> std::set<ApiFunc*> filterFuncs(
            std::set<ApiFunc*>, bool (ApiFunc::*)(T) const, T);
        ApiFunc* getFuncByName(std::string);

        void addInstr(std::string);
        void addObj(ApiObject*);
        void addType(ApiType*);
        void addFunc(ApiFunc*);

        virtual ApiObject* generateSet() = 0;

    protected:
        ApiObject* generateApiObjectAndDecl(std::string, std::string,
            std::string, std::initializer_list<std::string>);
        void applyFunc(ApiFunc*, ApiObject*, bool);
        void applyFunc(ApiFunc*, ApiObject*, bool,
            std::vector<ApiObject*>);
        void applyFuncAndStore(ApiFunc*, ApiObject*, ApiObject*,
            std::vector<ApiObject*>);
        std::vector<ApiObject*> getFuncArgs(ApiFunc*);

};

class ApiFuzzerISL : public ApiFuzzer {
    public:
        const unsigned int dims;
        const unsigned int params;
        const unsigned int constraints;

        ApiFuzzerISL(const unsigned int, const unsigned int, const unsigned int);
        ~ApiFuzzerISL();

        virtual ApiObject* generateSet();

    private:

        std::vector<ApiObject*> dim_var_list;

        std::vector<ApiObject*> getDimVarList();
        void addDimVar(ApiObject*);

        void initFuncs();
        void initTypes();
        void clearObjs();
        void clearFuncs();
        void clearTypes();

        virtual ApiObject* generateObject(ApiType*);
        ApiObject* generateObject(std::string, std::string);
        ApiObject* getCtx();
        ApiObject* generateContext();
        ApiObject* generateDimVar(ApiObject*, std::string, const unsigned int);
        ApiObject* getRandomDimVar();
        ApiObject* getExistingVal();
        ApiObject* generateVal();
        ApiObject* generateSimpleVal();
        void augmentVal(ApiObject*);
        void augmentConstraint(ApiObject*);
        void addConstraintFromSet(ApiObject*, ApiObject*);
        ApiObject* generatePWAff(ApiObject*);
        void applyPWAFunc(ApiObject*, std::string, std::initializer_list<std::string>);
        ApiObject* generateSetFromConstraints(ApiObject*, ApiObject*);
};

#endif
