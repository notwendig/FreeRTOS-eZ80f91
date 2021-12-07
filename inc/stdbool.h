#ifndef STDBOOL_H
#define STDBOOL_h

typedef char _Bool;

#define bool _Bool
#ifndef true
  #define true 1
#endif

#ifndef false
  #define false 0
#endif

#endif
