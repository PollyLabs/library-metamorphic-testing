#ifndef API_FUZZ_RAND_GEN_HPP
#define API_FUZZ_RAND_GEN_HPP

#include <cassert>
#include <map>
#include <random>
#include <string>

extern std::map<std::string, std::vector<char>> char_set;

class ApiFuzzRandGen {
    public:
        virtual int getRandInt(int, int) = 0;
        virtual long getRandLong(long, long) = 0;
        virtual double getRandDouble(double, double) = 0;
        virtual float getRandFloat(float, float) = 0;
        virtual std::string getRandString(uint8_t, uint8_t) = 0;
};

class ApiFuzzRandGen_mt19937 : public ApiFuzzRandGen {
    private:
        std::mt19937* rng;

    public:
        ApiFuzzRandGen_mt19937(std::mt19937* _rng) : rng(_rng) {} ;

        int getRandInt(int, int);
        long getRandLong(long, long);
        double getRandDouble(double, double);
        float getRandFloat(float, float);
        std::string getRandString(uint8_t, uint8_t);
};

#endif
