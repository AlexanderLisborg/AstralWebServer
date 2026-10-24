#ifndef __ass__
#define __ass__
// Assertion, can potentially be modified to handle concurrent logging instead of printing to stderr.
#define ass(exp,description)  \
    ({if(exp)else{fprintf (stderr,"%s:%u: failed assertion `%u'\n", __FILE__, __LINE__, description);}})

#endif // Guard