#ifndef SHMAPS_BENCH_CONF_H
#define SHMAPS_BENCH_CONF_H

#include <sys/types.h>

const int el_expires = 2;
const int el_num = 1 * 1024 * 1024;

size_t get_memory_size();

#endif //SHMAPS_BENCH_CONF_H
