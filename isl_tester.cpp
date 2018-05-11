#include "isl_tester.hpp"

enum Modes {
    SET_FUZZ,
    SET_TEST,
};

int
main(int argc, char **argv)
{
    if (argc != 2) {
        std::cout << "Expected 1 argument, test mode!" << std::endl;
        return 1;
    }
    if (!strcmp(argv[1], "SET_FUZZ")) {
        isl::set fuzzed_set = set_fuzzer::fuzz_set();
        std::cout << fuzzed_set.to_str() << std::endl;
    }
    else if (!strcmp(argv[1], "SET_TEST")) {
        std::srand(42);
        set_fuzzer::Arguments args;
        args = { std::rand(), std::rand() % 20 + 1, std::rand() % 20 + 1, std::rand() % 5 };
        isl::set set1 = set_fuzzer::fuzz_set(args);
        args = { std::rand(), std::rand() % 20 + 1, std::rand() % 20 + 1, std::rand() % 5 };
        isl::set set2 = set_fuzzer::fuzz_set(args);
        args = { std::rand(), std::rand() % 20 + 1, std::rand() % 20 + 1, std::rand() % 5 };
        isl::set set3 = set_fuzzer::fuzz_set(args);
        std::cout << set1.to_str() << std::endl;
        std::cout << set2.to_str() << std::endl;
        std::cout << set3.to_str() << std::endl;
        set_tester::run_tests(set1, set2, set3);
    }
    else {
        std::cout << "Unknown option " << argv[1] << std::endl;
        return 1;
    }
    return 0;
}
