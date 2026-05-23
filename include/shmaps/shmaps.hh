#ifndef SHMAPS_H
#define SHMAPS_H

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <boost/functional/hash.hpp>
#include <boost/heap/priority_queue.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/detail/managed_memory_impl.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/utility.hpp>

#include <libcuckoo/cuckoohash_map.hh>
#include <set>

#define SHMEM_SEG_NAME "SharedMemorySegment"

// you should define SHMAPS_SEG_SIZE on a building stage (e.g. cmake -DSHMAPS_SEG_SIZE=2147483648 ..)
#ifndef SHMAPS_SEG_SIZE
#define SHMAPS_SEG_SIZE 1024 * 1024 * 1024 // 1GB
#endif

#define INIT_MAP_SIZE libcuckoo::DEFAULT_SIZE

namespace bip = boost::interprocess;

namespace shmaps {
    using uint = unsigned int;

    typedef bip::managed_shared_memory::segment_manager SegmentManager;
    typedef bip::allocator<void, SegmentManager> VoidAllocator;
    typedef bip::allocator<char, SegmentManager> CharAllocator;
    // TODO: redeclare String so app doesn't crash if String is defined before init() is called (null allocator)
    typedef bip::basic_string<char, std::char_traits<char>, CharAllocator> String;

    typedef std::chrono::time_point<std::chrono::steady_clock> TimePoint;
    typedef std::chrono::seconds Seconds;

    template<typename T> using TAllocator = bip::allocator<T, SegmentManager>;
    template<typename T> using Vector = bip::vector<T, TAllocator<T>>;
    template<typename T> using List = bip::list<T, TAllocator<T>>;
    template<typename T> using Set = bip::set<T, std::less<T>, TAllocator<T>>;

    const std::string shmem_seg_name = SHMEM_SEG_NAME;

    inline bip::managed_shared_memory *segment_ = nullptr;
    inline VoidAllocator *seg_alloc = nullptr;

    // Serializes mutations of segment_/seg_alloc (init/reset/grow). Note this does NOT
    // protect concurrent map access during reset()/grow(); see warnings on those.
    inline std::mutex &segment_mutex_() {
        static std::mutex m;
        return m;
    }

    inline void reset() {
        /*
         do not call if you have static or other active shared tables in your app - they'll all become invalidated and
         your app will hang\crash; so you should either call reset() before anything else or restart app immediately
         after reset()
        */
        std::lock_guard<std::mutex> lock(segment_mutex_());
        bip::shared_memory_object::remove(SHMEM_SEG_NAME);

        delete seg_alloc;
        seg_alloc = nullptr;

        delete segment_;
        segment_ = nullptr;
        return;
    }

    inline uint64_t segment_size() {
        assert(segment_);
        return segment_->get_size();
    }

    inline uint64_t grow(uint64_t add_size) {
        /*
         WARNING: unsafe while any Map/MapSet objects in this process are in use.
         All previously cached map_/stats raw pointers become dangling. Stop the world
         (no concurrent map access in any process) before calling.
        */
        std::lock_guard<std::mutex> lock(segment_mutex_());
        if (segment_ == nullptr) {
            throw std::runtime_error("shmaps::grow called before init");
        }
        const uint64_t cur_seg_size = segment_->get_size();

        delete seg_alloc;
        seg_alloc = nullptr;

        delete segment_;
        segment_ = nullptr;

        try {
            bip::managed_shared_memory::grow(shmem_seg_name.c_str(), add_size);
            segment_ = new bip::managed_shared_memory(bip::open_only, shmem_seg_name.c_str());
            seg_alloc = new VoidAllocator(segment_->get_segment_manager());
        } catch (const std::exception &exc) {
            std::cerr << "shmaps::grow failed: " << exc.what() << std::endl;
            delete segment_;
            segment_ = nullptr;
            delete seg_alloc;
            seg_alloc = nullptr;
            throw;
        }

        assert(seg_alloc != nullptr);
        assert(segment_->get_size() == cur_seg_size + add_size);
        return cur_seg_size + add_size;
    }

    inline uint64_t init(uint64_t size) {
        // Thread-safe: serialize creation so concurrent static-init / explicit init() races
        // do not double-construct the segment.
        std::unique_lock<std::mutex> lock(segment_mutex_());
        if (segment_ == nullptr) {
            try {
                segment_ = new bip::managed_shared_memory(bip::open_or_create, SHMEM_SEG_NAME, size);
            }
            catch (const std::exception &exc) {
                std::cout << "error creating shared memory segment: " << exc.what() << std::endl;
                // try again: drop any half-state and remove the OS-level segment
                delete segment_;
                segment_ = nullptr;
                delete seg_alloc;
                seg_alloc = nullptr;
                bip::shared_memory_object::remove(SHMEM_SEG_NAME);
                try {
                    segment_ = new bip::managed_shared_memory(bip::open_or_create, SHMEM_SEG_NAME, size);
                } catch (const std::exception &exc2) {
                    std::cerr << "shmaps::init failed: " << exc2.what() << std::endl;
                    segment_ = nullptr;
                    throw;
                }
            }
            if (segment_ == nullptr) {
                throw std::runtime_error("shmaps::init: segment is null");
            }
            seg_alloc = new VoidAllocator(segment_->get_segment_manager());
        }
        const uint64_t cur_size = segment_->get_size();
        if (cur_size < size) {
            /*
             this happens on the very first run when there is a static map defined in your app,
             and it was instantiated with the default SHMAPS_SEG_SIZE;
             then you've called init() explicitly requesting a larger segment size.
            */
            const uint64_t add = size - cur_size;
            lock.unlock();
            grow(add);
            lock.lock();
            assert(segment_ && segment_->get_size() == size);
            std::cout << "shmaps segment size has been changed, restart your app\n" << std::endl;
            exit(0);
        }
        return segment_->get_size();
    }

    inline TimePoint now() {
        return std::chrono::steady_clock::now();
    }

    struct Stats {
        struct {
            struct {
                std::atomic<uint64_t> total{0};
                std::atomic<uint64_t> expiring{0};
                std::atomic<uint64_t> permanent{0};
                std::atomic<uint64_t> error{0};
            } insert;
            struct {
                std::atomic<uint64_t> total{0};
                std::atomic<uint64_t> hit{0};
                std::atomic<uint64_t> miss{0};
            } purge;
            struct {
                std::atomic<uint64_t> total{0};
                std::atomic<uint64_t> hit{0};
                std::atomic<uint64_t> miss{0};
            } del;
            std::atomic<uint64_t> update{0};
        } write;

        struct {
            std::atomic<uint64_t> total{0};
            std::atomic<uint64_t> hit{0};
            std::atomic<uint64_t> miss{0};
        } read;

        void print() {
            fprintf(stdout, "    stats\n"
                            "        inserts: %lu (%lu%% expiring, %lu%% errors)\n"
                            "        updates: %lu\n"
                            "        purges: %lu/%lu (%lu%% hits)\n"
                            "        deletes: %lu/%lu (%lu%% hits)\n"
                            "        reads: %lu/%lu (%lu%% hits)\n",
                    write.insert.total.load(std::memory_order_acquire),
                    write.insert.total.load(std::memory_order_acquire)
                        ? static_cast<uint64_t>(write.insert.expiring.load(std::memory_order_acquire) * 100 /
                                     write.insert.total.load(std::memory_order_acquire))
                        : 0,
                    write.insert.total.load(std::memory_order_acquire)
                        ? static_cast<uint64_t>(write.insert.error.load(std::memory_order_acquire) * 100 /
                                     write.insert.total.load(std::memory_order_acquire))
                        : 0,
                    write.update.load(std::memory_order_acquire),
                    write.purge.hit.load(std::memory_order_acquire),
                    write.purge.total.load(std::memory_order_acquire),
                    write.purge.total.load(std::memory_order_acquire)
                        ? static_cast<uint64_t>(write.purge.hit.load(std::memory_order_acquire) * 100 /
                                     write.purge.total.load(std::memory_order_acquire))
                        : 0,
                    write.del.hit.load(std::memory_order_acquire),
                    write.del.total.load(std::memory_order_acquire),
                    write.del.total.load(std::memory_order_acquire)
                        ? static_cast<uint64_t>(write.del.hit.load(std::memory_order_acquire) * 100 /
                                     write.del.total.load(std::memory_order_acquire))
                        : 0,
                    read.hit.load(std::memory_order_acquire),
                    read.total.load(std::memory_order_acquire),
                    read.total.load(std::memory_order_acquire)
                        ? static_cast<uint64_t>(read.hit.load(std::memory_order_acquire) * 100 /
                                     read.total.load(std::memory_order_acquire))
                        : 0);
        }
    };

    template<class PayloadType>  // PayloadType could be a simple int, a set or a complex struct (with strings)
    class MappedValType {
    public:
        MappedValType() {}

        MappedValType(const Seconds &ttl, const VoidAllocator &void_alloc) :
                payload_(void_alloc),
                created_at_(now()),
                ttl_(ttl) {}

        MappedValType(const PayloadType &payload, const Seconds &ttl) :
                payload_(payload),
                created_at_(now()),
                ttl_(ttl) {}

        ~MappedValType() {}

        bool expired() const {
            return ttl_ != Seconds(0) && (now() - created_at_ > ttl_);
        }

        void reset(const PayloadType &payload, const Seconds &ttl) {
            reset(payload);
            reset(ttl);
        }

        void reset(const Seconds &ttl) {
            created_at_ = now();
            ttl_ = ttl;
        }

        void reset(const PayloadType &payload) {
            payload_ = payload;
        }

        PayloadType &payload() {
            return payload_;
        }

        const PayloadType &cpayload() const {
            return payload_;
        }

    private:
        PayloadType payload_;
        TimePoint created_at_;
        Seconds ttl_;
    };

    template<class KeyType, class PayloadType, class Hash = boost::hash<KeyType>, class Pred = std::equal_to<KeyType>>
    class Map {
    public:
        typedef std::pair<const KeyType, MappedValType<PayloadType> > ValueType;
        typedef bip::allocator<ValueType, SegmentManager> ValueTypeAllocator;
        typedef libcuckoo::cuckoohash_map<KeyType, MappedValType<PayloadType>, Hash, Pred, ValueTypeAllocator> MapImpl;

        // No default-construct: leaves map_/stats as wild pointers. Always pass a name.
        Map() = delete;

        explicit Map(const std::string &name) : stats(nullptr), map_(nullptr), map_name_(name) {
            if (segment_ == nullptr) {
                // static map: ctor called before main()
                // normal map: created before init()
                init(SHMAPS_SEG_SIZE);
            }
            assert(segment_ != nullptr);
            map_ = segment_->find_or_construct<MapImpl>(map_name_.c_str())(
                    INIT_MAP_SIZE,
                    Hash(),
                    Pred(),
                    segment_->get_allocator<ValueType>());
            assert(map_ != nullptr);
            const std::string stats_name = name + "stats";
            stats = segment_->find_or_construct<Stats>(stats_name.c_str())();
            assert(stats != nullptr);
            print_stats();
        }

        ~Map() {
        }

        void print_stats() {
            fprintf(stdout, "shared memory segment of size %luMB (%luMB free)\n"
                            "    map %s (elements: %lu)\n",
                    segment_size() / (1 * 1024 * 1024),
                    segment_->get_free_memory() / (1 * 1024 * 1024),
                    map_name_.c_str(),
                    map_->size());
            stats->print();
        }

        void destroy() {
            if (segment_ == nullptr) {
                return;
            }
            segment_->destroy<MapImpl>(map_name_.c_str());
            return;
        }

        void clear() {
            map_->clear();
            return;
        }

        typename MapImpl::locked_table locked() {
            return map_->lock_table();
        }

        // set() return value:
        //   true  - value was inserted (new key, or existing key whose entry had expired),
        //           or value was updated (existing live key with create_only=false).
        //   false - insert into the underlying map failed (stats.write.insert.error bumped),
        //           or key already exists with a live (non-expired) entry and create_only=true.
        bool set(const KeyType &k, const PayloadType &pl, bool create_only = true, Seconds expires = Seconds(0)) {
            bool existing = false;
            if (!map_->update_fn(k, [&](MappedValType<PayloadType> &val) {
                if (val.expired()) {
                    val.reset(pl, expires);
                    ++stats->write.insert.total;
                    if (expires != Seconds(0)) {
                        ++stats->write.insert.expiring;
                    } else {
                        ++stats->write.insert.permanent;
                    }
                } else {
                    existing = true;
                    if (!create_only) {
                        val.reset(pl);
                        ++stats->write.update;
                    }
                }
            })) {
                if (!map_->insert(k, MappedValType<PayloadType>(pl, expires))) {
                    ++stats->write.insert.error;
                    return false;
                }
                purge();

                ++stats->write.insert.total;
                if (expires != Seconds(0)) {
                    ++stats->write.insert.expiring;
                } else {
                    ++stats->write.insert.permanent;
                }
                return true;
            }
            return !(create_only && existing);
        }

        bool get(const KeyType &k, PayloadType *pl) {
            bool found = false;
            map_->update_fn(k, [&](MappedValType<PayloadType> &val) {
                found = !val.expired();
                if (found) {
                    *pl = val.payload();
                }
            });
            ++stats->read.total;
            found ? ++stats->read.hit : ++stats->read.miss;
            return found;
        }

        bool exists(const KeyType &k) {
            bool found = false;
            map_->find_fn(k, [&](const MappedValType<PayloadType> &val) {
                found = !val.expired();
            });
            ++stats->read.total;
            found ? ++stats->read.hit : ++stats->read.miss;
            return found;
        }

        // IMPORTANT: callbacks passed to exec()/update_fn-based methods must not re-enter
        // this map (e.g. call set/get/exists/lock_table on the same Map) — doing so will
        // deadlock on per-bucket locks.
        bool del(const KeyType &k) {
            bool erased = map_->erase(k);
            ++stats->write.del.total;
            erased ? ++stats->write.del.hit : ++stats->write.del.miss;
            return erased;
        }

        template<typename K, typename F>
        auto exec(const K &key, F fn, PayloadType *foo = nullptr) -> decltype(fn(foo)) {
            /*
            // TODO!!!!!!!!
            decltype(fn(foo)) a;
            return a;*/
            bool found = false;
            auto res = map_->exec_fn(
                    key,
                    [&](MappedValType<PayloadType> *val, PayloadType *foo = nullptr) -> decltype(fn(foo)) {
                        found = val && !val->expired();
                        if (found) {
                            return fn(&val->payload());
                        } else {
                            return fn(nullptr);
                        }
                    });
            return res;
        }

        uint size() const {
            return map_->size();
        }

        Stats *stats;

    protected:
        MapImpl *map_;
        std::string map_name_;

        void purge() {
            /*
            const uint purge_every = 1;
            if (stats->write.insert.total % purge_every != 0)
                return;
            */
            const uint purge_elements = 4;
            uint purged_elements = map_->erase_random_fn(purge_elements, [&](MappedValType<PayloadType> &val) {
                ++stats->write.purge.total;
                return val.expired();
            });
            stats->write.purge.hit.fetch_add(purged_elements, std::memory_order_seq_cst);
            return;
        }
    };

    template<class KeyType, class SetValType, class Hash = boost::hash<KeyType>, class Pred = std::equal_to<KeyType>>
    class MapSet
            : public Map<KeyType, bip::set<SetValType, std::less<SetValType>, bip::allocator<SetValType, SegmentManager>>, Hash, Pred> {
        typedef bip::set<SetValType, std::less<SetValType>, bip::allocator<SetValType, SegmentManager>> PayloadType;
        using Map<KeyType, PayloadType, Hash, Pred>::map_;
        using Map<KeyType, PayloadType, Hash, Pred>::stats;
        using Map<KeyType, PayloadType, Hash, Pred>::purge;
        using ValueType = typename Map<KeyType, PayloadType, Hash, Pred>::ValueType;

    public:

        MapSet() = delete;

        explicit MapSet(const std::string &name) : Map<KeyType, PayloadType, Hash, Pred>(name) {};

        ~MapSet() {};

        bool add(const KeyType &k, const SetValType &pl_elem, Seconds expires = Seconds(0)) {
            // add one or more members into a set
            if (!map_->update_fn(k, [&](MappedValType<PayloadType> &val) {
                if (val.expired()) {
                    val.payload().clear();
                    val.payload().insert(pl_elem);
                    val.reset(expires);
                    ++stats->write.insert.total;
                    if (expires != Seconds(0)) {
                        ++stats->write.insert.expiring;
                    } else {
                        ++stats->write.insert.permanent;
                    }
                } else {
                    val.payload().insert(pl_elem);
                    ++stats->write.update;
                }
            })) {
                MappedValType<PayloadType> val(expires, VoidAllocator(segment_->get_segment_manager()));
                val.payload().insert(pl_elem);
                if (!map_->insert(k, val)) {
                    ++stats->write.insert.error;
                    return false;
                }
                purge();

                ++stats->write.insert.total;
                if (expires != Seconds(0)) {
                    ++stats->write.insert.expiring;
                } else {
                    ++stats->write.insert.permanent;
                }
            }
            return true;
        }

        bool members(const KeyType &k, std::set<SetValType> *pl) {
            bool found = false;
            // read-only path: use find_fn so concurrent readers don't serialize on a write lock.
            map_->find_fn(k, [&](const MappedValType<PayloadType> &val) {
                if (!val.expired()) {
                    found = true;
                    for (auto it = val.cpayload().begin(); it != val.cpayload().end(); ++it) {
                        pl->insert((*it));
                    }
                }
            });
            ++stats->read.total;
            found ? ++stats->read.hit : ++stats->read.miss;
            return found;
        }

        bool is_member(const KeyType &k, const SetValType pl_val) {
            bool found = false;
            // read-only path: use find_fn.
            map_->find_fn(k, [&](const MappedValType<PayloadType> &val) {
                found = !val.expired() && (val.cpayload().find(pl_val) != val.cpayload().end());
            });
            ++stats->read.total;
            found ? ++stats->read.hit : ++stats->read.miss;
            return found;
        }
    };
} // namespace shmaps

#endif // SHMAPS_H
