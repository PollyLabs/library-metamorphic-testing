#pragma once

#include "isl-noexceptions.h"

static size_t point_count = 0;

isl::point
get_point_with_coordinates(isl::space point_space)
{
    isl::point point_out(point_space);
    isl::ctx ctx = point_space.get_ctx();
    for (size_t i = 0; i < point_space.dim(isl::dim::set); ++i)
    {
        isl::val coord_val(ctx, i + point_count);
        point_out = point_out.set_coordinate_val(isl::dim::set, i, coord_val);
    }
    for (size_t i = 0; i < point_space.dim(isl::dim::param); ++i)
    {
        isl::val coord_val(ctx, i + point_count);
        point_out = point_out.set_coordinate_val(isl::dim::param, i, coord_val);
    }
    ++point_count;
    return point_out;
}
