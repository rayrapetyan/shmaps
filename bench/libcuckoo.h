#ifndef BENCH_SHMEM_LIBCUCKOO_H
#define BENCH_SHMEM_LIBCUCKOO_H

#include <string>

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

class FooStatsExtPlain {
public:
    FooStatsExtPlain() {}
    FooStatsExtPlain(const int i1, const char *c1, const char *c2) :
            i1(i1), s1(c1), s2(c2) {}
    ~FooStatsExtPlain() {}

    int i1;
    std::string s1;
    std::string s2;
};

#endif //BENCH_SHMEM_LIBCUCKOO_H
