#include "clang_interface.hpp"

namespace fuzzer {
namespace clang {

static ApiFuzzerNew* fuzzer_instance = nullptr;
size_t seed = -1;
const std::string meta_test_path =
    "/home/sentenced/Documents/Internships/2018_ETH/work/testing_functional/"
    "config_files/isl/set_meta_tests_isl.yaml";

ApiFuzzerNew*
getFuzzer()
{
    if (!fuzzer_instance)
    {
        logDebug(fmt::format("Generating new ApiFuzzerNew instance with seed {}.",
            seed));
        fuzzer_instance = new ApiFuzzerNew(seed);
        fuzzer_instance->setMaxDepthObjGen(true);
        addPrimitiveTypes(fuzzer_instance);
        addBaseFuncs(fuzzer_instance);
    }
    return fuzzer_instance;
}

void
setSeed(size_t new_seed)
{
    logDebug(fmt::format("Setting new seed {} over old value {}",
        new_seed, seed));
    seed = new_seed;
}

void
addPrimitiveTypes(ApiFuzzerNew* afn)
{
    afn->addType(new PrimitiveType("int"));
    afn->addType(new PrimitiveType("int32_t"));
    afn->addType(new PrimitiveType("unsigned int"));
    afn->addType(new PrimitiveType("long"));
    afn->addType(new PrimitiveType("std::string"));
    afn->addType(new PrimitiveType("double"));
    afn->addType(new PrimitiveType("bool"));
}

void
addBaseFuncs(ApiFuzzerNew* afn)
{
    const ApiFunc* assert_func = new ApiFunc("assert", nullptr, nullptr,
        std::vector<const ApiType*>({afn->getTypeByName("bool")}),
        std::vector<std::string>(), true);
    afn->addFunc(assert_func);
}

void
addLibType(std::string name)
{
    getFuzzer()->addType(new ApiType(name));
    logDebug(fmt::format("Added lib type {}", name));
}

void
addLibType(std::string name, bool ptr, bool sngl)
{
    getFuzzer()->addType(new ApiType(name, ptr, sngl));
    logDebug(fmt::format("Added lib type {} with pointer {}, singleton {}",
        name, ptr, sngl));
}

void
addLibEnumType(std::string name)
{
    getFuzzer()->addType(new EnumType(name));
    logDebug(fmt::format("Added lib enum type {}", name));
}

void
addLibEnumVal(std::string name, std::string enum_name)
{
    if (size_t enum_pos = enum_name.find("enum ");
        enum_pos != std::string::npos)
    {
        CHECK_CONDITION(enum_pos == 0, fmt::format(
            "Found `enum ` string at position {} instead of 0!", enum_pos));
        enum_name = enum_name.substr(sizeof("enum ") - 1);
    }
    const ApiType* get_type = getFuzzer()->getTypeByName(enum_name);
    CHECK_CONDITION(get_type->isEnum(),
        fmt::format("Expected {} type to be enumType!", enum_name));
    const EnumType* enum_type = dynamic_cast<const EnumType*>(get_type);
    enum_type->addValue(name);
    logDebug(fmt::format("Added enum value {} to type {}.",
        name, enum_name));
}

void
addLibTemplateType(std::string base_name, size_t template_count)
{
    const TemplateType* template_t = new TemplateType(base_name, template_count);
    getFuzzer()->addType(template_t);
    logDebug(fmt::format("Added lib template type {}", template_t->toStr()));
    //addLibType(base_name);
    //const ApiType* base_type = getFuzzer()->getTypeByName(base_name);
    //std::vector<const ApiType*> template_type_list;
    //std::transform(std::begin(template_list), std::end(template_list),
        //std::back_inserter(template_type_list),
        //[](std::string template_str)
        //{
            //return getFuzzer()->getTypeByName(template_str);
        //});
    //const TemplateType* template_t = new TemplateType(base_type,
        //template_type_list);
    //getFuzzer()->addType(template_t);
    //logDebug(fmt::format("Added lib template type {}", template_t->toStr()));
}

void
addLibFunc(std::string name, std::string enclosing_class_name,
    std::string return_type_name, std::vector<std::string> param_type_names,
    bool statik, bool ctor, bool special)
{
    std::vector<const ApiType*> param_types;
    std::transform(param_type_names.begin(), param_type_names.end(),
        std::back_inserter(param_types), [](std::string type_name)
        {
            return getFuzzer()->getTypeByName(cleanTypeName(type_name));
        });

    const ApiType* return_type = nullptr;
    if (return_type_name.compare("void"))
    {
        return_type = getFuzzer()->getTypeByName(cleanTypeName(return_type_name));
    }
    else
    {
        assert(ctor);
        return_type = getFuzzer()->getTypeByName(enclosing_class_name);
    }
    if (ctor)
    {
        enclosing_class_name = "";
    }

    const ApiType* enclosing_class = nullptr;
    if (!enclosing_class_name.empty())
    {
        enclosing_class = getFuzzer()->getTypeByName(enclosing_class_name);
    }
    const ApiFunc* new_func = new ApiFunc(name,
        enclosing_class, return_type, param_types, std::vector<std::string>(),
        special, statik, ctor);
    getFuzzer()->addFunc(new_func);
    logDebug(fmt::format("Added lib func {}", new_func->printSignature()));
}

void
addLibDeclaredObj(std::string name, std::string type_name)
{
    const ApiType* new_obj_type =
        getFuzzer()->getTypeByName(cleanTypeName(type_name));
    const ApiObject* new_obj = getFuzzer()->addNewNamedObj(name,
            getFuzzer()->getTypeByName(cleanTypeName(type_name)));
    new_obj->setDeclared();
    logDebug(fmt::format("Added lib obj {}", new_obj->toStrWithType()));
}

/**
 * @brief Call the fuzzer to generate a new object
 *
 * Given an object `type_name`, calls the fuzzer to generate a sequence of
 * instructions to generate a new object of the required type, with the
 * specified `indent` level. The fuzzer should be initialised before-hand with
 * appropriate function and type declarations in particular, and object
 * declarations optionally.
 *
 * @param type_name The name of the type of the generated object
 * @param indent The indentation level at which to generate the instruction
 * sequence
 *
 * @return A pair of strings, with the first element being the name of the
 * generated object, and the second element the sequence of instructions to
 * generate the object.
 */
std::pair<std::string, std::string>
generateObjectInstructions(std::string type_name, std::string indent)
{
    const ApiType* new_obj_type = getFuzzer()->getTypeByName(type_name);
    const ApiObject* new_obj = getFuzzer()->generateNewObject(new_obj_type);
    std::vector<std::string> instrs_str_vec = getFuzzer()->getInstrStrs();
    std::string instrs_str = std::accumulate(std::begin(instrs_str_vec),
        std::end(instrs_str_vec), std::string(),
        [indent](std::string acc, std::string nxt)
        {
            return acc + '\n' + indent + nxt;
        });
    instrs_str += '\n';
    getFuzzer()->flushInstrs();
    return std::make_pair(instrs_str, new_obj->toStr());
}

/**
 * @brief Generate a sequence of instructions which perform metamorphic testing
 *
 * Construct a number of metamorphic tests, as set in
 * `ApiFuzzerNew->meta_var_count`, using the given `input_vars_names` of type
 * `input_var_type` as inputs to those variants. The metamorphic relations are
 * read in from the file at `set_meta_tests_path`.
 *
 * @param input_vars_names The names of input_variables already generated
 * @param input_var_type The name of the type for all input variables
 * @param indent The indentation level at which to generate the instruction
 * sequence
 * @param set_meta_tests_path Location of the file containing metamorphic
 * relation delcaration in old-style YAML format
 * @param relation_count The number of relations per a metamorphic test
 *
 * @return A string representing the concatenation of all generated instructions
 * for metamorphic tests, at the specified indentation level
 */
std::string
generateMetaTestInstructions(std::vector<std::string>& input_vars_names,
    const std::string& input_var_type, const std::string& indent,
    const std::string& set_meta_tests_path, size_t relation_count)
{
    getFuzzer()->fuzzerMetaInit(set_meta_tests_path);
    std::vector<const ApiObject*> meta_input_vars;
    std::transform(input_vars_names.begin(), input_vars_names.end(),
        std::back_inserter(meta_input_vars),
        [&input_var_type](std::string obj_name)
        {
            return getFuzzer()->addNewNamedObj(obj_name,
                getFuzzer()->getTypeByName(input_var_type));
        });
    getFuzzer()->setMetaInputVars(meta_input_vars);
    SetMetaTesterNew smt(getFuzzer());
    smt.genMetaTests(relation_count);

    std::vector<std::string> instrs_str_vec = getFuzzer()->getInstrStrs();
    std::string instrs_str = std::accumulate(std::begin(instrs_str_vec),
        std::end(instrs_str_vec), std::string(),
        [indent](std::string acc, std::string nxt)
        {
            return acc + '\n' + indent + nxt;
        });
    instrs_str += '\n';
    getFuzzer()->flushInstrs();
    return instrs_str;
}

/**
 * @brief Reset the existing object list to the provided variables
 *
 * Removes all existing `ApiObject`s gathered by the fuzzer, and initializes
 * them with the list provided in `init_vars`: for each element, a type and name
 * is provided, for which a new `NamedObject` is created.
 *
 * @param init_vars Set of pairs of (name, type_name) for new variables
 */
void
resetApiObjs(std::set<std::pair<std::string, std::string>> init_vars)
{
    static size_t i = 0;
    logDebug(fmt::format("Reset count {}", ++i));
    getFuzzer()->resetApiObjs();
    for (std::pair<std::string, std::string> new_var : init_vars)
    {
        addLibDeclaredObj(new_var.first, new_var.second);
    }
}

int
generateRand(int min, int max)
{
    return getFuzzer()->getRandInt(min, max);
}

double
generateRand(double min, double max)
{
    return getFuzzer()->getRandDouble(min, max);
}

std::string
generateRandStr(uint8_t min, uint8_t max)
{
    return getFuzzer()->getRandString(min, max);
}

std::string
cleanTypeName(std::string type_name)
{
    std::vector<std::string> remove_decl_vec {"class", "const", "&", "enum"};
    for (std::string remove_decl : remove_decl_vec)
    {
        size_t class_pos = type_name.find(remove_decl);
        if (class_pos != std::string::npos)
        {
            type_name = type_name.erase(class_pos, remove_decl.length());
        }
    }
    while (std::isspace(type_name.front()))
    {
        type_name = type_name.erase(0, 1);
    }
    while (std::isspace(type_name.back()))
    {
        type_name.pop_back();
    }
    return type_name;
}


} // namespace clang
} // namespace fuzzer
