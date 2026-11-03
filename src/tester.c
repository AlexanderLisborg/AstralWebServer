#include "astral_error.h"


#include <stdio.h>

int main(){
    printf("Hello world.\n");
    printf("%s\n",error_str[E_URI_TOO_LONG]);
    printf("haa%s\n","lol");
    enforce(2==2,"something");
    printf("%u\n",NUMBER_OF_ASTRAL_ERRORS);
    enforce(0,"Enforce is working correctly, exiting...");
    return 0;
}