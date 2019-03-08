#ifndef GMP_HELPERS
#define GMP_HELPERS

#include "gmpxx.h"


mpz_class
get_one(mpz_class e)
{
    return e/e;
}


#endif
