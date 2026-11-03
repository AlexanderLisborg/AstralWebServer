#ifndef __astral_error__
#define __astral_error__

#include <stdlib.h>

// Helper macro, define enum entry
#define _GEN_ENUM(ENUM) ENUM,
// Helper macro, make string entry of macro name
#define _GEN_STRING(ENUM) #ENUM,

// Define new errors by modifying this generator
// macro.
#define _FOREACH_ERROR(GEN) \
    GEN(E_URI_TOO_LONG) \
    GEN(E_URI_EMPTY_STRING) \
    GEN(E_URI_IS_STATIC) \
    GEN(E_URI_ESCAPES_ROOT) \
    GEN(E_URI_RESOURCE_DNE) \
    GEN(E_URI_RESOURCE_READ_ACCESS_DENIED) \

// Basically assert. Unlike assert, this macro does not
// get excluded when DEBUG is set. Use to define scenarios 
// where the program should crash. Unexpected behavior 
// should crash the server. This may be extended to also log.
#define enforce(expression,description)  \
    ({if(expression);else{fprintf(stderr,"%s:%u: fatal error '%s' exiting...\n", __FILE__, __LINE__, (description)); exit(1);}})

// All errors should be handled without crashing. (non-fatal)
// For fatal errors, use enforce.
typedef enum _astral_error {
    _FOREACH_ERROR(_GEN_ENUM) \
    NUMBER_OF_ASTRAL_ERRORS
} astral_error_t;

// C-string representations of macro names.
const char* error_str[] = {
    _FOREACH_ERROR(_GEN_STRING)
};

#endif // Guard