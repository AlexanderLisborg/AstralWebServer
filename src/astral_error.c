#ifndef __astral_errorc__
#define __astral_errorc__

#include "astral_error.h"

// C-string representations of macro names.
const char* astralErrorStr[] = {
    _FOREACH_ERROR(_GEN_STRING)
};
#endif