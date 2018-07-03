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
        std::string to_str();
        std::string to_str_with_type();
};

class ApiFuzzer {
    public:
        ApiFuzzer(): next_obj_id(0), instrs(std::vector<std::string>()),
            objs(std::vector<ApiObject>()) {};
        std::vector<std::string> getInstrs();
        void addInstr(std::string);
        void addObj(ApiObject);
        unsigned int getNextID();
        virtual void generateSet() = 0;

    private:
        std::vector<std::string> instrs;
        std::vector<ApiObject> objs;
        unsigned int next_obj_id;
};

class ApiFuzzerISL : public ApiFuzzer {
    public:
        ApiFuzzerISL();

        virtual void generateSet();

    private:
        std::vector<ApiObject> inputs;

        std::vector<ApiObject> getDimVarList();

        ApiObject generateApiObjectAndDecl(std::string, std::string, std::string, std::initializer_list<std::string>);

        ApiObject generateConstraintExpr();
        ApiObject generateContext();
        ApiObject generateLocalSpace(ApiObject);
        ApiObject generateDimVar(ApiObject, std::string, const unsigned int);
        ApiObject getRandomDimVar();
        ApiObject generateSpace(ApiObject, const unsigned int, const unsigned int);
        ApiObject generateVal(ApiObject);
        ApiObject getVal(ApiObject);
        ApiObject generateSetDecl(ApiObject);
        ApiObject generateConstraint(ApiObject);
        void addConstraintToSet(ApiObject, ApiObject);
        //ApiObject generateSet(ApiObject);
};

#endif
