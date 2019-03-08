#include <random>

const char char_set[] =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "`¬|!\"£$%^&*()-=_+[];'#,./{}:@~<>?";

std::string
make_string(size_t len)
{
    std::string new_str(len, 0);
    std::unique_ptr<std::mt19937> rng(new std::mt19937(42));
    std::generate_n(new_str.begin(), len, [&rng]()
        {
           return char_set[(*rng)() % sizeof(char_set)];
        });
    return new_str;
}

bool
is_equal_string(std::string str1, std::string other)
{
    return !str1.compare(other);
}
