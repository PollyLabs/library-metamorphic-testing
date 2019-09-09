#ifndef Z3_META_CHECK_HPP
#define Z3_META_CHECK_HPP

#include "z3++.h"

z3::expr
val_to_expr(int val, z3::expr expr)
{
    return expr.ctx().int_val(val);
}

z3::expr
get_one(z3::expr e)
{
    return val_to_expr(1, e);
}

z3::expr
get_zero(z3::expr e)
{
    return val_to_expr(0, e);
}

z3::expr
expr_abs(z3::expr expr)
{
    return ite(expr < 0, -expr, expr);
}

namespace wrap {

/*******************************************************************************
 * Division wrap
 ******************************************************************************/

z3::expr
div(z3::expr e1, z3::expr e2)
{
    return ite(e2 != 0, e1/e2, e1);
}

z3::expr
div(int i, z3::expr e)
{
    return wrap::div(val_to_expr(i, e), e);
}

z3::expr
div(z3::expr e, int i)
{
    return wrap::div(e, val_to_expr(i, e));
}

/*******************************************************************************
 * Power wrap
 ******************************************************************************/

z3::expr
pw(z3::expr e1, z3::expr e2)
{
    return ite(e1 != 0 && e2 != 0, z3::pw(e1, e2), e1);
}

z3::expr
pw(int i, z3::expr e)
{
    return wrap::pw(val_to_expr(i, e), e);
}

z3::expr
pw(z3::expr e, int i)
{
    return wrap::pw(e, val_to_expr(i, e));
}

} // namespace wrap

/*******************************************************************************
 * Metamorphic check function
 ******************************************************************************/

bool
checkValid(z3::expr& e1, z3::expr& e2)
{
    z3::solver solver(e1.ctx());
    z3::expr conjecture = z3::operator==(e1, e2);
    solver.add(!conjecture);
    return solver.check() != z3::sat;
}

#endif
