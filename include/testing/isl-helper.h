#include "isl-noexceptions.h"

isl::pw_aff
divWrap(isl::pw_aff pw1, isl::pw_aff pw2)
{
    isl::val zero_val = isl::val(pw1.get_ctx(), 0);
    isl::pw_aff zero = isl::pw_aff(pw1.domain(), zero_val);
    if (!pw2.is_cst() || pw2.is_equal(zero))
    {
        return pw1;
    }
    return pw1.div(pw2);
}

isl::pw_aff
modWrap(isl::pw_aff pw1, isl::val val2)
{
    if (val2.get_num_si() <= 0)
    {
        return pw1;
    }
    return pw1.mod(val2);
}
