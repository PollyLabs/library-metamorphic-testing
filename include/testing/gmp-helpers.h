#ifndef GMP_HELPERS
#define GMP_HELPERS

#include "gmpxx.h"


mpz_class
get_one(mpz_class e)
{
    if (e==0) e = e+1;
    return e/e;
}

mpq_class
get_one(mpq_class e)
{
    if (e==0) e = e+1;
    return e/e;
}

mpz_class
get_minus_one(mpz_class e)
{
    if (e==0) e = e+1;
    return -(e/e);
}

mpq_class
get_minus_one(mpq_class e)
{
    if (e==0) e = e+1;
    return -(e/e);
}




#endif
