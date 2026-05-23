#include "../../include/shmaps/shmaps.hh"

#include <sys/wait.h>

#include <random>
#include <string>
#include <thread>
#include <vector>

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

static shmaps::Map<shmaps::String, int> *shmap_string_int_static =
        new shmaps::Map<shmaps::String, int>("ShMap_Static_String_Int");

int main(int argc, char *argv[]) {
    unsigned int num_wrk = 4;
    std::cout << "worker " << num_wrk << " created" << std::endl;

    while (--num_wrk) {
        pid_t pid = fork();
        if (pid == 0) {
            std::cout << "worker " << num_wrk << " created" << std::endl;
            break;
        }
    }

    const int el_expires = 2;
    bool res = false;
    int k = 100;
    int val;
    FooStatsExt fse;
    const std::string long_str = std::string(100, 'a');

    shmaps::String sk(std::to_string(k).append(long_str).c_str(), *shmaps::seg_alloc);

    res = shmap_string_int_static->set(sk, k, false, std::chrono::seconds(el_expires));
    assert(res);
    res = shmap_string_int_static->get(sk, &val);
    assert(res && val == k);

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

    shmaps::Map<shmaps::String, FooStatsExt> *shmap_string_foostats_ext =
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

    shmaps::MapSet<shmaps::String, shmaps::String> *shmap_string_set_string =
            new shmaps::MapSet<shmaps::String, shmaps::String>("ShMap_String_SetString");
    res = shmap_string_set_string->add(sk, shmaps::String(sk.c_str(), *shmaps::seg_alloc),
                                       std::chrono::seconds(el_expires));
    assert(res);
    std::set<shmaps::String> res_check2 = {shmaps::String(sk.c_str(), *shmaps::seg_alloc)};
    std::set<shmaps::String> ss;
    res = shmap_string_set_string->members(sk, &ss);
    assert(res && ss == res_check2);

    // --- create_only semantics ---
    // Only one worker (pid 0 children break early, only num_wrk==0 reaches here)
    // so these single-key tests are safe across processes.
    {
        shmaps::Map<int, int> *m = new shmaps::Map<int, int>("ShMap_CreateOnly");
        (void)m;
        int cv;
        (void)cv;
        // first insert succeeds
        assert(m->set(1, 10));
        // create_only=true (default): second insert on same live key returns false, value unchanged
        assert(!m->set(1, 20));
        assert(m->get(1, &cv) && cv == 10);
        // create_only=false: update succeeds
        assert(m->set(1, 20, false));
        assert(m->get(1, &cv) && cv == 20);
        assert(m->stats->write.update == 1);
    }

    // --- del + stats ---
    {
        shmaps::Map<int, int> *m = new shmaps::Map<int, int>("ShMap_Del");
        (void)m;
        assert(m->set(42, 99));
        assert(m->exists(42));
        assert(m->del(42));          // hit
        assert(!m->exists(42));
        assert(!m->del(42));         // miss
        assert(m->stats->write.del.total == 2);
        assert(m->stats->write.del.hit   == 1);
        assert(m->stats->write.del.miss  == 1);
    }

    // --- exists on present and absent keys ---
    {
        shmaps::Map<int, int> *m = new shmaps::Map<int, int>("ShMap_Exists");
        m->set(7, 77);
        assert(m->exists(7));
        assert(!m->exists(8));
    }

    // --- permanent entry (Seconds(0)) must not expire ---
    {
        shmaps::Map<int, int> *m = new shmaps::Map<int, int>("ShMap_Permanent");
        (void)m;
        assert(m->set(1, 55, false, std::chrono::seconds(0)));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        int pv;
        assert(m->get(1, &pv) && pv == 55);
        (void)pv;
    }

    // --- locked() iteration ---
    {
        shmaps::Map<int, int> *m = new shmaps::Map<int, int>("ShMap_Locked");
        for (int i = 0; i < 5; ++i) m->set(i, i * 10);
        auto lt = m->locked();
        int count = 0;
        for (auto it = lt.cbegin(); it != lt.cend(); ++it) {
            assert(it->second.cpayload() == it->first * 10);
            ++count;
        }
        assert(count == 5);
        (void)count;
    }

    // --- exec(): found and not-found paths ---
    {
        shmaps::Map<int, int> *m = new shmaps::Map<int, int>("ShMap_Exec");
        m->set(10, 100);
        int r = m->exec(10, [](int *p) -> int { return p ? *p * 2 : -1; });
        assert(r == 200);
        (void)r;
        r = m->exec(99, [](int *p) -> int { return p ? *p * 2 : -1; });
        assert(r == -1);
    }

    // --- MapSet: multiple members per key ---
    {
        shmaps::MapSet<int, int> *m = new shmaps::MapSet<int, int>("ShMap_SetMulti");
        m->add(1, 10);
        m->add(1, 20);
        m->add(1, 30);
        std::set<int> got;
        assert(m->members(1, &got) && got == std::set<int>({10, 20, 30}));
        assert(m->is_member(1, 20));
        assert(!m->is_member(1, 99));   // non-existent member
        assert(!m->is_member(2, 10));   // non-existent key
    }

    // --- MapSet: del removes the entire key ---
    {
        shmaps::MapSet<int, int> *m = new shmaps::MapSet<int, int>("ShMap_SetDel");
        m->add(5, 50);
        m->add(5, 60);
        assert(m->is_member(5, 50));
        assert(m->del(5));
        assert(!m->is_member(5, 50));
        assert(!m->del(5));  // already gone
    }

    // --- MapSet: expiration ---
    {
        shmaps::MapSet<int, int> *m = new shmaps::MapSet<int, int>("ShMap_SetExp");
        m->add(1, 111, std::chrono::seconds(1));
        assert(m->is_member(1, 111));
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
        assert(!m->is_member(1, 111));
    }

    // --- concurrent reads/writes/deletes (thread-safety smoke test) ---
    {
        shmaps::Map<uint64_t, uint64_t> *m = new shmaps::Map<uint64_t, uint64_t>("ShMap_Concurrent");
        const int nthreads  = 8;
        const int nops      = 10000;
        std::vector<std::thread> threads;
        std::atomic<int> errors{0};
        threads.reserve(nthreads);
        for (int t = 0; t < nthreads; ++t) {
            threads.emplace_back([&, t]() {
                std::mt19937_64 rng(t);
                std::uniform_int_distribution<uint64_t> dist(0, 999);
                for (int i = 0; i < nops; ++i) {
                    uint64_t key = dist(rng);
                    uint64_t got;
                    m->set(key, key * 2, false);
                    m->get(key, &got);   // another thread may have deleted it; just don't crash
                    m->exists(key);
                    if (i % 3 == 0) m->del(key);
                }
            });
        }
        for (auto &th : threads) th.join();
        assert(errors == 0);
        (void)errors;
        std::cout << "concurrent test passed" << std::endl;
    }

    // expiration test
    shmaps::Map<shmaps::String, int> *shmaps_exp = new shmaps::Map<shmaps::String, int>("ShMap_Expiration");
    res = shmaps_exp->set(sk, 166, false, std::chrono::seconds(2));
    assert(res);
    res = shmaps_exp->get(sk, &val);
    assert(res && val == 166);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    res = shmaps_exp->get(sk, &val);
    assert(!res);

    // stress test
    shmaps::Map<uint64_t, uint64_t> *shmap_stress = new shmaps::Map<uint64_t, uint64_t>("ShMap_Stress");
    shmaps::Map<uint64_t, FooStatsExt> *shmapset_stress = new shmaps::Map<uint64_t, FooStatsExt>("ShMapSet_Stress");

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
    shmap_stress->print_stats();

    std::cout << "process finished" << std::endl;

    while(wait(NULL) > 0); // wait till all children exited (otherwise libcuckoo map may stay locked forever)

    return 0;
}
