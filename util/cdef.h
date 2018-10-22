#ifndef _UTIL_CDEF_
#define _UTIL_CDEF_

#define SAFE_FREE(p)              if(NULL != (p)){free (p);(p) = NULL;}

#define CHECK_POINT_VOID(p)       if(NULL == (p)){printf("NULL == "#p);return;}
#define CHECK_POINT_BOOL(p)       if(NULL == (p)){printf("NULL == "#p);return false;}
#define CHECK_POINT_NULLPTR(p)    if(NULL == (p)){printf("NULL == "#p);return NULL;}
#define CHECK_POINT_INT(p,r)      if(NULL == (p)){printf("NULL == "#p);return (r);}

#define boolc uint8_t
#define true  1
#define false 0

#endif