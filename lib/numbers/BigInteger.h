#ifndef NUMBERS_CONFIG_H
#define NUMBERS_CONFIG_H

#define USE_GMP

#ifdef USE_GMP
#include <numbers/BigIntegerGMP.h>
#else
#include <numbers/BigIntegerCustom.h>
#endif

#endif
