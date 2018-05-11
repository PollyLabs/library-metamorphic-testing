#include "isl-noexceptions.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <list>
#include <tuple>
#include <string>
#include <limits>
#include <experimental/random>
#include <vector>
#include <queue>

typedef struct {
    const unsigned int dims;
    const unsigned int params;
    const isl::ctx *ctx;
    const isl::space *space;
    const isl::local_space *ls;
} Parameters;

typedef struct {
    unsigned int seed;
    unsigned int max_dims;
    unsigned int max_params;
    unsigned int max_set_count;
} Arguments;

Arguments
parse_args(int argc, char **argv)
{
    Arguments args = { 42, 20, 20, 10 };
    int i = 1;
    while (i < argc) {
        if (!strcmp(argv[i], "--dims")) {
            args.max_dims = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "--params")) {
            args.max_params = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "--seed")) {
            args.seed = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "--set-count")) {
            args.max_set_count = atoi(argv[++i]);
        }
        else {
            std::cout << "Found unknown argument: " << argv[i] << std::endl;
            exit(1);
        }
        i++;
    }
    return args;
}

std::vector<isl::pw_aff>
generate_dims(isl::local_space ls, isl::dim type)
{
    std::vector<isl::pw_aff> dim_affs;
    for (int i = 0; i < ls.dim(type); i++)
        dim_affs.push_back(isl::pw_aff::var_on_domain(ls, type, i));
    return dim_affs;
}

/* Possible operations for expression generation
 */
struct {
    isl::pw_aff (isl::pw_aff::*op)(isl::pw_aff other) const;
} expr_ops[] = {
    { &isl::pw_aff::add },
    { &isl::pw_aff::sub },
};

isl::pw_aff
generate_expr(isl::pw_aff &input, const Parameters &pars)
{
    if (std::rand() % 2) {
        input = input.mul(isl::pw_aff(isl::set::universe(*pars.space),
                                      isl::val(*pars.ctx, std::rand() % 42)));
    }
    return input;
}

isl::pw_aff
generate_expr(std::vector<isl::pw_aff> inputs, const Parameters &pars)
{
    isl::pw_aff new_expr = generate_expr(inputs.back(), pars);
    inputs.pop_back();
    while (!inputs.empty()) {
        //std::cout << "EXPR " << new_expr.to_str() << std::endl;
        //std::cout << "FRONT " << inputs.front().to_str() << std::endl;
        isl::pw_aff (isl::pw_aff::*op)(isl::pw_aff) const =
            expr_ops[std::rand() % (sizeof(expr_ops) / sizeof(expr_ops)[0])].op;
        new_expr = (new_expr.*op)(generate_expr(inputs.back(), pars));

        inputs.pop_back();
    }
    return new_expr;
}

/* Randomly split the given dimensions into two
 */
void
split_dims(const std::vector<isl::pw_aff> &dims,
    std::vector<isl::pw_aff> &lhs_vars,
    std::vector<isl::pw_aff> &rhs_vars)
{
    do {
        for (isl::pw_aff var : dims) {
            int x = std::rand() % 2;
            if (x)
                lhs_vars.push_back(var);
            else
                rhs_vars.push_back(var);
        }
    } while (lhs_vars.empty() ||  rhs_vars.empty());
}

/* Possible operations used to create a set from pw_aff's
 */
struct {
    isl::set (isl::pw_aff::*op)(isl::pw_aff other) const;
} gen_set_ops[] = {
    { &isl::pw_aff::le_set },
    { &isl::pw_aff::lt_set },
    { &isl::pw_aff::ge_set },
    { &isl::pw_aff::gt_set },
    { &isl::pw_aff::eq_set },
    { &isl::pw_aff::ne_set },
};

isl::set
generate_set(const Parameters &pars)
{
    // Generate list of identifiers
    std::vector<isl::pw_aff> dims = generate_dims(*pars.ls, isl::dim::param);
    std::vector<isl::pw_aff> params = generate_dims(*pars.ls, isl::dim::set);
    std::vector<isl::pw_aff> all_dims(dims);
    all_dims.insert(all_dims.end(), params.begin(), params.end());

    // Generate expressions
    std::vector<isl::pw_aff> lhs_dims;
    std::vector<isl::pw_aff> rhs_dims;
    split_dims(all_dims, lhs_dims, rhs_dims);
    /*for (isl::pw_aff pa : lhs_dims)
        std::cout << "LHSDIM " << pa.to_str() << std::endl;
    for (isl::pw_aff pa : rhs_dims)
        std::cout << "RHSDIM " << pa.to_str() << std::endl;*/
    isl::pw_aff lhs = generate_expr(lhs_dims, pars);
    isl::pw_aff rhs = generate_expr(rhs_dims, pars);
    //std::cout << "LHS " << lhs.to_str() << std::endl;
    //std::cout << "RHS " << rhs.to_str() << std::endl;

    // Create set
    isl::set (isl::pw_aff::*op)(isl::pw_aff) const =
        gen_set_ops[std::rand() % (sizeof(gen_set_ops) /
                                   sizeof(gen_set_ops)[0])].op;
    isl::set generated_set = (lhs.*op)(rhs);
    std::cout << "GSET " << generated_set.to_str() << std::endl;
    return generated_set;
}

int
main(int argc, char **argv)
{
    Arguments args = parse_args(argc, argv);

    isl_ctx *ctx_ptr = isl_ctx_alloc();
    const isl::ctx ctx(ctx_ptr);
    std::srand(args.seed);

    const unsigned int dims = std::rand() % args.max_dims + 1;
    const unsigned int params = std::rand() % args.max_params + 1;
    const unsigned int set_count = std::rand() % args.max_set_count + 1;
    //std::cout << "--Dims: " << dims << std::endl;
    //std::cout << "--Params: " << params << std::endl;
    const isl::space space = isl::space(ctx, dims, params);
    const isl::local_space ls = isl::local_space(space);
    Parameters pars = { dims, params, &ctx, &space, &ls };
    //std::cout << "All: " << ls.dim(isl::dim::all) << std::endl;
    //std::cout << "Param: " << ls.dim(isl::dim::param) << std::endl;
    //std::cout << "Set: " << ls.dim(isl::dim::set) << std::endl;
    //std::cout << "Div: " << ls.dim(isl::dim::div) << std::endl;
    //std::cout << "In: " << ls.dim(isl::dim::in) << std::endl;
    //std::cout << "Out: " << ls.dim(isl::dim::out) << std::endl;
    //std::cout << "CST: " << ls.dim(isl::dim::cst) << std::endl;

    isl::set final_set = isl::set::universe(space);
    for (int i = 0; i < set_count; i++)
        final_set = final_set.intersect(generate_set(pars));
    std::cout << "FSET " << final_set.to_str() << std::endl;

    return 0;
}
