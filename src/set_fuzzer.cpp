#include "set_fuzzer.hpp"

namespace set_fuzzer {

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
    isl::pw_aff (isl::pw_aff::*op)(void) const;
} unary_expr_ops[] = {
    { &isl::pw_aff::ceil },
    { &isl::pw_aff::floor },
};

struct {
    isl::pw_aff (isl::pw_aff::*op)(isl::val val) const;
} val_bin_expr_ops[] = {
    { &isl::pw_aff::mod },
    { &isl::pw_aff::scale },
};

struct {
    isl::pw_aff (isl::pw_aff::*op)(isl::pw_aff other) const;
} bin_expr_ops[] = {
    { &isl::pw_aff::add },
    { &isl::pw_aff::sub },
    //{ &isl::pw_aff::div },
    //{ &isl::pw_aff::mul },
    { &isl::pw_aff::max },
    { &isl::pw_aff::min },
};

template<typename T>
const unsigned int
get_random_func_id(T &func_struct)
{
    return (std::rand() % sizeof(func_struct) / sizeof(func_struct[0]));
}

isl::val
generate_expr(const Parameters &pars)
{
    isl::val new_val(*pars.ctx, (long) std::rand() % 10 + 1);
    return new_val;
}

isl::pw_aff
generate_expr(isl::pw_aff &input, const Parameters &pars)
{
    //if (std::rand() % 5) {
        //isl::pw_aff (isl::pw_aff::*op)(void) const =
            //unary_expr_ops[get_random_func_id(unary_expr_ops);
        //input = input.mul(isl::pw_aff(isl::set::universe(*pars.space),
                                      //isl::val(*pars.ctx, std::rand() % 42)));
    //}
    return input;
}

isl::pw_aff
generate_expr(std::vector<isl::pw_aff> inputs, unsigned int max_operand_count,
    const Parameters &pars)
{
    isl::pw_aff new_expr = generate_expr(inputs.back(), pars);
    while (max_operand_count-- > 0) {
        //std::cout << "OPCOUNT " << max_operand_count << std::endl;
        switch (std::rand() % 3) {
            case 0: {
                isl::pw_aff dim_add = generate_expr(inputs.at(std::rand() % inputs.size()), pars);
                isl::pw_aff (isl::pw_aff::*pw_op)(isl::pw_aff) const =
                    bin_expr_ops[get_random_func_id(bin_expr_ops)].op;
                new_expr = (new_expr.*pw_op)(dim_add);
                break;
            }
            case 1: {
                isl::val val_add = generate_expr(pars);
                isl::pw_aff (isl::pw_aff::*pw_op)(isl::val) const =
                    val_bin_expr_ops[get_random_func_id(val_bin_expr_ops)].op;
                new_expr = (new_expr.*pw_op)(val_add);
                break;
            }
            case 2: {
                isl::val val_expr = generate_expr(pars);
                isl::pw_aff val_bin_add(isl::set::universe(*pars.space), val_expr);
                isl::pw_aff (isl::pw_aff::*pw_op)(isl::pw_aff) const =
                    bin_expr_ops[get_random_func_id(bin_expr_ops)].op;
                new_expr = (new_expr.*pw_op)(val_bin_add);
                break;
            }
            // Does it do this implicitly?
            /*case 2: {
                std::cout << "BEFORE FLOOR " << new_expr.to_str() << std::endl;
                isl::pw_aff (isl::pw_aff::*u_op)(void) const =
                    unary_expr_ops[get_random_func_id(unary_expr_ops)].op;
                new_expr = (new_expr.*u_op)();
                std::cout << "AFTER FLOOR " << new_expr.to_str() << std::endl;
                break;
            }*/
        }
        //std::cout << "EXPR " << new_expr.to_str() << std::endl;
        //std::cout << "BACK" << inputs.back().to_str() << std::endl;
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
generate_one_set(const Parameters &pars)
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
    isl::pw_aff lhs = generate_expr(lhs_dims, 2, pars);
    isl::pw_aff rhs = generate_expr(rhs_dims, 6, pars);
    //std::cout << "LHS " << lhs.to_str() << std::endl;
    //std::cout << "RHS " << rhs.to_str() << std::endl;

    // Create set
    isl::set (isl::pw_aff::*op)(isl::pw_aff) const =
        gen_set_ops[std::rand() % (sizeof(gen_set_ops) /
                                   sizeof(gen_set_ops)[0])].op;
    isl::set generated_set = (lhs.*op)(rhs);
    //std::cout << "GSET " << generated_set.to_str() << std::endl;
    return generated_set;
}

isl::set
fuzz_set(isl::ctx ctx, const unsigned int max_dims,
    const unsigned int max_params,
    const unsigned int max_set_count)
{
    unsigned int dims = std::rand() % (max_dims + 1);
    if (!dims)
        dims++;
    unsigned int params = std::rand() % (max_params + 1);
    if (!params)
        params++;
    unsigned int set_count = std::rand() % (max_set_count + 1);
    if (!set_count)
        set_count++;
    //std::cout << "--Dims: " << dims << std::endl;
    //std::cout << "--Params: " << params << std::endl;
    //std::cout << "--Set_Count: " << set_count << std::endl;
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
        final_set = final_set.intersect(generate_one_set(pars));
    //std::cout << "FSET " << final_set.to_str() << std::endl;

    return final_set;
}

}
