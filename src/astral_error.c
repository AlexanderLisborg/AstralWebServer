#include "astral_error.h"

// C-string representations of macro names.
const char* astral_error_str[] = {
    _FOREACH_ERROR(_GEN_STRING)
};