#include "./conf.h"

#include <unistd.h>

size_t get_memory_size() {
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}
