#pragma once

#include "lux/debug.h"

// macros
// ---------------------------------------------------------------- 

#define GUARD(cond, err, ...)                                   \
    if (cond)                                                   \
    {                                                           \
        lx_error err;                                           \
        return __VA_ARGS__;                                     \
    }
