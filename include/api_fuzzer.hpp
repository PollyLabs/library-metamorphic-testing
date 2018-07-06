#ifndef API_FUZZER_HPP
#define API_FUZZER_HPP

#include <initializer_list>
#include <string>
#include <sstream>
#include <vector>

#include "isl-noexceptions.h"
#include "fmt/format.h"

enum ApiTarget {
    ISL,
};

class ApiObject {
    const unsigned int id;
    const std::string name;
    const std::string type;

    public:
        ApiObject(std::string _name, unsigned int _id, std::string _type) :
            id(_id), name(_name), type(_type) {};
        std::string toStr();
        std::string toStrWithType();
        std::string getType();
};

class ApiFunc {
    const std::string name;
    const std::string member_of;
    const std::vector<std::string> param_types;
    const std::vector<std::string> conditions;

    public:
        ApiFunc(std::string _name, std::string _member_of,
            std::initializer_list<std::string> _param_types,
            std::initializer_list<std::string> _conditions) :
            name(_name), member_of(_member_of),
            param_types(std::vector<std::string>(_param_types)),
            conditions(std::vector<std::string>(_conditions)) {};

        std::string getName();
        std::vector<std::string> getParamTypes();
        unsigned int getParamCount();

        bool isMemberOf(std::string);
        bool hasParamTypes(std::initializer_list<std::string>);
};

class ApiFuzzer {
    public:
        ApiFuzzer(): next_obj_id(0), instrs(std::vector<std::string>()),
            objs(std::vector<ApiObject>()) {};
        std::vector<std::string> getInstrs();
        void addInstr(std::string);
        std::vector<ApiObject> getObjList();
        std::vector<ApiObject> getObjByType(std::string);
        void addObj(ApiObject);
        unsigned int getNextID();
        virtual ApiObject generateSet() = 0;

    protected:
        ApiObject generateApiObjectAndDecl(std::string, std::string,
            std::string, std::initializer_list<std::string>);
        void applyFunc(ApiObject&, std::string, bool,
            std::initializer_list<std::string>);

    private:
        std::vector<std::string> instrs;
        std::vector<ApiObject> objs;
        unsigned int next_obj_id;
};

class ApiFuzzerISL : public ApiFuzzer {
    public:
        const unsigned int dims;
        const unsigned int params;
        const unsigned int constraints;

        ApiFuzzerISL(const unsigned int, const unsigned int, const unsigned int);

        virtual ApiObject generateSet();

    private:

        std::vector<ApiObject> dim_var_list;

        std::vector<ApiObject> getDimVarList();
        void addDimVar(ApiObject);

        ApiObject getCtx();
        ApiObject generateContext();
        ApiObject generateDimVar(ApiObject, std::string, const unsigned int);
        ApiObject getRandomDimVar();
        ApiObject getExistingVal();
        ApiObject generateVal(ApiObject&, bool);
        void augmentVal(ApiObject&);
        void augmentConstraint(ApiObject&);
        void addConstraintFromSet(ApiObject&, ApiObject&);
        ApiObject generatePWAff(ApiObject&);
        void applyPWAFunc(ApiObject&, std::string, std::initializer_list<std::string>);
        ApiObject generateSetFromConstraints(ApiObject&, ApiObject&);
};

#endif
