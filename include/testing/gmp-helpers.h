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

mpq_class 
mpq_class_w(mpz_class a, mpz_class b)
{
    mpq_class x = mpq_class(a,b);
    x.canonicalize();
    return x;
}

mpz_class 
get_zero_int()
{
    mpz_class x(0);
    return x;
}

mpq_class
canonicalize_w(mpq_class e)
{
    mpq_class x = mpq_class(e);
    x.canonicalize();
    return x;
}




#endif
