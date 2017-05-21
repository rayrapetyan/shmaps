#ifndef BENCH_SHMEM_SHMAP_H
#define BENCH_SHMEM_SHMAP_H

#include <string>

#include <shmaps.h>
namespace shmem = shared_memory;

class FooStats {
public:
    FooStats() {

    }

    FooStats(int k, int b, float rev) : k(k), b(b), rev(rev) {

    }

    FooStats(const FooStats &fs) : k(fs.k), b(fs.b), rev(fs.rev) {

    }

    FooStats(FooStats &fs) : k(fs.k), b(fs.b), rev(fs.rev) {

    }

    ~FooStats() {

    };
    int k;
    int b;
    float rev;
};

class FooStatsExt {
public:
    FooStatsExt() : s1(*shmem::seg_alloc), s2(*shmem::seg_alloc) {}
    FooStatsExt(const int i1, const char *c1, const char *c2) :
            i1(i1), s1(c1, *shmem::seg_alloc), s2(c2, *shmem::seg_alloc) {}
    ~FooStatsExt() {}

    int i1;
    shmem::String s1;
    shmem::String s2;
};

#endif //BENCH_SHMEM_SHMAP_H
