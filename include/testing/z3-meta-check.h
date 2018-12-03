#ifndef Z3_META_CHECK_HPP
#define Z3_META_CHECK_HPP

#include "z3++.h"

z3::expr
ite(bool cond, z3::expr if_then, z3::expr if_else)
{
    if (cond)
    {
        return if_then;
    }
    return if_else;
}

z3::expr
divWrap(z3::expr e1, z3::expr e2)
{
    return ite(e2 != 0, e1/e2, e1);
}

z3::expr
divWrap(int i, z3::expr e)
{
    return ite(e != 0, i/e, e.ctx().num_val(i, e.get_sort()));
}

z3::expr
divWrap(z3::expr e, int i)
{
    return ite(i != 0, e/i, e);
}

bool
checkValid(z3::expr& e1, z3::expr& e2, z3::context& ctx)
{
    z3::solver solver(ctx);
    z3::expr conjecture = z3::operator==(e1, e2);
    solver.add(!conjecture);
    return solver.check() != z3::sat;
}

#endif
