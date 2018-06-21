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
    //set.from_params(); // not a parameter space -- need 0-dim set
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
    //set.min_val(); // TODO Need affine expression generator
    //set.max_val();
    set.dim_max(0); // unbounded optimum -- is_bounded
    set.dim_min(0);
    set.solutions(); // not a wrapping space
    set.coefficients();
    set.unwrap(); // not a wrapping set
    set.flatten();
    // set.lift(); // missing func or deprecated?
    set.align_params(set.get_space()); // model has unnamed parameters
    set.neg();
    set.insert_dims(isl::dim::set, 0, std::rand() % 50); // runs out of memory; previously constructed sets not cleared?
    set.insert_dims(isl::dim::param, 0, std::rand() % 50);
    set.add_dims(isl::dim::set, 0);
    set.add_dims(isl::dim::param, 0);
    set.move_dims(isl::dim::set, 0, isl::dim::param, 0, 0);
    set.move_dims(isl::dim::param, 0, isl::dim::set, 0, 0);
    set.lexmin(); // should take second set as argument?
    set.lexmax(); // unbounded optimum
}

void
run_binary_tests(isl::set set1, isl::set set2)
{
    set1.intersect(set2);
    set1.intersect_params(set2);
    set1.unite(set2);
    set1.subtract(set2);
    //set1.preimage_multi_aff();
    //set1.preimage_pw_multi_aff();
    //set1.preimage_multi_pw_aff();
    set1.product(set2);
    set1.flat_product(set2);
    set1.gist(set2);
    set1.gist_params(set2);
    set1.sum(set2);
    //set1.partial_lexmin(set2) // missing
}

int
run_tests (isl::set set1, isl::set set2)
{
    //unsigned long long mem_size = (unsigned long long) 8 * 1024 * 1024 * 1024;
    //struct rlimit rl;
    //rl.rlim_cur = mem_size;
    //setrlimit(RLIMIT_AS, &rl);

    std::srand(42);
    run_unary_tests(set1);
    //run_unary_tests(set2);
    run_binary_tests(set1, set1);
    run_binary_tests(set2, set2);
    //run_binary_tests(set1, set2);
    //run_binary_tests(set2, set1);

    return 0;
}

}
