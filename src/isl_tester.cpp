#include "isl_tester.hpp"

namespace isl_tester {

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
        else if (!strcmp(argv[i], "--mode") || !strcmp(argv[i], "-m")) {
            std::string mode_arg = argv[++i];
            std::map<std::string, Modes>::iterator mode_find =
                string_to_mode.find(mode_arg);
            if (mode_find == string_to_mode.end()) {
                std::cout << "Found unknown mode: " << mode_arg << std::endl;
                exit(1);
            }
            args.mode = mode_find->second;
        }
        else if (!strcmp(argv[i], "--dims")) {
            args.max_dims = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "--params")) {
            args.max_params = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "--set-count")) {
            args.max_set_count = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "--input-sets-file")) {
            args.input_sets = argv[++i];
        }
        else {
            std::cout << "Found unknown argument: " << argv[i] << std::endl;
            exit(1);
        }
        i++;
    }
    return args;
}

std::vector<std::string>
gatherSets(std::string file_path)
{
    std::vector<std::string> input_sets = std::vector<std::string>();
    std::string line_buffer;
    std::ifstream input_file(file_path.c_str());
    assert(input_file.good() && "Could not open given file.");
    while (std::getline(input_file, line_buffer))
        input_sets.push_back(line_buffer);
    return input_sets;
}

isl::set
retrieveSet(isl::ctx ctx, std::vector<std::string> input_sets)
{
    //return isl::set(ctx, input_sets[std::rand() % input_sets.size()]);
}

std::vector<std::string>
generateSetDeclFromObj(isl::set set_in, std::string set_var_name)
{
    std::string set_decl = fmt::format("isl::set {} (ctx, {});", set_var_name,
        set_in.to_str());
    return std::vector<std::string>{set_decl};
}

}

int
main(int argc, char **argv)
{
    isl_tester::Arguments args = isl_tester::parseArgs(argc, argv);
    std::mt19937 rng(args.seed);
    isl_ctx *ctx_pointer = isl_ctx_alloc();
    isl::ctx ctx(ctx_pointer);

    if (args.mode == isl_tester::Modes::SET_FUZZ) {
        isl::set fuzzed_set = set_fuzzer::fuzz_set(ctx, args.max_dims,
                            args.max_params, args.max_set_count);
        std::cout << fuzzed_set.to_str() << std::endl;
    }
    else if (args.mode == isl_tester::Modes::API_FUZZ) {
        //ApiFuzzer *api_fuzzer = new ApiFuzzerISL(args.max_dims,
            //args.max_params, args.max_set_count);
        //api_fuzzer->generateSet();
        //for (std::string s : api_fuzzer->getInstrList())
            //std::cout << s << std::endl;
    }
    else if (args.mode == isl_tester::Modes::SET_TEST) {
        //isl::set set1, set2;
        //if (args.input_sets != "") {
            //std::vector<std::string> input_sets = isl_tester::gatherSets(args.input_sets);
            //set1 = isl_tester::retrieveSet(ctx, input_sets);
            //set2 = isl_tester::retrieveSet(ctx, input_sets);
        //} else {
            //set1 = set_fuzzer::fuzz_set(ctx, args.max_dims,
                                //args.max_params, args.max_set_count);
            //set2 = set_fuzzer::fuzz_set(ctx, args.max_dims,
                                //args.max_params, args.max_set_count);
        //}
        //std::cout << set1.to_str() << std::endl;
        //std::cout << set2.to_str() << std::endl;
        //set_tester::run_tests(set1, set2);
    }
    else if (args.mode == isl_tester::Modes::SET_META_STR) {
        //isl::set set1;
        //if (args.input_sets != "") {
            //std::vector<std::string> input_sets = isl_tester::gatherSets(args.input_sets);
            //set1 = isl_tester::retrieveSet(ctx, input_sets);
        //} else {
            //set1 = set_fuzzer::fuzz_set(ctx, args.max_dims,
                                //args.max_params, args.max_set_count);
        //}
        //std::cout << set1.to_str() << std::endl;
        //set_meta_tester::runSimple(
            //isl_tester::generateSetDeclFromObj(set1, "s"), args);
    }
    else if (args.mode == isl_tester::Modes::SET_META_API) {
        //std::unique_ptr<ApiFuzzer> api_fuzzer (new ApiFuzzerISL(args.max_dims,
            //args.max_params, args.max_set_count));
        //std::unique_ptr<const ApiObject> fuzzed_set =
            //std::unique_ptr<const ApiObject>(api_fuzzer->generateSet());
        //std::vector<std::string> set_decl_calls = api_fuzzer->getInstrList();
        //set_decl_calls.push_back("isl::set s = isl::set(" + fuzzed_set->toStr() + ");");
        //set_meta_tester::runSimple(set_decl_calls, args);
    }
    else if (args.mode == isl_tester::Modes::SET_META_NEW) {
        std::string config_path = "/home/sentenced/Documents/Internships/2018_ETH/work/sets/config_files/api_fuzzer_isl_point.yaml";
        std::unique_ptr<ApiFuzzer> api_fuzzer (new ApiFuzzerNew(config_path, rng));
        std::vector<std::string> set_decl_calls = api_fuzzer->getInstrList();
        set_meta_tester::runSimple(set_decl_calls, args);
    }
    else {
        std::cout << "Unknown option " << argv[1] << std::endl;
        return 1;
    }

    ctx.release();
    isl_ctx_free(ctx_pointer);
    return 0;
}
