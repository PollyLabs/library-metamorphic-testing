#include "test_emitter.hpp"

static int indent = 0;

std::map<std::string, Modes> string_to_mode {
    {"SET_FUZZ", SET_FUZZ},
    {"API_FUZZ", API_FUZZ},
    {"SET_TEST", SET_TEST},
    {"SET_META_STR", SET_META_STR},
    {"SET_META_API", SET_META_API},
    {"SET_META_NEW", SET_META_NEW},
};

Arguments
parseArgs(int argc, char **argv)
{
    Arguments args;
    int i = 1;
    while (i < argc) {
        if (!strcmp(argv[i], "--seed") || !strcmp(argv[i], "-s")) {
            args.seed = atoi(argv[++i]);
        }
        //else if (!strcmp(argv[i], "--mode") || !strcmp(argv[i], "-m")) {
            //std::string mode_arg = argv[++i];
            //std::map<std::string, Modes>::iterator mode_find =
                //string_to_mode.find(mode_arg);
            //if (mode_find == string_to_mode.end()) {
                //std::cout << "Found unknown mode: " << mode_arg << std::endl;
                //exit(1);
            //}
            //args.mode = mode_find->second;
        //}
        //else if (!strcmp(argv[i], "--dims")) {
            //args.max_dims = atoi(argv[++i]);
        //}
        //else if (!strcmp(argv[i], "--params")) {
            //args.max_params = atoi(argv[++i]);
        //}
        //else if (!strcmp(argv[i], "--set-count")) {
            //args.max_set_count = atoi(argv[++i]);
        //}
        //else if (!strcmp(argv[i], "--input-sets-file")) {
            //args.input_sets = argv[++i];
        //}
        else {
            std::cout << "Found unknown argument: " << argv[i] << std::endl;
            exit(1);
        }
        i++;
    }
    return args;
}

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
    Arguments args = parseArgs(argc, argv);
    std::mt19937* rng = new std::mt19937(args.seed);
    std::stringstream test_ss;
    std::string config_path =
        "/home/sentenced/Documents/Internships/2018_ETH/work/sets/config_files/api_fuzzer_isl.yaml";

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
    // ISL specific required instruction
    writeLine(test_ss, "isl_ctx *ctx_ptr = isl_ctx_alloc();");

    std::unique_ptr<ApiFuzzer> api_fuzzer (new ApiFuzzerNew(config_path, rng));
    for (std::string instr : api_fuzzer->getInstrStrs())
    {
        writeLine(test_ss, instr);
    }

    std::string meta_test_file = "../config_files/set_meta_tests.yaml";
    std::unique_ptr<SetMetaTester> smt = std::unique_ptr<SetMetaTester>(new SetMetaTester(meta_test_file, rng));
    for (std::string meta_instr : smt->getMetaExprStrs())
    {
        writeLine(test_ss, meta_instr);
    }
    //smt->genMetaTests();
    //
    mainPostSetup(test_ss);

    std::ofstream ofs;
    ofs.open("../out/test_new.cpp");
    ofs << test_ss.rdbuf();
    ofs.close();
}
