#include "test_emitter.hpp"

static unsigned int indent = 0;
const bool DEBUG = false;
//const bool DEBUG = true;

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
	else if (!strcmp(argv[i], "--output") || !strcmp(argv[i], "-o"))
	{
	    args.output_file = argv[++i];
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
prepareHeader(std::stringstream &ss, std::vector<std::string> &include_list,
    Arguments& args, std::string api_file, std::string meta_file)
{
    writeLine(ss, fmt::format("// SEED : {}", args.seed));
    writeLine(ss, fmt::format("// API CONFIG FILE : {}", api_file));
    writeLine(ss, fmt::format("// META CONFIG FILE : {}", meta_file));
    std::time_t curr_time_t = std::time(nullptr);
    writeLine(ss, fmt::format("// GENERATION TIME : {}",
        std::ctime(&curr_time_t)));
    writeLine(ss, "");
    for (std::string incl : include_list)
        writeLine(ss, "#include " + incl);
}

void
mainPreSetup(std::stringstream &ss, std::vector<std::string>& pre_setup_instrs)
{
    writeLine(ss, "int main()");
    writeLine(ss, "{");
    for (std::string pre_setup_instr : pre_setup_instrs)
    {
        writeLine(ss, pre_setup_instr);
    }
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
        "/home/isl_testing/isl-metamorphic-testing/config_files/api_fuzzer_isl_point.yaml";
    std::string meta_test_file =
        "/home/isl_testing/isl-metamorphic-testing/config_files/set_meta_tests_isl.yaml";

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
    prepareHeader(test_ss, include_list, args, config_path, meta_test_file);
    std::vector<std::string> pre_setup_instrs;
    if (config_file["pre_setup"].IsDefined())
    {
        for (YAML::Node pre_setup_yaml : config_file["pre_setup"])
        {
            pre_setup_instrs.push_back(pre_setup_yaml.as<std::string>());
        }
    }
    mainPreSetup(test_ss, pre_setup_instrs);
    std::unique_ptr<SetMetaTester> smt (new SetMetaTester(meta_test_file, rng));

    std::unique_ptr<ApiFuzzer> api_fuzzer (
        new ApiFuzzerNew(config_path, args.seed, rng, std::move(smt)));
    for (std::string instr : api_fuzzer->getInstrStrs())
    {
        writeLine(test_ss, instr);
    }

    //for (std::string meta_instr : smt->getMetaExprStrs())
    //{
        //writeLine(test_ss, meta_instr);
    //}
    mainPostSetup(test_ss);

    std::ofstream ofs;
    ofs.open(args.output_file);
    ofs << test_ss.rdbuf();
//    printf(test_ss.rdbuf()->str().c_str());
    ofs.close();
}
