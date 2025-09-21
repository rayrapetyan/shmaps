# shmaps
Persistent shared memory key-value storage supporting customized STL containers with keys expiration capabilities.

The proposed solution allows you to organize multiple independent mapping (key-value) storages in a shared memory segment.
So after your processes exited, data stays in the memory and can be re-used on the next run.

Based on a boost/interprocess and libcuckoo lock-free map, so you shouldn't worry about access synchronization.

## Limitations:
- if you want to use your own memory-allocating data types, you should always call `shmaps::init()` from ctor and use `shmaps::seg_alloc` as allocator;
- TODO: you can't declare shmaps::String before init() is called (because `shmaps::seg_alloc` is not initialized yet);
- TODO: if one of the processes crash in the middle of accessing a shared map, it may leave it locked forever.

# Dependencies
Currently only clang 10+ with llvm's c++ lib is supported. So everything should work out of the box in FreeBSD 11+.
With g++ (tested on g++ 10.2) compilation fails with a lot of errors like:
    
`error: cannot convert ‘boost::interprocess::offset_ptr`

### FreeBSD
You need to install boost libraries:

    pkg install boost-libs

### Linux
For Linux-based OS, please install following dependencies (tested in `Debian bookworm`):
(note: clang++-11 is a minimum supported compiler)

    apt install libboost-dev
    apt install cmake clang libc++-19-dev libc++abi-19-dev

Set clang++ as a default c++ compiler:

    update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 60

Then you should be able to compile your code with `-stdlib=libc++` option (this is a libcuckoo's requirement).
See `Dockerfile` for a detailed step-by-step installation.

### libcuckoo
A custom version of libcuckoo (with "erase random" support) required.

    git clone --branch erase_random https://github.com/rayrapetyan/libcuckoo.git libcuckoo
    mkdir /usr/local/include/libcuckoo
    cp libcuckoo/libcuckoo/*.hh /usr/local/include/libcuckoo

### Benchmark
Packages required:

    benchmark redis hiredis

A running instance of redis-server is required

## Compilation
shmaps is implemented as a header-only library, just include shmaps.hh into your sources.

## Init
The recommended way to init shared memory is to specify it's size (in bytes) on a building stage, e.g.:

    -DSHMAPS_SEG_SIZE=2147483648

Another approach would be to use an explicit `init()` call. This is not recommended because depending on the order of
initialization (e.g. static shared maps) this may lead to the app restart requirement (due to the shared segment size change).

```
    #include "shmaps/shmaps.hh"    
    const long est_shmem_size = 1024 * 1024 * 100; // 100MB, make sure to allocate at least x2 of size you plan to use.
    shmaps::init(est_shmem_size);
```

## Example 1: shared map of `int`s.
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    int val;
    FooStatsExt fse;
    shmaps::String sk(std::to_string(k).c_str(), *shmaps::seg_alloc);
    
    shmaps::Map<shmaps::String, int> *shmap_string_int = new shmaps::Map<shmaps::String, int>("ShMap_String_Int");
    res = shmap_string_int->set(sk, k, false, std::chrono::seconds(el_expires));
    assert(res);
    res = shmap_string_int->get(sk, &val);
    assert(res && val == k);
```

## Example 2: shared map of basic `struct`s.
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    int val;
    FooStatsExt fse;
    shmaps::String sk(std::to_string(k).c_str(), *shmaps::seg_alloc);
    
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
    
    shmaps::Map<int, FooStats> *shmap_int_foostats = new shmaps::Map<int, FooStats>("ShMap_Int_FooStats");
    res = shmap_int_foostats->set(k, FooStats(k, 2, 3.0), false, std::chrono::seconds(el_expires));
    assert(res);
    FooStats fs;
    res = shmap_int_foostats->get(k, &fs);
    assert(res && fs.k == k);
```

## Example 3: shared map of advanced structs (containing `shmaps::String`s):
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    int val;
    FooStatsExt fse;
    shmaps::String sk(std::to_string(k).c_str(), *shmaps::seg_alloc);
    
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
    
    shmaps::Map <shmaps::String, FooStatsExt> *shmap_string_foostats_ext = new shmaps::Map<shmaps::String, FooStatsExt>(
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
    shmaps::String sk(std::to_string(k).c_str(), *shmaps::seg_alloc);
    
    shmaps::MapSet<shmaps::String, int> *shmap_string_set_int = new shmaps::MapSet<shmaps::String, int>("ShMap_String_SetInt");
    res = shmap_string_set_int->add(sk, k);
    assert(res);
    res = shmap_string_set_int->is_member(sk, k);
    assert(res);
    std::set<int> res_check1 = {k};
    std::set<int> si;
    res = shmap_string_set_int->members(sk, &si);
    assert(res && si == res_check1);
```

## Example 5: shared map of advanced sets (`bip::set<shmaps::String>`):
```
    const int el_expires = 2;
    bool res;
    int k = 100;
    int val;
    FooStatsExt fse;
    shmaps::String sk(std::to_string(k).c_str(), *shmaps::seg_alloc);
    
    shmaps::MapSet <shmaps::String, shmaps::String> *shmap_string_set_string = new shmaps::MapSet<shmaps::String, shmaps::String>("ShMap_String_SetString");
    res = shmap_string_set_string->add(sk, shmaps::String(sk.c_str(), *shmaps::seg_alloc), std::chrono::seconds(el_expires));
    assert(res);
    std::set<shmaps::String> res_check2 = {shmaps::String(sk.c_str(), *shmaps::seg_alloc)};
    std::set<shmaps::String> ss;
    res = shmap_string_set_string->members(sk, &ss);
    assert(res && ss == res_check2);
```

## Build and run shmaps tests and benchmarks in the isolated env

    make test
    make bench   

## Credits

Written by Robert Ayrapetyan (robert.ayrapetyan@gmail.com).

No copyright. This work is dedicated to the public domain.
For full details, see https://creativecommons.org/publicdomain/zero/1.0/

The third-party libraries have their own licenses, as detailed in their source files.
