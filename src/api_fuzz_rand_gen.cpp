#include "api_fuzz_rand_gen.hpp"

std::map<std::string, std::vector<char>> char_set =
    {
     {"numeric", {'0','1','2','3','4','5','6','7','8','9'}},
     {"low_alpha", {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
                    'p','q','r','s','t','u','v','w','x','y','z'}},
     {"up_alpha", {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
                   'P','Q','R','S','T','U','V','W','X','Y','Z'}},
     //{"symbol", {'`','!','$','%','^','&','*','(',')','_','+','{','}','[',']',
                 //';',':','\'','@','#','~',',','<','.','>','/','?','|'}}
    };

int
ApiFuzzRandGen_mt19937::getRandInt(int min, int max)
{
    assert(max >= min);
    //return Random::get<int>(min, max);
    std::uniform_int_distribution<int> dist(min, max);
    return dist(*this->rng);
    //return (*this->rng)() % (max - min + 1) + min;
}

long
ApiFuzzRandGen_mt19937::getRandLong(long min, long max)
{
    assert(max >= min);
    //return Random::get<long>(min, max);
    std::uniform_int_distribution<long> dist(min, max);
    return dist(*this->rng);
    //return (*this->rng)() % (max - min + 1) + min;
}

double
ApiFuzzRandGen_mt19937::getRandDouble(double min, double max)
{
    assert(max >= min);
    //return Random::get<double>(min, max);
    std::uniform_real_distribution<double> dist(min, max);
    return dist(*this->rng);
}

float
ApiFuzzRandGen_mt19937::getRandFloat(float min, float max)
{
    assert(max >= min);
    //return Random::get<float>(min, max);
    std::uniform_real_distribution<float> dist(min, max);
    return dist(*this->rng);
}

std::string
ApiFuzzRandGen_mt19937::getRandString(uint8_t min_len, uint8_t max_len)
{
    std::string rand_str = "";
    std::uniform_int_distribution<uint8_t> dist(min_len, max_len);
    uint8_t str_len = dist(*this->rng);
    std::string str_type = "low_alpha";
    std::uniform_int_distribution<uint8_t> chr_dist(0, char_set.at(str_type).size() - 1);
    for (int i = 0; i < str_len; ++i)
    {
        rand_str += char_set.at(str_type).at(chr_dist(*this->rng));
        //rand_str += Random::get('a', 'z')
    }
    return "\"" + rand_str + "\"";
}

