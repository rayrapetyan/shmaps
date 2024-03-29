#include "./libcuckoo.h"

#include <benchmark/benchmark.h>

#include <libcuckoo/cuckoohash_map.hh>

#include "./conf.h"

const std::string long_str = std::string(100, 'a');

class LibCuckooFixture : public ::benchmark::Fixture {
public:
    typedef libcuckoo::cuckoohash_map<int, int> LibCuckooIntInt;
    typedef libcuckoo::cuckoohash_map<int, FooStats> LibCuckooIntFooStats;
    typedef libcuckoo::cuckoohash_map<std::string, int> LibCuckooStringInt;
    typedef libcuckoo::cuckoohash_map<std::string, FooStatsExtPlain> LibCuckooStringFooStatsExt;

    LibCuckooFixture() {
        // called once per benchmark before running any benchmarks
        // std::cout << "LibCuckooFixture ctor" << std::endl;
    }

    void SetUp(const ::benchmark::State& state) {
        // std::cout << "LibCuckooFixture SetUp" << std::endl;
        // called once per benchmark before benchmark starts executing
        libcuckoo_int_int = new LibCuckooIntInt;
        libcuckoo_int_foostats = new LibCuckooIntFooStats;
        libcuckoo_string_foostats_ext = new LibCuckooStringFooStatsExt;
        libcuckoo_string_int = new LibCuckooStringInt;
    }

    void TearDown(const ::benchmark::State& state) {
        // called once per benchmark after benchmark finished executing
        // std::cout << "LibCuckooFixture TearDown" << std::endl;
        libcuckoo_int_int->clear();
        delete libcuckoo_int_int;
        libcuckoo_int_foostats->clear();
        delete libcuckoo_int_foostats;
        libcuckoo_string_foostats_ext->clear();
        delete libcuckoo_string_foostats_ext;
        libcuckoo_string_int->clear();
        delete libcuckoo_string_int;
    }

    ~LibCuckooFixture() {
        return;
    }

    LibCuckooIntInt *libcuckoo_int_int;
    LibCuckooIntFooStats *libcuckoo_int_foostats;
    LibCuckooStringInt *libcuckoo_string_int;
    LibCuckooStringFooStatsExt *libcuckoo_string_foostats_ext;
};

BENCHMARK_F(LibCuckooFixture, BM_LibCuckoo_Set_IntInt)(benchmark::State &state) {
    bool res;
    for (auto _ : state) {
        libcuckoo_int_int->clear();
        for (int i = 0; i < el_num; ++i) {
            res = libcuckoo_int_int->insert(i, i);
            assert(res);
        }
    }
}

BENCHMARK_F(LibCuckooFixture, BM_LibCuckoo_SetGet_IntInt)(benchmark::State &state) {
    bool res;
    int val;
    for (auto _ : state) {
        libcuckoo_int_int->clear();
        for (int i = 0; i < el_num; ++i) {
            res = libcuckoo_int_int->insert(i, i);
            assert(res);
            res = libcuckoo_int_int->find(i, val);
            assert(res && val == i);
        }
    }
}

BENCHMARK_F(LibCuckooFixture, BM_LibCuckoo_Set_IntFooStats)(benchmark::State &state) {
    bool res;
    for (auto _ : state) {
        libcuckoo_int_foostats->clear();
        for (int i = 0; i < el_num; ++i) {
            res = libcuckoo_int_foostats->insert(i, FooStats(i, 2, 3.0));
            assert(res);
        }
    }
}

BENCHMARK_F(LibCuckooFixture, BM_LibCuckoo_SetGet_IntFooStats)(benchmark::State &state) {
    bool res;
    FooStats fs;
    for (auto _ : state) {
        libcuckoo_int_foostats->clear();
        for (int i = 0; i < el_num; ++i) {
            res = libcuckoo_int_foostats->insert(i, FooStats(i, 2, 3.0));
            assert(res);
            res = libcuckoo_int_foostats->find(i, fs);
            assert(res && fs.k == i);
        }
    }
}

BENCHMARK_F(LibCuckooFixture, BM_LibCuckoo_Set_StringInt)(benchmark::State &state) {
    bool res;
    for (auto _ : state) {
        libcuckoo_string_int->clear();
        for (int i = 0; i < el_num; ++i) {
            std::string s = std::to_string(i).append(long_str);
            res = libcuckoo_string_int->insert(s, i);
            assert(res);
        }
    }

}

BENCHMARK_F(LibCuckooFixture, BM_LibCuckoo_SetGet_StringInt)(benchmark::State &state) {
    bool res;
    int val;
    for (auto _ : state) {
        libcuckoo_string_int->clear();
        for (int i = 0; i < el_num; ++i) {
            std::string s = std::to_string(i).append(long_str);
            res = libcuckoo_string_int->insert(s, i);
            assert(res);
            res = libcuckoo_string_int->find(s.c_str(), val);
            assert(res && val == i);
        }
    }
}


BENCHMARK_F(LibCuckooFixture, BM_LibCuckoo_Set_StringFooStatsExt)(benchmark::State &state) {
    bool res;
    for (auto _ : state) {
        libcuckoo_string_foostats_ext->clear();
        for (int i = 0; i < el_num; ++i) {
            std::string s = std::to_string(i).append(long_str);
            res = libcuckoo_string_foostats_ext->insert(s, FooStatsExtPlain(i, s.c_str(), s.c_str()));
            assert(res);
        }
    }

}

BENCHMARK_F(LibCuckooFixture, BM_LibCuckoo_SetGet_StringFooStatsExt)(benchmark::State &state) {
    bool res;
    FooStatsExtPlain fse;
    for (auto _ : state) {
        libcuckoo_string_foostats_ext->clear();
        for (int i = 0; i < el_num; ++i) {
            std::string s = std::to_string(i).append(long_str);
            res = libcuckoo_string_foostats_ext->insert(s, FooStatsExtPlain(i, s.c_str(), s.c_str()));
            assert(res);
            res = libcuckoo_string_foostats_ext->find(s.c_str(), fse);
            assert(res && (fse.i1 == i) && (std::string(fse.s1.c_str()) == s) && std::string(fse.s2.c_str()) == s);
        }
    }
}
