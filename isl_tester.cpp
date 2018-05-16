#include "isl_tester.hpp"

enum Modes {
    SET_FUZZ,
    SET_TEST,
    SET_META,
};

int
main(int argc, char **argv)
{
    std::srand(42);
    if (argc != 2) {
        std::cout << "Expected 1 argument, test mode!" << std::endl;
        return 1;
    }
    if (!strcmp(argv[1], "SET_FUZZ")) {
        isl::set fuzzed_set = set_fuzzer::fuzz_set();
        std::cout << fuzzed_set.to_str() << std::endl;
        //isl_ctx_free(fuzzed_set.get_ctx().get());
    }
    else if (!strcmp(argv[1], "SET_TEST")) {
        set_fuzzer::Arguments args;
        args = { std::rand(), std::rand() % 20 + 1, std::rand() % 20 + 1, std::rand() % 5 };
        isl::set set1 = set_fuzzer::fuzz_set(args);
        args = { std::rand(), std::rand() % 20 + 1, std::rand() % 20 + 1, std::rand() % 5 };
        isl::set set2 = set_fuzzer::fuzz_set(args);
        std::cout << set1.to_str() << std::endl;
        std::cout << set2.to_str() << std::endl;
        set_tester::run_tests(set1, set2);
    }
    else if (!strcmp(argv[1], "SET_META")) {
        set_fuzzer::Arguments args;
        args = { std::rand(), std::rand() % 20 + 1, std::rand() % 20 + 1, std::rand() % 5 };
        isl::set set1 = set_fuzzer::fuzz_set(args);
        std::cout << set1.to_str() << std::endl;
        set_meta_tester::run_simple(set1);
    }
    else {
        std::cout << "Unknown option " << argv[1] << std::endl;
        return 1;
    }
    return 0;
}
