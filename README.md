# shmaps
Persistent shared memory maps with STL containers and TTL support.

The proposed solution allows you to organize multiple independent mapping (key-value) storages in a shared memory segment.

Based on a boost/interprocess module.

UPD:
- new lock-free map implementation is used internally (a patched libcuckoo), so no mutexes are needed for syncing access to the shared maps anymore. 
Also this implementation is magnitude times faster than previous (using boost unordered_map and mutexes).
It also consumes much less memory.
- refactored expiration logic (std::chrono is used for all timing operations)
- added bench and tests


## Compilation
You should add shmaps.cpp and shmaps.h into your sources tree.

## Init
```
    #include "shmaps.h"
    
    namespace shmem = shared_memory;
    
    const long est_shmem_size = 1024 * 1024 * 1; // 1MB
    
    if (shmem::init(est_shmem_size) != est_shmem_size) {
        // shmaps segment of a different size already initilized, drop and recreate
        shmem::remove();
        if (shmem::init(est_shmem_size) != est_shmem_size) {
            return 1;
        }
    }
```

## Example 1: shared map of int-s.
`ShMap_String_Int` is opened or created and is available from any other process in the system
```
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
```

## Example 2: shared map of basic structs.
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    std::string sk = std::to_string(k);
    
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

## Example 3: shared map of advanced structs (containing std::string-s):
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    std::string sk = std::to_string(k);
    
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
```

## Example 4: shared map of basic sets (`std::set<int>`):
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    std::string sk = std::to_string(k);
    
    shmem::MapSet<std::string, int> *shmap_string_set_int = new shmem::MapSet<std::string, int>("ShMap_String_SetInt");
    res = shmap_string_set_int->add(sk, k);
    assert(res);
    
    res = shmap_string_set_int->is_member(sk, k);
    assert(res);
    
    std::set<int> res_check1 = {k};
    std::set<int> si;
    res = shmap_string_set_int->members(sk, &si);
    assert(res && si == res_check1);
```

## Example 5: shared map of advanced sets (`std::set<std::string>`):
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    std::string sk = std::to_string(k);
    
    shmem::MapSet <std::string, shmem::String> *shmap_string_set_string =
            new shmem::MapSet<std::string, shmem::String>("ShMap_String_SetString");
    res = shmap_string_set_string->add(sk, shmem::String(sk.c_str(), *shmem::seg_alloc), std::chrono::seconds(el_expires));
    assert(res);
    
    std::set<shmem::String> res_check2 = {shmem::String(sk.c_str(), *shmem::seg_alloc)};
    std::set<shmem::String> ss;
    res = shmap_string_set_string->members(sk, &ss);
    assert(res && ss == res_check2);
```

## Clean-up (usually never used as shmaps are persistent by nature):
```
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
```

Written by Robert Ayrapetyan (robert.ayrapetyan@gmail.com).

No copyright. This work is dedicated to the public domain.
For full details, see https://creativecommons.org/publicdomain/zero/1.0/

The third-party libraries have their own licenses, as detailed in their source files.
