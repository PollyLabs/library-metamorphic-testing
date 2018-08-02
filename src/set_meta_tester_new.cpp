#ifndef SET_META_TESTER_NEW_HPP
#define SET_META_TESTER_NEW_HPP

#include <fstream>
#include <queue>

#include "yaml-cpp/yaml.h"
#include "fmt/format.h"


class SetMetaTester {
    public:
        SetMetaTester(std::vector<std::string>);
};

std::vector<std::string>
genMetaTests()
{
    /* Loading config files */
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

    const unsigned int variant_count = 20;
    std::set<size_t> generated_exprs = std::set<size_t>();

    unsigned int meta_rel_count = std::rand() % 7 + 1;
    std::string r1_expr, r2_expr;
    //std::queue<std::string> meta_rel = genMetaRelation(
                                        //meta_list["relations"], meta_rel_count);

    //std::stringstream ss;
    //writeArgs(ss, args, getMetaRelation(meta_rel));
    //prepareHeader(ss);
    //ss << std::endl;
    //mainPreSetup(ss);
    //genSetDeclaration(ss, set_decl_calls);
    ////genCoalesceSplitTest(ss);
    //for (int i = 0; i < variant_count; i++) {
        //size_t result = genMetaExpr(ss, i, meta_rel, meta_list, generated_exprs);
        //if (result == -1)
            //break;
        //generated_exprs.insert(result);
    //}
    //mainPostSetup(ss);

    //std::ofstream ofs;
    //ofs.open(output_path);
    //ofs << ss.rdbuf();
    //ofs.close();

}


#endif
