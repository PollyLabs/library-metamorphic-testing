#include "isl_tester.hpp"

enum Modes {
    SET_FUZZ,
    SET_TEST,
    SET_META,
};

int
main(int argc, char **argv)
{
    std::srand(43);
    isl_ctx *ctx_pointer = isl_ctx_alloc();
    isl::ctx ctx(ctx_pointer);

    if (argc != 2) {
        std::cout << "Expected 1 argument, test mode!" << std::endl;
        return 1;
    }
    if (!strcmp(argv[1], "SET_FUZZ")) {
        isl::set fuzzed_set = set_fuzzer::fuzz_set(ctx);
        std::cout << fuzzed_set.to_str() << std::endl;
        //isl_ctx_free(fuzzed_set.get_ctx().get());
    }
    else if (!strcmp(argv[1], "SET_TEST")) {
        set_fuzzer::Arguments args;
        args = { std::rand(), std::rand() % 20 + 1, std::rand() % 20 + 1, std::rand() % 5 };
        isl::set set1 = set_fuzzer::fuzz_set(ctx, args);
        args = { std::rand(), std::rand() % 20 + 1, std::rand() % 20 + 1, std::rand() % 5 };
        isl::set set2 = set_fuzzer::fuzz_set(ctx, args);
        std::cout << set1.to_str() << std::endl;
        std::cout << set2.to_str() << std::endl;
        set_tester::run_tests(set1, set2);
    }
    else if (!strcmp(argv[1], "SET_META")) {
        set_fuzzer::Arguments args;
        args = { std::rand(), std::rand() % 20 + 1, std::rand() % 20 + 1, std::rand() % 5 };
        isl::set set1 = set_fuzzer::fuzz_set(ctx, args);
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
