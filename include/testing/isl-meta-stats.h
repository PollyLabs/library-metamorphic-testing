#ifndef ISL_META_STATS_HPP
#define ISL_META_STATS_HPP

static unsigned int c_cnt;

isl::stat
getBSConstraints(isl::basic_set bs)
{
    c_cnt += bs.n_constraint();
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
    fprintf(stderr, "N CONSTRAINTS = %d\n", c_cnt);
}

#endif
