#pragma once

#include <vector>
#include "isl-noexceptions.h"

isl::point
get_point_with_coordinates(isl::space point_space, isl::val val0,
    isl::val val1, isl::val val2, isl::val val3, isl::val val4, isl::val val5,
    isl::val val6, isl::val val7, isl::val val8, isl::val val9)
{
    isl::point point_out(point_space);
    isl::ctx ctx = point_space.get_ctx();
    std::cout << point_space.dim(isl::dim::set) << std::endl;
    std::cout << point_space.dim(isl::dim::param) << std::endl;
    size_t index = 0;
    std::vector<isl::val> vals {val0, val1, val2, val3, val4, val5, val6, val7,
        val8, val9};
    for (size_t i = 0; i < point_space.dim(isl::dim::set); ++i)
    {
        point_out = point_out.set_coordinate_val(isl::dim::set, i, vals.at(index % 10));
        index++;
    }
    for (size_t i = 0; i < point_space.dim(isl::dim::param); ++i)
    {
        point_out = point_out.set_coordinate_val(isl::dim::param, i, vals.at(index % 10));
        index++;
    }
    assert(index <= 10);
    return point_out;
}
