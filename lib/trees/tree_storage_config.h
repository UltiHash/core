#ifndef NUMBERS_CONFIG_H
#define NUMBERS_CONFIG_H

#define USE_TINY_TREE

#ifdef USE_TINY_TREE
#define LATENCY_TEST_SIZE (std::size_t) std::pow(2, 20)
#define PERFORMANCE_TEST_SIZE (std::size_t) (std::pow(2,21))
#define STORE_MAX (unsigned int) (std::numeric_limits<unsigned char>::max() / 2)
#else
#define LATENCY_TEST_SIZE (std::size_t) std::pow(2, 22)
#define PERFORMANCE_TEST_SIZE std::pow(1024, 4) * 4
#define STORE_MAX (unsigned int) std::numeric_limits<unsigned int>::max()
#endif
#define STORE_HARD_LIMIT (unsigned long) (STORE_MAX * 2)
#define STORE_MAX_LATENCY 32

#endif
