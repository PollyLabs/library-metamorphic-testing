#include "isl-noexceptions.h"
#include "set_fuzzer.hpp"

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
    else {
        std::cout << "Unknown option " << argv[1] << std::endl;
        return 1;
    }
    return 0;
}
