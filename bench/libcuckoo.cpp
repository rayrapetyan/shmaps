#include "./libcuckoo.h"

#include <benchmark/benchmark.h>

#include <libcuckoo/cuckoohash_map.hh>
#include <libcuckoo/city_hasher.hh>

#include "./conf.h"

class LibCuckooFixture : public ::benchmark::Fixture {
public:
    typedef cuckoohash_map<int, int> LibCuckooIntInt;
    typedef cuckoohash_map<int, FooStats> LibCuckooIntFooStats;
    typedef cuckoohash_map<std::string, int> LibCuckooStringInt;
    typedef cuckoohash_map<std::string, FooStatsExt> LibCuckooStringFooStatsExt;

    LibCuckooFixture() {
        libcuckoo_int_int = new LibCuckooIntInt;
        libcuckoo_int_foostats = new LibCuckooIntFooStats;
        libcuckoo_string_foostats_ext = new LibCuckooStringFooStatsExt;
        libcuckoo_string_int = new LibCuckooStringInt;
    }

    void SetUp() {
        //std::cout << "ShMapFixture SetUp" << std::endl;
    }

    void TearDown() {
        //std::cout << "ShMapFixture TearDown" << std::endl;
    }

    ~LibCuckooFixture() {
        return;
    }

    LibCuckooIntInt *libcuckoo_int_int;
    LibCuckooIntFooStats *libcuckoo_int_foostats;
    LibCuckooStringInt *libcuckoo_string_int;
    LibCuckooStringFooStatsExt *libcuckoo_string_foostats_ext;
};

BENCHMARK_F(LibCuckooFixture, BM_LibCuckoo_Set_IntInt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            res = libcuckoo_int_int->insert(i, i);
            assert(res);
        }
    }
}

BENCHMARK_F(LibCuckooFixture, BM_LibCuckoo_Get_IntInt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            int val;
            res = libcuckoo_int_int->find(i, val);
            assert(res && val.k == i);
        }
    }
}

BENCHMARK_F(LibCuckooFixture, BM_LibCuckoo_Set_IntFooStats)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            res = libcuckoo_int_foostats->insert(i, FooStats(i, 2, 3.0));
            assert(res);
        }
    }
}

BENCHMARK_F(LibCuckooFixture, BM_LibCuckoo_Get_IntFooStats)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            FooStats fs;
            res = libcuckoo_int_foostats->find(i, fs);
            assert(res && fs.k == i);
        }
    }
}

BENCHMARK_F(LibCuckooFixture, BM_LibCuckoo_Set_StringInt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            std::string s = std::to_string(i).append("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
            res = libcuckoo_string_int->insert(s,
                                        i);
            assert(res);
        }
    }

}

BENCHMARK_F(LibCuckooFixture, BM_LibCuckoo_Get_StringInt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            int val;
            std::string s = std::to_string(i).append("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
            res = libcuckoo_string_int->find(s.c_str(), val);
            assert(res && val == i);
        }
    }
}


BENCHMARK_F(LibCuckooFixture, BM_LibCuckoo_Set_StringFooStatsExt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            std::string s = std::to_string(i).append("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
            res = libcuckoo_string_foostats_ext->insert(s,
                                                        FooStatsExt(i, s.c_str(), s.c_str()));
            assert(res);
        }
    }

}

BENCHMARK_F(LibCuckooFixture, BM_LibCuckoo_Get_StringFooStatsExt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            FooStatsExt fse;
            std::string s = std::to_string(i).append("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
            res = libcuckoo_string_foostats_ext->find(s.c_str(), fse);
            assert(res && (fse.i1 == i) && (std::string(fse.s1.c_str()) == s) && std::string(fse.s2.c_str()) == s);
        }
    }
}