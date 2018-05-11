#include "set_tester.hpp"

namespace set_tester {

void
run_unary_tests(isl::set set)
{
    set.complement();
    set.project_out(isl::dim::set, 0, 0);
    set.project_out(isl::dim::param, 0, 0);
    set.eliminate(isl::dim::set, 0, 0);
    set.eliminate(isl::dim::param, 0, 0);
    set.from_params();
    set.fix_si(isl::dim::set, 0, std::rand());
    set.fix_val(isl::dim::param, 0, isl::val(set.get_ctx(), std::rand()));
    set.identity();
    set.indicator_function();
    set.coalesce();
    set.detect_equalities();
    set.remove_redundancies();
    set.convex_hull();
    set.simple_hull();
    set.affine_hull();
    set.polyhedral_hull();
    set.drop_constraints_involving_dims(isl::dim::set, 0, 0);
    set.drop_constraints_involving_dims(isl::dim::param, 0, 0);
    set.drop_constraints_not_involving_dims(isl::dim::set, 0, 0);
    set.drop_constraints_not_involving_dims(isl::dim::param, 0, 0);
    set.sample();
    set.sample_point();
    //set.min_val();
    //set.max_val();
    set.dim_max(std::rand());
    set.dim_min(std::rand());
    set.solutions();
    set.coefficients();
    set.unwrap();
    set.flatten();
    //set.lift();
    //set.align_params();
    set.neg();
    set.insert_dims(isl::dim::set, 0, std::rand());
    set.insert_dims(isl::dim::param, 0, std::rand());
    set.add_dims(isl::dim::set, 0);
    set.add_dims(isl::dim::param, 0);
    set.move_dims(isl::dim::set, 0, isl::dim::param, 0, 0);
    set.move_dims(isl::dim::param, 0, isl::dim::set, 0, 0);

}

int
run_tests (isl::set set1, isl::set set2, isl::set set3)
{
    std::srand(42);
    run_unary_tests(set1);
    run_unary_tests(set2);
    run_unary_tests(set3);
    return 0;
}

}
