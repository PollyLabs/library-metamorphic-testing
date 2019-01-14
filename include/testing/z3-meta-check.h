#ifndef Z3_META_CHECK_HPP
#define Z3_META_CHECK_HPP

#include "z3++.h"

z3::expr
val_to_expr(int val, z3::expr expr)
{
    return expr.ctx().num_val(val, expr.get_sort());
}

z3::expr
abs(z3::expr expr)
{
    return ite(expr < 0, -expr, expr);
}

z3::expr
divWrap(z3::expr e1, z3::expr e2)
{
    return ite(e2 != 0, e1/e2, e1);
}

z3::expr
divWrap(int i, z3::expr e)
{
    return divWrap(val_to_expr(i, e), e);
}

z3::expr
divWrap(z3::expr e, int i)
{
    return divWrap(e, val_to_expr(i, e));
}

bool
checkValid(z3::expr& e1, z3::expr& e2)
{
    z3::solver solver(e1.ctx());
    z3::expr conjecture = z3::operator==(e1, e2);
    solver.add(!conjecture);
    return solver.check() != z3::sat;
}

#endif
