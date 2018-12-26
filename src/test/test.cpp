#include "../../include/shmaps/shmaps.hh"

#include <random>

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
    FooStatsExt() : s1(*shmaps::seg_alloc), s2(*shmaps::seg_alloc) {}

    FooStatsExt(const int i1, const char *c1, const char *c2) :
            i1(i1), s1(c1, *shmaps::seg_alloc), s2(c2, *shmaps::seg_alloc) {}

    ~FooStatsExt() {}

    int i1;
    shmaps::String s1;
    shmaps::String s2;
};

int main(int argc, char *argv[]) {
    shmaps::reset();
    const uint64_t est_shmem_size = 1024 * 1024 * 2000; // make sure value here is x2 of what you get as "free" after tests passed
    shmaps::init(est_shmem_size);

    unsigned int wrk = 4;
    std::cout << "worker " << wrk << " created" << std::endl;

    while (--wrk) {
        pid_t pid = fork();
        if (pid == 0) {
            std::cout << "worker " << wrk << " created" << std::endl;
            break;
        }
    }

    const int el_expires = 2;
    bool res;
    int k = 100;
    int val;
    const std::string long_str= "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    FooStatsExt fse;
    shmaps::String sk(std::to_string(k).append(long_str).c_str(),
                     *shmaps::seg_alloc);

    shmaps::Map<shmaps::String, int> *shmap_string_int = new shmaps::Map<shmaps::String, int>("ShMap_String_Int");
    res = shmap_string_int->set(sk, k, false, std::chrono::seconds(el_expires));
    assert(res);
    res = shmap_string_int->get(sk, &val);
    assert(res && val == k);

    shmaps::Map<int, FooStats> *shmap_int_foostats = new shmaps::Map<int, FooStats>("ShMap_Int_FooStats");
    res = shmap_int_foostats->set(k, FooStats(k, 2, 3.0), false, std::chrono::seconds(el_expires));
    assert(res);
    FooStats fs;
    res = shmap_int_foostats->get(k, &fs);
    assert(res && fs.k == k);

    shmaps::Map <shmaps::String, FooStatsExt> *shmap_string_foostats_ext =
            new shmaps::Map<shmaps::String, FooStatsExt>("ShMap_String_FooStatsExt");
    res = shmap_string_foostats_ext->set(sk,
                                         FooStatsExt(k, sk.c_str(), sk.c_str()),
                                         false,
                                         std::chrono::seconds(el_expires));
    assert(res);
    res = shmap_string_foostats_ext->get(sk, &fse);
    assert(res && (fse.i1 == k) && (fse.s1 == sk) && (fse.s2 == sk));

    shmaps::MapSet<shmaps::String, int> *shmap_string_set_int =
            new shmaps::MapSet<shmaps::String, int>("ShMap_String_SetInt");
    res = shmap_string_set_int->add(sk, k);
    assert(res);
    res = shmap_string_set_int->is_member(sk, k);
    assert(res);
    std::set<int> res_check1 = {k};
    std::set<int> si;
    res = shmap_string_set_int->members(sk, &si);
    assert(res && si == res_check1);

    shmaps::MapSet <shmaps::String, shmaps::String> *shmap_string_set_string =
            new shmaps::MapSet<shmaps::String, shmaps::String>("ShMap_String_SetString");
    res = shmap_string_set_string->add(sk, shmaps::String(sk.c_str(), *shmaps::seg_alloc),
                                       std::chrono::seconds(el_expires));
    assert(res);
    std::set<shmaps::String> res_check2 = {shmaps::String(sk.c_str(), *shmaps::seg_alloc)};
    std::set<shmaps::String> ss;
    res = shmap_string_set_string->members(sk, &ss);
    assert(res && ss == res_check2);

    shmaps::Map <uint64_t, uint64_t> *shmap_stress = new shmaps::Map<uint64_t, uint64_t>("ShMap_Stress");
    shmaps::Map <uint64_t, FooStatsExt> *shmapset_stress = new shmaps::Map<uint64_t, FooStatsExt>("ShMapSet_Stress");

    std::random_device rnd_dev;
    std::mt19937_64 rnd_gen(rnd_dev());
    std::uniform_int_distribution<uint64_t> dist_keys(0, 2000000);

    uint64_t rnd_k;
    uint64_t val_stress;

    for (uint64_t kstress = 0; kstress < 500000; ++kstress) {
        rnd_k = dist_keys(rnd_gen);
        res = shmap_stress->set(rnd_k, kstress, true, std::chrono::seconds(el_expires));
        res = shmap_stress->get(rnd_k, &val_stress);
        std::string s(std::to_string(kstress).append(long_str));
        res = shmapset_stress->set(rnd_k,
                                   FooStatsExt(kstress, s.c_str(), s.c_str()),
                                   false,
                                   std::chrono::seconds(el_expires));
        res = shmapset_stress->get(rnd_k, &fse);
        if (kstress % 10000 == 0)
            std::cout << kstress << std::endl;
    }
    shmap_stress->info();

    std::cout << "process finished" << std::endl;

    return 0;
}
