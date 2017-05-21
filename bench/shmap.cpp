#include "./shmap.h"

#include <benchmark/benchmark.h>

#include "./conf.h"

namespace shmem = shared_memory;
namespace bip = boost::interprocess;

class ShMapFixture : public ::benchmark::Fixture {
public:
    ShMapFixture() {
        //std::cout << "ShMapFixture ctor" << std::endl;

        shmem::init(1 * 1024);
        shmem::remove();

        const long est_shmem_size = get_memory_size() / 12;
        if (shmem::init(est_shmem_size) != est_shmem_size) {
            throw 1;
        }

        shmap_int_int = new shmem::Map<int, int>("ShMapIntInt");
        shmap_int_foostats = new shmem::Map<int, FooStats>("ShMapIntFooStats");
        shmap_string_foostats_ext = new shmem::Map<std::string, FooStatsExt>("ShMapStringFooStatsExt");
        shmap_string_int = new shmem::Map<std::string, int>("ShMapStringInt");

        shmap_string_set_int = new shmem::MapSet<std::string, int>("ShMapStringSetInt");
        shmap_string_set_string = new shmem::MapSet<std::string, shmem::String>("ShMapStringSetString");
    }

    void SetUp() {
        //std::cout << "ShMapFixture SetUp" << std::endl;
    }

    void TearDown() {
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

        shmem::remove();
    }

    shmem::Map<int, int> *shmap_int_int;
    shmem::Map<int, FooStats> *shmap_int_foostats;
    shmem::Map<std::string, FooStatsExt> *shmap_string_foostats_ext;
    shmem::Map<std::string, int> *shmap_string_int;

    shmem::MapSet<std::string, int> *shmap_string_set_int;
    shmem::MapSet<std::string, shmem::String> *shmap_string_set_string;
};

BENCHMARK_F(ShMapFixture, BM_ShMap_Set_IntInt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            res = shmap_int_int->set(i, i, false, std::chrono::seconds(el_expires));
            assert(res);
        }
    }
}

BENCHMARK_F(ShMapFixture, BM_ShMap_Get_IntInt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            int val;
            res = shmap_int_int->get(i, &val);
            assert(res && val.k == i);
        }
    }
}

BENCHMARK_F(ShMapFixture, BM_ShMap_Set_IntFooStats)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            res = shmap_int_foostats->set(i, FooStats(i, 2, 3.0), false, std::chrono::seconds(el_expires));
            assert(res);
        }
    }
}

BENCHMARK_F(ShMapFixture, BM_ShMap_Get_IntFooStats)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            FooStats fs;
            res = shmap_int_foostats->get(i, &fs);
            assert(res && fs.k == i);
        }
    }
}

BENCHMARK_F(ShMapFixture, BM_ShMap_Set_StringInt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            std::string s = std::to_string(i).append("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
            res = shmap_string_int->set(s,
                                        i,
                                        false,
                                        std::chrono::seconds(el_expires));
            assert(res);
        }
    }

}

BENCHMARK_F(ShMapFixture, BM_ShMap_Get_StringInt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            int val;
            std::string s = std::to_string(i).append("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
            res = shmap_string_int->get(s.c_str(), &val);
            assert(res && val == i);
        }
    }
}

BENCHMARK_F(ShMapFixture, BM_ShMap_Set_StringFooStatsExt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            std::string s = std::to_string(i).append("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
            res = shmap_string_foostats_ext->set(s,
                                                 FooStatsExt(i, s.c_str(), s.c_str()),
                                                 false,
                                                 std::chrono::seconds(el_expires));
            assert(res);
        }
    }

}

BENCHMARK_F(ShMapFixture, BM_ShMap_Get_StringFooStatsExt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            FooStatsExt fse;
            std::string s = std::to_string(i).append("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
            res = shmap_string_foostats_ext->get(s.c_str(), &fse);
            assert(res && (fse.i1 == i) && (std::string(fse.s1.c_str()) == s) && std::string(fse.s2.c_str()) == s);
        }
    }
}

/*
BENCHMARK_F(ShMapFixture, BM_ShMap_Add_String_SetString)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            shmem::LockType lock(shmap_string_set_string->mutex());
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
            shmem::LockType lock(shmap_string_set_string->mutex());
            res = shmap_string_set_string->members(std::to_string(i), &ss);
            assert(res && ss == res_check);
        }
    }
}


BENCHMARK_F(ShMapFixture, BM_ShMap_Add_String_SetInt)(benchmark::State &st) {
    bool res;
    while (st.KeepRunning()) {
        for (int i = 0; i < el_num; ++i) {
            shmem::LockType lock(shmap_string_set_int->mutex());
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
            shmem::LockType lock(shmap_string_set_int->mutex());
            res = shmap_string_set_int->members(std::to_string(i), &si);
            assert(res && si == res_check);
        }
    }
}
*/
