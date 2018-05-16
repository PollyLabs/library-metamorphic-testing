#include "set_meta_tester.hpp"

namespace set_meta_tester {

enum VAR_TYPE {
    SET,
    UNIVERSE,
    RESULT,
};

struct {
    std::string fn_name;
    std::string fn;
} set_funcs [] = {
    { "complement", ".complement()" },
    { "unite", ".unite(%s)" },
    { "interesect", ".intersect(%s)" },
    { "subtract", ".subtract(%s)" },
};

std::string
apply_func(isl::set set1, std::vector<isl::set> replace_set_list,
    std::string func_desc)
{
    int index = 0;
    while (func_desc.find("%s") != std::string::npos) {
        assert(index < replace_set_list.size());
        func_desc.replace(func_desc.find("%s"), strlen("%s"),
            replace_set_list.at(index++).to_str());
    }
    return func_desc;
}

void
write_line(std::stringstream &ss, std::string line, int indent)
{
    while (indent-- > 0)
        ss << "\t";
    ss << line << std::endl;
}

void
prepare_header(std::stringstream &ss)
{
    std::vector<std::string> include_list = {
        "\"isl-noexceptions.h\"",
        "<cassert>",
    };
    for (std::string incl : include_list)
        write_line(ss, "#include " + incl, 0);
}

void
main_pre_setup(std::stringstream &ss)
{
    write_line(ss, "int main()", 0);
    write_line(ss, "{", 0);
    write_line(ss, "isl_ctx *ctx_ptr = isl_ctx_alloc();", 1);
    write_line(ss, "isl::ctx ctx(ctx_ptr);", 1);
}

void
main_post_setup(std::stringstream &ss)
{
    write_line(ss, "}", 0);
}

std::string
gen_meta_func(isl::set set, const std::vector<std::string> relation_list)
{
    std::string rel = relation_list.at(std::rand() % relation_list.size());
    std::cout << rel << std::endl;
    while (true) {
        int pos = rel.find("%");
        if (pos == std::string::npos)
            break;
        char type = rel[pos + 1];
        switch (type) {
            case 'l': rel.replace(pos, 2, "s"); break;
            case 'e': rel.replace(pos, 2, "e"); break;
            case 'u': rel.replace(pos, 2, "u"); break;
        }
    }
    if (rel[0] == '.')
        rel.insert(0, "s");
    std::cout << rel << std::endl;
    return rel;
}

/*std::string
gen_new_var_name(std::map<std::string, isl::set> &var_map, VAR_TYPE vt)
{
    std::string prefix;
    switch(vt):
        case VAR_TYPE::SET: prefix = "s"; break;
        case VAR_TYPE::UNIVERSE: prefix = "u"; break;
        case VAR_TYPE::RESULT: prefix = "r"; break;

    int index = 0;
    while (var_map.count(prefix + index) != 0)
        index++;
    return prefix + std::to_string(index);
}

void
define_set_var(std::stringstream &ss, isl::set set,
    std::map<isl::set, std::string> &var_map)
{
    std::string new_var_name = gen_new_var_name(var_map, VAR_TYPE::SET);
    std::string set_def = "isl::set " + new_var_name + " = isl::set(ctx, \"";
    set_def += set.to_str() + "\");";
    write_line(ss, set_def, 1);
    var_map.insert(std::pair(set, new_var_name));
}

void
define_universe_set_var(std::stringstream &ss, isl::set set,
    std::map<isl::set, std::string> &var_map)
{
    std::string new_var_name = gen_new_var_name(var_map, VAR_TYPE::UNIVERSE);
    std::string set_def = "isl::set " + new_var_name + " = isl::set::universe(";
    write_line(ss, set_def, 1);
    var_map.insert(std::pair<isl::set, std::string>(set, new_var_name));
}*/

void
run_simple(isl::set set_in)
{
    std::ifstream ifs("set_meta_tests", std::ios::in);
    std::vector<std::string> relation_list;
    for (std::string curr_rel; getline(ifs, curr_rel);)
        relation_list.push_back(curr_rel);

    std::stringstream ss;
    prepare_header(ss);
    ss << std::endl;
    main_pre_setup(ss);
    ss << "\tisl::set s = isl::set(ctx, \"" << set_in.to_str() << "\");\n";
    ss << "\tisl::set u = isl::set::universe(s.get_space());\n";
    ss << "\tisl::set e = isl::set::empty(s.get_space());\n";

    ss << "\tisl::set r1 = " << gen_meta_func(set_in, relation_list) << ";\n";
    ss << "\tisl::set r2 = " << gen_meta_func(set_in, relation_list) << ";\n";
    ss << "\tassert(r1.is_equal(r2));\n";

    main_post_setup(ss);

    std::ofstream ofs;
    ofs.open("out/test.cpp");
    ofs << ss.rdbuf();
    ofs.close();
}

}
