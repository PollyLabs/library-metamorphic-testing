#ifndef ISL_META_POINT_STATS_HPP
#define ISL_META_POINT_STATS_HPP

static size_t c_cnt;
//static unsigned int eq_cnt;

isl::stat
getBSConstraints(isl::basic_set bs)
{
    c_cnt += bs.n_constraint();
    return isl::stat::ok;
}

isl::stat
writeConstraintCoeff(isl::constraint c)
{
    fprintf(stderr, "[");
    for (size_t i = 0; i < c.get_space().dim(isl::dim::set); ++i)
    {
        fputs(
            std::to_string(c.get_coefficient_val(isl::dim::set, i).get_num_si()).c_str(),
	    stderr);
    }
    for (size_t i = 0; i < c.get_space().dim(isl::dim::param); ++i)
    {
        fputs(
            std::to_string(c.get_coefficient_val(isl::dim::param, i).get_num_si()).c_str(),
	    stderr);
    }
    fprintf(stderr, "]");
    return isl::stat::ok;
}

isl::stat
writeBSConstraintCoeff(isl::basic_set bs)
{
    bs.foreach_constraint(writeConstraintCoeff);
    return isl::stat::ok;
}


void
printStats(isl::set s)
{
    c_cnt = 0;

    fprintf(stderr, "DIM SET = %d\n", s.dim(isl::dim::set));
    fprintf(stderr, "DIM PARAM = %d\n", s.dim(isl::dim::param));
    fprintf(stderr, "SET EMPTY = %s\n", s.is_empty() ? "true" : "false");
    fprintf(stderr, "N BASIC SET = %d\n", s.n_basic_set());
    s.foreach_basic_set(getBSConstraints);
    fprintf(stderr, "N CONSTRAINTS = %zu\n", c_cnt);

    //fprintf(stderr, "CONSTRAINT COEFF:");
    //s.foreach_basic_set(writeBSConstraintCoeff);
}

#endif
