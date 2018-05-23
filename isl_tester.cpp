#include "isl_tester.hpp"

namespace isl_tester {

Arguments
parse_args(int argc, char **argv)
{
    Arguments args;
    int i = 1;
    while (i < argc) {
        if (!strcmp(argv[i], "--seed") || !strcmp(argv[i], "-s")) {
            args.seed = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "--mode") || !strcmp(argv[i], "-m")) {
            std::string mode_arg = argv[++i];
            if (!mode_arg.compare("SET_FUZZ"))
                args.mode = Modes::SET_FUZZ;
            else if (!mode_arg.compare("SET_TEST"))
                args.mode = Modes::SET_TEST;
            else if (!mode_arg.compare("SET_META"))
                args.mode = Modes::SET_META;
            else {
                std::cout << "Found unknown mode: " << mode_arg << std::endl;
                exit(1);
            }
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
        else {
            std::cout << "Found unknown argument: " << argv[i] << std::endl;
            exit(1);
        }
        i++;
    }
    return args;
}

}

int
main(int argc, char **argv)
{
    isl_tester::Arguments args = isl_tester::parse_args(argc, argv);
    std::srand(args.seed);
    isl_ctx *ctx_pointer = isl_ctx_alloc();
    isl::ctx ctx(ctx_pointer);

    if (args.mode == isl_tester::Modes::SET_FUZZ) {
        isl::set fuzzed_set = set_fuzzer::fuzz_set(ctx, args.max_dims,
                                args.max_params, args.max_set_count);
        std::cout << fuzzed_set.to_str() << std::endl;
    }
    else if (args.mode == isl_tester::Modes::SET_TEST) {
        isl::set set1 = set_fuzzer::fuzz_set(ctx, args.max_dims,
                            args.max_params, args.max_set_count);
        isl::set set2 = set_fuzzer::fuzz_set(ctx, args.max_dims,
                            args.max_params, args.max_set_count);
        std::cout << set1.to_str() << std::endl;
        std::cout << set2.to_str() << std::endl;
        set_tester::run_tests(set1, set2);
    }
    else if (args.mode == isl_tester::Modes::SET_META) {
        isl::set set1 = set_fuzzer::fuzz_set(ctx, args.max_dims,
                            args.max_params, args.max_set_count);
        std::cout << set1.to_str() << std::endl;
        set_meta_tester::run_simple(set1);
    }
    else {
        std::cout << "Unknown option " << argv[1] << std::endl;
        return 1;
    }

    ctx.release();
    isl_ctx_free(ctx_pointer);
    return 0;
}
