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
write_args(std::stringstream &ss, isl_tester::Arguments args,
   std::string meta_rel)
{
    write_line(ss, "// Seed: " + std::to_string(args.seed));
    write_line(ss, "// Max dims: " + std::to_string(args.max_dims));
    write_line(ss, "// Max params: " + std::to_string(args.max_params));
    write_line(ss, "// Max set count: " + std::to_string(args.max_set_count));
    write_line(ss, "// Meta relation: " + meta_rel);
    write_line(ss, "");
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
gen_coalesce_split_test(std::stringstream &ss)
{
    write_line(ss, "s = s.convex_hull();");
    write_line(ss, "isl::point p = s.sample_point();");
    write_line(ss, "isl::set split1 = s.upper_bound_si(isl::dim::set, 0, p.get_coordinate_val(isl::dim::set, 0).get_num_si());");
    write_line(ss, "isl::set split2 = s.lower_bound_si(isl::dim::set, 0, p.get_coordinate_val(isl::dim::set, 0).get_num_si());");
    write_line(ss, "isl::union_set split_us = isl::union_set(split1);");
    write_line(ss, "split_us.add_set(split2);");
    write_line(ss, "s = split_us.coalesce().sample();");
}

void
main_post_setup(std::stringstream &ss)
{
    write_line(ss, "assert(r0.is_equal(r1));");
    indent--;
    write_line(ss, "}");
    write_line(ss, "isl_ctx_free(ctx_ptr);");
    indent--;
    write_line(ss, "}");
}

std::string
get_generator(const YAML::Node generator_list, std::string type)
{
    YAML::Node typed_generator_list;
    try {
        typed_generator_list = generator_list[type];
    } catch (YAML::KeyNotFound) {
        std::cout << "Could not find given type " << type << std::endl;
        exit(1);
    }
    return typed_generator_list[std::rand() % typed_generator_list.size()]
            .as<std::string>();
}

std::queue<std::string>
gen_meta_relation(const YAML::Node relation_list, unsigned int count)
{
    std::queue<std::string> meta_relation;
    std::cout << "REL ";
    while (count-- > 0) {
        int relation_id = std::rand() % relation_list.size();
        YAML::const_iterator it = relation_list.begin();
        while (relation_id-- > 0)
            it++;
        meta_relation.push(it->first.as<std::string>());
        //std::cout << it->first.as<std::string>() << " ";
    }
    std::cout << std::endl;
    return meta_relation;
}

std::string
get_meta_relation(std::queue<std::string> meta_rel_queue)
{
    std::string string_rel = "";
    while (!meta_rel_queue.empty()) {
        string_rel += meta_rel_queue.front() + "-";
        meta_rel_queue.pop();
    }
    return string_rel;
}

std::string
get_relation(const YAML::Node relation_list, std::string relation_type)
{
    const YAML::Node selected_relation_list = relation_list[relation_type];
    return selected_relation_list[std::rand() % selected_relation_list.size()]
            .as<std::string>();
}

void
replace_meta_inputs(std::string &rel, const std::string input_var,
    const YAML::Node meta_list)
{
    while (true) {
        int pos = rel.find("%");
        if (pos == std::string::npos)
            break;
        char type = rel[pos + 1];
        if (std::isdigit(type))
            rel.replace(pos, 2, input_var);
        else
            switch (type) {
                case 'e': rel.replace(pos, 2,
                    get_generator(meta_list["generators"], "empty")); break;
                case 'u': rel.replace(pos, 2,
                    get_generator(meta_list["generators"], "universe")); break;
                default:
                    std::cout << "Unknown input modifier %" << type << std::endl;
                    exit(1);
            }
    }
    if (rel[0] == '.')
        rel.insert(0, input_var);
}

std::string
gen_meta_func(const std::string input_var, std::string meta_relation,
    const YAML::Node meta_list)
{
    std::string new_rel = get_relation(meta_list["relations"], meta_relation);
    assert(new_rel.find("%1") != std::string::npos);
    //std::cout << new_rel << std::endl;
    replace_meta_inputs(new_rel, input_var, meta_list);
    //std::cout << new_rel << std::endl;
    return new_rel;
}

std::string
gen_meta_expr(std::stringstream &ss, const unsigned int var_count, std::queue<std::string> meta_rel,
    const YAML::Node meta_list, std::set<std::string> gen_exprs)
{
    std::string input_var = "r" + std::to_string(var_count);
    std::string new_expr = gen_meta_func("s", meta_rel.front(), meta_list);
    write_line(ss, "isl::set " + input_var + " = " + new_expr + ";");
    meta_rel.pop();
    while (!meta_rel.empty()) {
        new_expr = gen_meta_func(input_var, meta_rel.front(), meta_list);
        if (meta_rel.front() == "identity" && new_expr.find("coalesce") == std::string::npos)
            continue;
        write_line(ss, input_var + " = " + new_expr + ";");
        meta_rel.pop();
    }
    return "yes";
}

void
run_simple(isl::set set_in, isl_tester::Arguments &args)
{
    YAML::Node meta_list = YAML::LoadFile("./set_meta_tests.yaml");
    std::string variant = "single_distinct";

    unsigned int meta_rel_count = std::rand() % 5 + 1;
    std::string r1_expr, r2_expr;
    std::queue<std::string> meta_rel = gen_meta_relation(
                                        meta_list["relations"], meta_rel_count);
    meta_rel.push("identity");

    std::stringstream ss;
    write_args(ss, args, get_meta_relation(meta_rel));
    prepare_header(ss);
    ss << std::endl;
    main_pre_setup(ss);
    gen_var_declarations(ss, set_in);
    gen_coalesce_split_test(ss);
    gen_meta_expr(ss, 0, meta_rel, meta_list, std::set<std::string>());
    gen_meta_expr(ss, 1, meta_rel, meta_list, std::set<std::string>());
    main_post_setup(ss);

    std::ofstream ofs;
    ofs.open("out/test.cpp");
    ofs << ss.rdbuf();
    ofs.close();
}

}
