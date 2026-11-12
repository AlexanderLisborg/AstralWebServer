#ifndef __span__
#define __span__

typedef struct char_span_small{
    char* head;
    unsigned char length; // 255 char max length
}char_span_small_t;

typedef struct char_span{
    char* head;
    unsigned int length;
}char_span_t;

#endif // Guard