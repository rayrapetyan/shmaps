#include "./shmaps.h"

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

int main(int argc, char *argv[]) {
    const uint64_t est_shmem_size = 1024 * 1024 * 800; // XMB

    shmem::init(1 * 1024);
    shmem::remove();

    if (shmem::init(est_shmem_size) != est_shmem_size) {
        return 1;
    }

    const int el_expires = 2;
    bool res;
    int k = 100;
    std::string sk = std::to_string(k);

    shmem::Map<std::string, int> *shmap_string_int = new shmem::Map<std::string, int>("ShMap_String_Int");
    res = shmap_string_int->set(sk, k, el_expires);
    assert(res);
    int val;
    res = shmap_string_int->get(sk, &val);
    assert(res && val == k);


    shmem::Map<int, FooStats> *shmap_int_foostats = new shmem::Map<int, FooStats>("ShMap_Int_FooStats");
    res = shmap_int_foostats->set(k, FooStats(k, 2, 3.0), false, std::chrono::seconds(el_expires));
    assert(res);
    FooStats fs;
    res = shmap_int_foostats->get(k, &fs);
    assert(res && fs.k == k);


    shmem::Map <std::string, FooStatsExt> *shmap_string_foostats_ext = new shmem::Map<std::string, FooStatsExt>(
            "ShMap_String_FooStatsExt");
    res = shmap_string_foostats_ext->set(sk,
                                         FooStatsExt(k, sk.c_str(), sk.c_str()),
                                         false,
                                         std::chrono::seconds(el_expires));
    assert(res);
    FooStatsExt fse;
    res = shmap_string_foostats_ext->get(sk, &fse);
    assert(res && (fse.i1 == k) && (std::string(fse.s1.c_str()) == sk) && std::string(fse.s2.c_str()) == sk);


    shmem::MapSet<std::string, int> *shmap_string_set_int = new shmem::MapSet<std::string, int>("ShMap_String_SetInt");
    res = shmap_string_set_int->add(sk, k);
    assert(res);
    res = shmap_string_set_int->is_member(sk, k);
    assert(res);
    std::set<int> res_check1 = {k};
    std::set<int> si;
    res = shmap_string_set_int->members(sk, &si);
    assert(res && si == res_check1);

    shmem::MapSet <std::string, shmem::String> *shmap_string_set_string =
            new shmem::MapSet<std::string, shmem::String>("ShMap_String_SetString");
    res = shmap_string_set_string->add(sk, shmem::String(sk.c_str(), *shmem::seg_alloc), std::chrono::seconds(el_expires));
    assert(res);
    std::set<shmem::String> res_check2 = {shmem::String(sk.c_str(), *shmem::seg_alloc)};
    std::set<shmem::String> ss;
    res = shmap_string_set_string->members(sk, &ss);
    assert(res && ss == res_check2);


    shmem::Map<uint64_t, uint64_t> *shmap_stress = new shmem::Map<uint64_t, uint64_t>("ShMap_Stress");

    unsigned int wrk = 1;
    std::cout << "worker " << wrk << " created" << std::endl;

    while (--wrk) {
        pid_t pid = fork();
        if (pid == 0) {
            std::cout << "worker " << wrk << " created" << std::endl;
            break;
        }
    }

    std::random_device rnd_dev;
    std::mt19937_64 rnd_gen(rnd_dev());
    std::uniform_int_distribution<uint64_t> dist_keys(0, 400000);

    for (uint64_t kstress=0; kstress < 10000000; ++kstress) {
        uint64_t rnd_k = dist_keys(rnd_gen);
        res = shmap_stress->set(rnd_k, kstress, true, std::chrono::seconds(el_expires));
        uint64_t val;
        res = shmap_stress->get(rnd_k, &val);
        if (kstress % 1000000 == 0)
            std::cout << kstress << std::endl;
    }
    shmap_stress->info();

    std::cout << "process finished" << std::endl;

    return 0;
}
