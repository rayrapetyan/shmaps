# shmaps
Persistent shared memory key-value storage supporting customized STL containers and TTL.

The proposed solution allows you to organize multiple independent mapping (key-value) storages in a shared memory segment.

Based on a boost/interprocess and libcuckoo lock-free map.

Limitations:
- you can't use any STL containers for key or value type (e.g. instead of std::string should use shmem::String);
- you can't declare shmap maps using "static" keyword, they will raise in constructor.


## Compilation
shmaps is implemented as a header-only library, just include shmaps.hh into your sources.

## Init
```
    #include "shmaps/shmaps.hh"
    namespace shmem = shared_memory;
    const long est_shmem_size = 1024 * 1024 * 100; // 100MB, make sure to allocate at least x2 of size expected to be used.
    shmem::init(est_shmem_size);
```

## Example 1: shared map of int-s.
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    int val;
    FooStatsExt fse;
    shmem::String sk(std::to_string(k).c_str(), *shmem::seg_alloc);
    
    shmem::Map<shmem::String, int> *shmap_string_int = new shmem::Map<shmem::String, int>("ShMap_String_Int");
    res = shmap_string_int->set(sk, k, false, std::chrono::seconds(el_expires));
    assert(res);
    res = shmap_string_int->get(sk, &val);
    assert(res && val == k);
```

## Example 2: shared map of basic structs.
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    int val;
    FooStatsExt fse;
    shmem::String sk(std::to_string(k).c_str(), *shmem::seg_alloc);
    
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
    
    shmem::Map<int, FooStats> *shmap_int_foostats = new shmem::Map<int, FooStats>("ShMap_Int_FooStats");
    res = shmap_int_foostats->set(k, FooStats(k, 2, 3.0), false, std::chrono::seconds(el_expires));
    assert(res);
    FooStats fs;
    res = shmap_int_foostats->get(k, &fs);
    assert(res && fs.k == k);
```

## Example 3: shared map of advanced structs (containing shmem::String-s):
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    int val;
    FooStatsExt fse;
    shmem::String sk(std::to_string(k).c_str(), *shmem::seg_alloc);
    
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
    
    shmem::Map <shmem::String, FooStatsExt> *shmap_string_foostats_ext = new shmem::Map<shmem::String, FooStatsExt>(
            "ShMap_String_FooStatsExt");
    res = shmap_string_foostats_ext->set(sk,
                                         FooStatsExt(k, sk.c_str(), sk.c_str()),
                                         false,
                                         std::chrono::seconds(el_expires));
    assert(res);
    res = shmap_string_foostats_ext->get(sk, &fse);
    assert(res && (fse.i1 == k) && (fse.s1 == sk) && (fse.s2 == sk));
```

## Example 4: shared map of basic sets (`bip::set<int>`):
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    int val;
    FooStatsExt fse;
    shmem::String sk(std::to_string(k).c_str(), *shmem::seg_alloc);
    
    shmem::MapSet<shmem::String, int> *shmap_string_set_int = new shmem::MapSet<shmem::String, int>("ShMap_String_SetInt");
    res = shmap_string_set_int->add(sk, k);
    assert(res);
    res = shmap_string_set_int->is_member(sk, k);
    assert(res);
    std::set<int> res_check1 = {k};
    std::set<int> si;
    res = shmap_string_set_int->members(sk, &si);
    assert(res && si == res_check1);
```

## Example 5: shared map of advanced sets (`bip::set<shmem::String>`):
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    int val;
    FooStatsExt fse;
    shmem::String sk(std::to_string(k).c_str(), *shmem::seg_alloc);
    
    shmem::MapSet <shmem::String, shmem::String> *shmap_string_set_string = new shmem::MapSet<shmem::String, shmem::String>("ShMap_String_SetString");
    res = shmap_string_set_string->add(sk, shmem::String(sk.c_str(), *shmem::seg_alloc), std::chrono::seconds(el_expires));
    assert(res);
    std::set<shmem::String> res_check2 = {shmem::String(sk.c_str(), *shmem::seg_alloc)};
    std::set<shmem::String> ss;
    res = shmap_string_set_string->members(sk, &ss);
    assert(res && ss == res_check2);
```

Written by Robert Ayrapetyan (robert.ayrapetyan@gmail.com).

No copyright. This work is dedicated to the public domain.
For full details, see https://creativecommons.org/publicdomain/zero/1.0/

The third-party libraries have their own licenses, as detailed in their source files.
