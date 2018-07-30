#include "set_meta_tester.hpp"

namespace set_meta_tester {

int indent = 0;

void
writeLine(std::stringstream &ss, std::string line)
{
    int indent_count = 0;
    while (indent_count++ < indent)
        ss << "\t";
    ss << line << std::endl;
}

void
writeArgs(std::stringstream &ss, isl_tester::Arguments args,
   std::string meta_rel)
{
    writeLine(ss, "// Seed: " + std::to_string(args.seed));
    writeLine(ss, "// Max dims: " + std::to_string(args.max_dims));
    writeLine(ss, "// Max params: " + std::to_string(args.max_params));
    writeLine(ss, "// Max set count: " + std::to_string(args.max_set_count));
    writeLine(ss, "// Meta relation: " + meta_rel);
    //writeLine(ss, "// Generation date: " +
    writeLine(ss, "");
}

void
prepareHeader(std::stringstream &ss)
{
    std::vector<std::string> include_list = {
        "\"isl-noexceptions.h\"",
        "<cassert>",
        "<iostream>",
    };
    for (std::string incl : include_list)
        writeLine(ss, "#include " + incl);
}

void
mainPreSetup(std::stringstream &ss)
{
    writeLine(ss, "int main()");
    writeLine(ss, "{");
    indent++;
    writeLine(ss, "isl_ctx *ctx_ptr = isl_ctx_alloc();");
    writeLine(ss, "{");
    indent++;
    writeLine(ss, "isl::ctx ctx(ctx_ptr);");
}

void
genSetDeclaration(std::stringstream &ss, std::vector<std::string> &set_decl_calls)
{
    unsigned int i = 0;
    for (std::string decl_call : set_decl_calls) {
        //writeLine(ss, "std::cout << " + std::to_string(i++) + " << std::endl;");
        writeLine(ss, decl_call);
    }
}

void
genCoalesceSplitTest(std::stringstream &ss)
{
    writeLine(ss, "s = s.convex_hull();");
    writeLine(ss, "isl::point p = s.sample_point();");
    writeLine(ss, "isl::set split1 = s.upper_bound_si(isl::dim::set, 0, p.get_coordinate_val(isl::dim::set, 0).get_num_si());");
    writeLine(ss, "isl::set split2 = s.lower_bound_si(isl::dim::set, 0, p.get_coordinate_val(isl::dim::set, 0).get_num_si());");
    writeLine(ss, "isl::union_set split_us = isl::union_set(split1);");
    writeLine(ss, "split_us.add_set(split2);");
    writeLine(ss, "s = split_us.coalesce().sample();");
}

void
mainPostSetup(std::stringstream &ss)
{
    indent--;
    writeLine(ss, "}");
    writeLine(ss, "isl_ctx_free(ctx_ptr);");
    indent--;
    writeLine(ss, "}");
}

std::string
getGenerator(const YAML::Node generator_list, std::string type)
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
genMetaRelation(const YAML::Node relation_list, unsigned int count)
{
    std::queue<std::string> meta_relation;
    //std::cout << "REL ";
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
getMetaRelation(std::queue<std::string> meta_rel_queue)
{
    std::string string_rel = "";
    while (!meta_rel_queue.empty()) {
        string_rel += meta_rel_queue.front() + "-";
        meta_rel_queue.pop();
    }
    return string_rel;
}

std::string
getRelation(const YAML::Node relation_list, std::string relation_type)
{
    const YAML::Node selected_relation_list = relation_list[relation_type];
    return selected_relation_list[std::rand() % selected_relation_list.size()]
            .as<std::string>();
}

void
replaceMetaInputs(std::string &rel, const std::string input_var,
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
                    getGenerator(meta_list["generators"], "empty")); break;
                case 'u': rel.replace(pos, 2,
                    getGenerator(meta_list["generators"], "universe")); break;
                default:
                    std::cout << "Unknown input modifier %" << type << std::endl;
                    exit(1);
            }
    }
    if (rel[0] == '.')
        rel.insert(0, input_var);
}

std::string
genMetaFunc(const std::string input_var, std::string meta_relation,
    const YAML::Node meta_list)
{
    std::string new_rel = getRelation(meta_list["relations"], meta_relation);
    assert(new_rel.find("%1") != std::string::npos);
    //std::cout << new_rel << std::endl;
    replaceMetaInputs(new_rel, input_var, meta_list);
    //std::cout << new_rel << std::endl;
    return new_rel;
}

size_t
genMetaExpr(std::stringstream &ss, const unsigned int var_count, std::queue<std::string> meta_rel,
    const YAML::Node meta_list, std::set<size_t> gen_exprs)
{
    std::hash<std::string> string_hash_func;
    size_t new_expr_hash;
    std::queue<std::string> new_expr_strings;
    std::string input_var = "r" + std::to_string(var_count);
    unsigned int max_tries = 10;
    unsigned int curr_tries = 0;
    do {
        std::queue<std::string> meta_rel_copy = std::queue<std::string>(meta_rel);
        std::string new_expr = genMetaFunc("s", meta_rel_copy.front(), meta_list);
        new_expr_strings = std::queue<std::string>();
        new_expr_hash = string_hash_func(new_expr);
        new_expr_strings.push("isl::set " + input_var + " = " + new_expr + ";");
        meta_rel_copy.pop();
        while (!meta_rel_copy.empty()) {
            new_expr = genMetaFunc(input_var, meta_rel_copy.front(), meta_list);
            new_expr_strings.push(input_var + " = " + new_expr + ";");
            new_expr_hash += string_hash_func(new_expr);
            meta_rel_copy.pop();
        }
        curr_tries += 1;
    } while (gen_exprs.count(new_expr_hash) != 0 && curr_tries < max_tries);
    if (curr_tries >= max_tries)
        return -1;
    while (!new_expr_strings.empty()) {
        writeLine(ss, new_expr_strings.front());
        new_expr_strings.pop();
    }
    writeLine(ss, "assert(r0.is_equal(r" + std::to_string(var_count) + "));");
    return new_expr_hash;
}

void
runSimple(std::vector<std::string> set_decl_calls, isl_tester::Arguments &args)
{
    const std::string this_file_path = __FILE__;
    size_t curr_pos = -1, next_pos = 0;
    while (next_pos != std::string::npos) {
        curr_pos = next_pos;
        next_pos = this_file_path.find("/", curr_pos + 1);
    }
    const std::string this_file_dir = this_file_path.substr(0, curr_pos + 1);
    const std::string config_file_path = fmt::format("{}{}", this_file_dir,
        "../config_files/config.yaml");
    YAML::Node config_file = YAML::LoadFile(config_file_path);
    const std::string output_path = fmt::format("{}/{}",
        config_file["working_dir"].as<std::string>(),
        config_file["set_meta_tester"]["output_file"].as<std::string>());
    YAML::Node meta_list = YAML::LoadFile(fmt::format("{}/{}",
        config_file["working_dir"].as<std::string>(),
        config_file["set_meta_tester"]["meta_tests_file"].as<std::string>()));

    std::string variant = "single_distinct";
    const unsigned int variant_count = 20;
    std::set<size_t> generated_exprs = std::set<size_t>();

    unsigned int meta_rel_count = std::rand() % 7 + 1;
    std::string r1_expr, r2_expr;
    std::queue<std::string> meta_rel = genMetaRelation(
                                        meta_list["relations"], meta_rel_count);

    std::stringstream ss;
    writeArgs(ss, args, getMetaRelation(meta_rel));
    prepareHeader(ss);
    ss << std::endl;
    mainPreSetup(ss);
    genSetDeclaration(ss, set_decl_calls);
    //genCoalesceSplitTest(ss);
    for (int i = 0; i < variant_count; i++) {
        size_t result = genMetaExpr(ss, i, meta_rel, meta_list, generated_exprs);
        if (result == -1)
            break;
        generated_exprs.insert(result);
    }
    mainPostSetup(ss);

    std::ofstream ofs;
    ofs.open(output_path);
    ofs << ss.rdbuf();
    ofs.close();
}

}
