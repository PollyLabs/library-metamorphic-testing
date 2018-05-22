#include "set_meta_tester.hpp"

namespace set_meta_tester {

int indent = 0;

void
write_line(std::stringstream &ss, std::string line)
{
    int indent_count = 0;
    while (indent_count++ < indent)
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
        write_line(ss, "#include " + incl);
}

void
main_pre_setup(std::stringstream &ss)
{
    write_line(ss, "int main()");
    write_line(ss, "{");
    indent++;
    write_line(ss, "isl_ctx *ctx_ptr = isl_ctx_alloc();");
    write_line(ss, "{");
    indent++;
    write_line(ss, "isl::ctx ctx(ctx_ptr);");
}

void
gen_var_declarations(std::stringstream &ss, isl::set set)
{
    write_line(ss, "isl::set s = isl::set(ctx, \"" + set.to_str() + "\");");
    write_line(ss, "isl::set u = isl::set::universe(s.get_space());");
    write_line(ss, "isl::set e = isl::set::empty(s.get_space());");
}


void
main_post_setup(std::stringstream &ss)
{
    write_line(ss, "assert(r1.is_equal(r2));");
    indent--;
    write_line(ss, "}");
    write_line(ss, "isl_ctx_free(ctx_ptr);");
    indent--;
    write_line(ss, "}");
}

std::string
get_relation(const YAML::Node relation_list)
{
    int selected_relation_id = std::rand() % relation_list.size();
    YAML::const_iterator it = relation_list.begin();
    while (--selected_relation_id > 0)
        it++;
    YAML::Node selected_relation_list = it->second;
    return selected_relation_list[std::rand() % selected_relation_list.size()]
            .as<std::string>();
}

std::string
gen_meta_func(const YAML::Node relation_list, int count)
{
    std::string rel = "%I";
    while (count-- > 0) {
        std::string new_rel = get_relation(relation_list);
        int pos = new_rel.find("%1");
        assert(pos != std::string::npos);
        rel = new_rel.replace(pos, 2, rel);
    }
    std::cout << rel << std::endl;
    while (true) {
        int pos = rel.find("%");
        if (pos == std::string::npos)
            break;
        char type = rel[pos + 1];
        switch (type) {
            case 'i':
            case 'I': rel.replace(pos, 2, "s"); break;
            case 'e': rel.replace(pos, 2, "e"); break;
            case 'u': rel.replace(pos, 2, "u"); break;
        }
    }
    if (rel[0] == '.')
        rel.insert(0, "s");
    std::cout << rel << std::endl;
    return rel;
}

std::pair<std::string, std::string>
gen_pair_exprs(const YAML::Node relation_list, int meta_rel_count)
{
    std::string expr1 = gen_meta_func(relation_list, meta_rel_count);
    std::string expr2;
    do {
        expr2 = gen_meta_func(relation_list, meta_rel_count);
    } while (!expr1.compare(expr2));
    return std::pair<std::string, std::string>(expr1, expr2);
}

void
run_simple(isl::set set_in)
{
    YAML::Node meta_input = YAML::LoadFile("set_meta_tests.yaml");
    YAML::Node relation_list = meta_input["relations"];
    YAML::Node generator_list = meta_input["generators"];
    std::string variant = "single_distinct";

    std::stringstream ss;
    prepare_header(ss);
    ss << std::endl;
    main_pre_setup(ss);
    gen_var_declarations(ss, set_in);

    int meta_rel_count = std::rand() % 5;
    std::string r1_expr, r2_expr;
    if (!variant.compare("single_random")) {
        r1_expr = gen_meta_func(relation_list, meta_rel_count);
        r2_expr = gen_meta_func(relation_list, meta_rel_count);
    }
    else if (!variant.compare("single_distinct")) {
        std::pair<std::string, std::string> exprs =
            gen_pair_exprs(relation_list, meta_rel_count);
        r1_expr = exprs.first;
        r2_expr = exprs.second;
    }
    write_line(ss, "isl::set r1 = " + r1_expr +  ";");
    write_line(ss, "isl::set r2 = " + r2_expr +  ";");

    main_post_setup(ss);

    std::ofstream ofs;
    ofs.open("out/test.cpp");
    ofs << ss.rdbuf();
    ofs.close();
}

}
