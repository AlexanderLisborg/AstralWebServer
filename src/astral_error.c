#include "astral_error.h"

// C-string representations of macro names.
const char* astralErrorStr[] = {
    _FOREACH_ERROR(_GEN_STRING)
};