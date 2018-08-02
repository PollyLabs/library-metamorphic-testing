#include "test_emitter.hpp"

static int indent = 0;

void
writeLine(std::stringstream &ss, std::string line)
{
    int indent_count = 0;
    while (indent_count++ < indent)
        ss << "\t";
    ss << line << std::endl;
}

void
prepareHeader(std::stringstream &ss, std::vector<std::string> &include_list)
{
    for (std::string incl : include_list)
        writeLine(ss, "#include " + incl);
}

void
mainPreSetup(std::stringstream &ss)
{
    writeLine(ss, "int main()");
    writeLine(ss, "{");
    indent++;
}

void
mainPostSetup(std::stringstream &ss)
{ indent--;
    writeLine(ss, "}");
}

int
main(int argc, char** argv)
{
    Arguments args;
    std::mt19937 rng(args.seed);
    std::stringstream test_ss;
    std::string config_path = "/home/sentenced/Documents/Internships/2018_ETH/work/sets/config_files/api_fuzzer_ppl.yaml";

    YAML::Node config_file = YAML::LoadFile(config_path);
    std::vector<std::string> include_list = {
        "<cassert>",
        "<iostream>",
    };
    if (config_file["includes"].IsDefined())
    {
        for (YAML::Node include_yaml : config_file["includes"])
        {
            include_list.push_back(fmt::format("\"{}\"",
                include_yaml.as<std::string>()));
        }
    }

    //writeArgs(ss, args, getMetaRelation(meta_rel));
    prepareHeader(test_ss, include_list);
    mainPreSetup(test_ss);

    std::unique_ptr<ApiFuzzer> api_fuzzer (new ApiFuzzerNew(config_path, rng));
    std::vector<std::string> set_decl_calls = api_fuzzer->getInstrList();
    for (std::string instr : api_fuzzer->getInstrList())
    {
        writeLine(test_ss, instr);
    }

    //std::unique_ptr<SetMetaTester> smt = new SetMetaTester();
    //smt->genMetaTests();
    //
    mainPostSetup(test_ss);

    std::ofstream ofs;
    ofs.open("../out/test_new.cpp");
    ofs << test_ss.rdbuf();
    ofs.close();
}
