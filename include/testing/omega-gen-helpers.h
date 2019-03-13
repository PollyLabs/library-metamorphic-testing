#include <random>
#include "omega.h"

int
getRandInt(std::mt19937& rng, int min, int max)
{
    return rng() % (max - min + 1) + min;
}

omega::Relation
makeTupleRelation(int first, int second)
{
    omega::Relation rel_out(2);
    omega::F_And* f_and = rel_out.add_and();
    omega::EQ_Handle eq = f_and->add_EQ();
    eq.update_coef(rel_out.input_var(1), 1);
    eq.update_const(-1 * first);
    eq = f_and->add_EQ();
    eq.update_coef(rel_out.input_var(2), 1);
    eq.update_const(-1 * second);
    return rel_out;
}

omega::Relation
makeRelation(unsigned int seed, unsigned int dim_cnt)
{
    std::mt19937 rng(seed);
    omega::Relation rel_out = omega::Relation(dim_cnt);
    omega::F_And* f_and = rel_out.add_and();
    for (int i = 0; i < rel_out.n_set(); ++i)
    {
        omega::EQ_Handle eq = f_and->add_EQ();
        eq.update_const(-1 * getRandInt(rng, 1, 10));
        eq.update_coef(rel_out.input_var(i + 1), 1);
    }
    return rel_out;
}

