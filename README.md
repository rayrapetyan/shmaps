# shmaps
Persistent shared memory maps with STL containers and TTL support

The proposed solution allows you to organize multiple independent mapping (key-value) storages in a shared memory segment.
Under the hood a boost/interprocess is being used.

## Compilation
You should add shmaps.cpp and shmaps.h into your sources tree.

## Init
```
    namespace shmem = shared_memory;
    
    const long est_shmem_size = 1024 * 1024 * 1024; // 1MB
    
    if (shmem::init(est_shmem_size) != est_shmem_size) {
        shmem::remove();
        if (shmem::init(est_shmem_size) != est_shmem_size) {
            return 1;
        }
    }
```

## Example 1: shared map of int-s.
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    
    shmem::Map<shmem::String, int> *shmap_string_int = new shmem::Map<shmem::String, int>("ShMapStringInt");
    {
        shmem::LockType lock(shmap_string_int->mutex());
        res = shmap_string_int->set(std::to_string(k).c_str(), k, el_expires);
        assert(res);
    }
    int val;
    {
        shmem::LockType lock(shmap_string_int->mutex());
        res = shmap_string_int->get(std::to_string(k).c_str(), &val);
        assert(res && val == k);
    }
```

## Example 2: shared map of basic structs.
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    
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
    shmem::Map<int, FooStats> *shmap_int_foostats = new shmem::Map<int, FooStats>("ShMapIntFooStats");
    {
        shmem::LockType lock(shmap_int_foostats->mutex());
        res = shmap_int_foostats->set(k, FooStats(k, 2, 3.0), false, el_expires);
        assert(res);
    }
    FooStats fs;
    {
        shmem::LockType lock(shmap_int_foostats->mutex());
        res = shmap_int_foostats->get(k, &fs);
        assert(res && fs.k == k);
    }
```

## Example 3: shared map of advanced structs (containing std::string-s):
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    
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
    shmem::Map<shmem::String, FooStatsExt> *shmap_string_foostats_ext = new shmem::Map<shmem::String, FooStatsExt>("ShMapStringFooStatsExt");
    {
        std::string sk = std::to_string(k);
        shmem::LockType lock(shmap_string_foostats_ext->mutex());
        res = shmap_string_foostats_ext->set(sk.c_str(),
                                             FooStatsExt(k, sk.c_str(), sk.c_str()),
                                             false,
                                             el_expires);
        assert(res);
    }
    FooStatsExt fse;
    {
        std::string s = std::to_string(k);
        shmem::LockType lock(shmap_string_foostats_ext->mutex());
        res = shmap_string_foostats_ext->get(s.c_str(), &fse);
        assert(res && (fse.i1 == k) && (std::string(fse.s1.c_str()) == s) && std::string(fse.s2.c_str()) == s);
    }
```

## Example 4: shared map of basic sets (`std::set<int>`):
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    
    shmem::MapSet<shmem::String, int> *shmap_string_set_int = new shmem::MapSet<shmem::String, int>("ShMapStringSetInt");
    {
        shmem::LockType lock(shmap_string_set_int->mutex());
        res = shmap_string_set_int->add(std::to_string(k), k);
        assert(res);
    }
    std::set<int> res_check1 = {k};
    std::set<int> si;
    {
        shmem::LockType lock(shmap_string_set_int->mutex());
        res = shmap_string_set_int->members(std::to_string(k), &si);
        assert(res && si == res_check1);
    }
```

## Example 5: shared map of advanced sets (`std::set<std::string>`):
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    
    shmem::MapSet<shmem::String, shmem::String> *shmap_string_set_string = new shmem::MapSet<shmem::String, shmem::String>("ShMapStringSetString");
    {
        shmem::LockType lock(shmap_string_set_string->mutex());
        res = shmap_string_set_string->add(std::to_string(k), std::to_string(k), el_expires);
        assert(res);
    }
    std::set<std::string> res_check2 = {std::to_string(k)};
    std::set<std::string> ss;
    {
        shmem::LockType lock(shmap_string_set_string->mutex());
        res = shmap_string_set_string->members(std::to_string(k), &ss);
        assert(res && ss == res_check2);
    }
```

## Clean-up (usually never needed as shmaps are persistent by nature):
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
