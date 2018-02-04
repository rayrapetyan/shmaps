#include "./shmap.h"

#include <benchmark/benchmark.h>

#include "./conf.h"

namespace bip = boost::interprocess;

class ShMapFixture : public ::benchmark::Fixture {
public:
    ShMapFixture() {
        //std::cout << "ShMapFixture ctor" << std::endl;
        const long est_shmem_size = get_memory_size() / 10;
        if (shmaps::init(est_shmem_size) != est_shmem_size) {
            shmaps::remove();
            if (shmaps::init(est_shmem_size) != est_shmem_size) {
                throw 1;
            }
        }
        shmap_int_int = new shmaps::Map<int, int>("ShMapIntInt");
        shmap_int_foostats = new shmaps::Map<int, FooStats>("ShMapIntFooStats");
        shmap_string_foostats_ext = new shmaps::Map<shmaps::String, FooStatsExtShared>("ShMapStringFooStatsExt");
        shmap_string_int = new shmaps::Map<shmaps::String, int>("ShMapStringInt");

        shmap_string_set_int = new shmaps::MapSet<shmaps::String, int>("ShMapStringSetInt");
        shmap_string_set_string = new shmaps::MapSet<shmaps::String, shmaps::String>("ShMapStringSetString");
    }

    void SetUp(const ::benchmark::State &state) {
        // std::cout << "ShMapFixture SetUp" << std::endl;
    }

    void TearDown(const ::benchmark::State &state) {
        //std::cout << "ShMapFixture TearDown" << std::endl;
    }

    ~ShMapFixture() {
        //std::cout << "ShMapFixture dtor" << std::endl;
        return;

        shmap_int_foostats->destroy();
        shmap_string_foostats_ext->destroy();
        shmap_string_int->destroy();
        shmap_string_set_int->destroy();
        shmap_string_set_string->destroy();

        delete shmap_int_foostats;
        delete shmap_string_foostats_ext;
        delete shmap_string_int;
        delete shmap_string_set_int;
        delete shmap_string_set_string;

        shmaps::remove();
    }

    shmaps::Map<int, int> *shmap_int_int;
    shmaps::Map<int, FooStats> *shmap_int_foostats;
    shmaps::Map<shmaps::String, FooStatsExtShared> *shmap_string_foostats_ext;
    shmaps::Map<shmaps::String, int> *shmap_string_int;

    shmaps::MapSet<shmaps::String, int> *shmap_string_set_int;
    shmaps::MapSet<shmaps::String, shmaps::String> *shmap_string_set_string;
};

BENCHMARK_F(ShMapFixture, BM_ShMap_Set_IntInt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        shmap_int_int->clear();
        for (int i = 0; i < el_num; ++i) {
            res = shmap_int_int->set(i, i, false, std::chrono::seconds(el_expires));
            assert(res);
        }
    }
}

BENCHMARK_F(ShMapFixture, BM_ShMap_SetGet_IntInt)(benchmark::State &st) {
    bool res;
    int val;
    while (st.KeepRunning()) {
        shmap_int_int->clear();
        for (int i = 0; i < el_num; ++i) {
            res = shmap_int_int->set(i, i, false, std::chrono::seconds(el_expires));
            assert(res);
            res = shmap_int_int->get(i, &val);
            assert(res && val == i);
        }
    }
}


BENCHMARK_F(ShMapFixture, BM_ShMap_Set_IntFooStats)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        shmap_int_foostats->clear();
        for (int i = 0; i < el_num; ++i) {
            res = shmap_int_foostats->set(i, FooStats(i, 2, 3.0), false, std::chrono::seconds(el_expires));
            assert(res);
        }
    }
}

BENCHMARK_F(ShMapFixture, BM_ShMap_SetGet_IntFooStats)(benchmark::State &st) {
    bool res;
    FooStats fs;
    while (st.KeepRunning()) {
        shmap_int_foostats->clear();
        for (int i = 0; i < el_num; ++i) {
            res = shmap_int_foostats->set(i, FooStats(i, 2, 3.0), false, std::chrono::seconds(el_expires));
            assert(res);
            res = shmap_int_foostats->get(i, &fs);
            assert(res && fs.k == i);
        }
    }
}

BENCHMARK_F(ShMapFixture, BM_ShMap_Set_StringInt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        shmap_string_int->clear();
        for (int i = 0; i < el_num; ++i) {
            shmaps::String s(std::to_string(i).append(
                    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").c_str(),
                            *shmaps::seg_alloc);
            res = shmap_string_int->set(s,
                                        i,
                                        false,
                                        std::chrono::seconds(el_expires));
            assert(res);
        }
    }

}

BENCHMARK_F(ShMapFixture, BM_ShMap_SetGet_StringInt)(benchmark::State &st) {
    bool res;
    int val;
    while (st.KeepRunning()) {
        shmap_string_int->clear();
        for (int i = 0; i < el_num; ++i) {
            shmaps::String s(std::to_string(i).append(
                    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").c_str(),
                            *shmaps::seg_alloc);
            res = shmap_string_int->set(s,
                                        i,
                                        false,
                                        std::chrono::seconds(el_expires));
            assert(res);
            res = shmap_string_int->get(s, &val);
            assert(res && val == i);
        }
    }
}

BENCHMARK_F(ShMapFixture, BM_ShMap_Set_StringFooStatsExt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        shmap_string_foostats_ext->clear();
        for (int i = 0; i < el_num; ++i) {
            shmaps::String s(std::to_string(i).append(
                    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").c_str(),
                            *shmaps::seg_alloc);
            res = shmap_string_foostats_ext->set(s,
                                                 FooStatsExtShared(i, s.c_str(), s.c_str()),
                                                 false,
                                                 std::chrono::seconds(el_expires));
            assert(res);
        }
    }

}

BENCHMARK_F(ShMapFixture, BM_ShMap_SetGet_StringFooStatsExt)(benchmark::State &st) {
    bool res;
    FooStatsExtShared fse;
    while (st.KeepRunning()) {
        shmap_string_foostats_ext->clear();
        for (int i = 0; i < el_num; ++i) {
            shmaps::String s(std::to_string(i).append("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").c_str(),
                            *shmaps::seg_alloc);
            res = shmap_string_foostats_ext->set(s,
                                                 FooStatsExtShared(i, s.c_str(), s.c_str()),
                                                 false,
                                                 std::chrono::seconds(el_expires));
            assert(res);
            res = shmap_string_foostats_ext->get(s, &fse);
            assert(res && (fse.i1 == i) && (fse.s1 == s) && (fse.s2 == s));
        }
    }
}

/*
BENCHMARK_F(ShMapFixture, BM_ShMap_Add_String_SetString)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            shmaps::LockType lock(shmap_string_set_string->mutex());
            res = shmap_string_set_string->add(std::to_string(i), std::to_string(i), el_expires);
            assert(res);
        }
    }
}


BENCHMARK_F(ShMapFixture, BM_ShMap_Get_String_SetString)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            std::set<std::string> res_check = {std::to_string(i)};
            std::set<std::string> ss;
            shmaps::LockType lock(shmap_string_set_string->mutex());
            res = shmap_string_set_string->members(std::to_string(i), &ss);
            assert(res && ss == res_check);
        }
    }
}


BENCHMARK_F(ShMapFixture, BM_ShMap_Add_String_SetInt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            shmaps::LockType lock(shmap_string_set_int->mutex());
            res = shmap_string_set_int->add(std::to_string(i), i);
            assert(res);
        }
    }
}

BENCHMARK_F(ShMapFixture, BM_ShMap_Get_String_SetInt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            std::set<int> res_check = {i};
            std::set<int> si;
            shmaps::LockType lock(shmap_string_set_int->mutex());
            res = shmap_string_set_int->members(std::to_string(i), &si);
            assert(res && si == res_check);
        }
    }
}
*/
