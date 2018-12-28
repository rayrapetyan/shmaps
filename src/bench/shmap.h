#ifndef BENCH_SHMEM_SHMAP_H
#define BENCH_SHMEM_SHMAP_H

#include <string>

#include "../../include/shmaps/shmaps.hh"

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

class FooStatsExtShared {
public:
    FooStatsExtShared() : s1(*shmaps::seg_alloc), s2(*shmaps::seg_alloc) {}
    FooStatsExtShared(const int i1, const char *c1, const char *c2) :
            i1(i1), s1(c1, *shmaps::seg_alloc), s2(c2, *shmaps::seg_alloc) {}
    ~FooStatsExtShared() {}

    int i1;
    shmaps::String s1;
    shmaps::String s2;
};

#endif //BENCH_SHMEM_SHMAP_H
