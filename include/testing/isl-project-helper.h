#ifndef ISL_PROJECT_HELPER_H
#define ISL_PROJECT_HELPER_H

#include "isl-noexceptions.h"

isl::set
make_project(isl::set set_in)
{
    if (set_in.dim(isl::dim::set) != 0)
    {
        return set_in.project_out(isl::dim::set, 0, 1);
    }
    return set_in;
}

#endif
