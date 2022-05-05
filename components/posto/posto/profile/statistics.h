/*
 *  Copyright (C) 2021-2022 Alibaba Group Holding Limited
 */

#ifndef POSTO_PROFILE_STATISTICS_H_
#define POSTO_PROFILE_STATISTICS_H_

#include <stdio.h>

#define _PRINT_TAB1() printf("  ")
#define _PRINT_TAB2() printf("    ")
#define _PRINT_TAB3() printf("      ")
#define _PRINT_TAB4() printf("        ")

#ifdef TAB_PRINT
#undef TAB_PRINT
#endif
#define TAB_PRINT(level, fmt, ...)              \
  do {                                          \
    switch (level) {                            \
    case 0: break;                              \
    case 1: _PRINT_TAB1(); break;               \
    case 2: _PRINT_TAB2(); break;               \
    case 3: _PRINT_TAB3(); break;               \
    case 4: _PRINT_TAB4(); break;               \
    default:                                    \
      for (int i = 0; i < level; ++i) {         \
        _PRINT_TAB1();                          \
      }                                         \
      break;                                    \
    }                                           \
    printf(fmt, ##__VA_ARGS__);                 \
  } while(0)

namespace posto {
namespace profile {

class Statistics {
public:
  virtual ~Statistics();

  virtual void Print(int level) = 0;
};

}  // namespace profile
}  // namespace posto

#endif  // POSTO_PROFILE_STATISTICS_H_
