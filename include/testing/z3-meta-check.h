#ifndef Z3_META_CHECK_HPP
#define Z3_META_CHECK_HPP

#include "z3++.h"

bool
checkValid(z3::expr& e1, z3::expr& e2, z3::context& ctx)
{
    z3::solver solver(ctx);
    z3::expr conjecture = z3::operator==(e1, e2);
    solver.add(!conjecture);
    return solver.check() != z3::sat;
}

#endif
