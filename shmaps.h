#ifndef SHMAPS_H
#define SHMAPS_H

#include <unistd.h>
#include <iostream>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/utility.hpp>
#include <boost/interprocess/detail/managed_memory_impl.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include "./libcuckoo_mod/cuckoohash_map.hh"

namespace bip = boost::interprocess;

namespace shared_memory {
    typedef bip::managed_shared_memory::segment_manager SegmentManager;
    typedef bip::allocator<void, SegmentManager> VoidAllocator;
    typedef bip::allocator<char, SegmentManager> CharAllocator;
    typedef bip::basic_string<char, std::char_traits<char>, CharAllocator> String;

    const std::string shmem_seg_name = "SharedMemorySegment";
    extern bip::managed_shared_memory *segment_;
    extern VoidAllocator *seg_alloc;

    uint64_t init(uint64_t size);

    void remove();

    uint64_t grow(uint64_t add_size);

    uint64_t size();

    uint64_t get_memory_size();

    struct Stats {
        struct {
            struct {
                std::atomic<uint64_t> total;
                std::atomic<uint64_t> expiring;
                std::atomic<uint64_t> permanent;
                std::atomic<uint64_t> error;
            } insert;
            struct {
                std::atomic<uint64_t> total;
                std::atomic<uint64_t> hit;
                std::atomic<uint64_t> miss;
            } purge;
            std::atomic<uint64_t> update;
        } write;

        struct {
            std::atomic<uint64_t> total;
            std::atomic<uint64_t> hit;
            std::atomic<uint64_t> miss;
        } read;

        void info() {
            fprintf(stdout, "    stats\n"
                            "        inserts: %lu (%lu%% expiring, %lu%% errors)\n"
                            "        updates: %lu\n"
                            "        purges: %lu/%lu (%lu%% hits)\n"
                            "        reads: %lu/%lu (%lu%% hits)\n",
                    write.insert.total.load(std::memory_order_acquire),
                    write.insert.total ? static_cast<uint64_t>(write.insert.expiring * 100 / write.insert.total) : 0,
                    write.insert.total ? static_cast<uint64_t>(write.insert.error * 100 / write.insert.total) : 0,
                    write.update.load(std::memory_order_acquire),
                    write.purge.hit.load(std::memory_order_acquire),
                    write.purge.total.load(std::memory_order_acquire),
                    write.purge.total ? static_cast<uint64_t>(write.purge.hit * 100 / write.purge.total) : 0,
                    read.hit.load(std::memory_order_acquire),
                    read.total.load(std::memory_order_acquire),
                    read.total ? static_cast<uint64_t>(read.hit * 100 / read.total) : 0);
        }
    };


    template<class PayloadType>  // PayloadType could be a set or int or complex struct (with strings)
    class MappedValType {
    public:
        MappedValType() {}

        MappedValType(std::chrono::seconds &ttl, const VoidAllocator &void_alloc) :
                payload_(void_alloc),
                created_at_(std::chrono::steady_clock::now()),
                ttl_(ttl) {}

        MappedValType(const PayloadType &payload, std::chrono::seconds &ttl) :
                payload_(payload),
                created_at_(std::chrono::steady_clock::now()),
                ttl_(ttl) {}

        ~MappedValType() {}

        bool expired() const {
            return ttl_ != std::chrono::seconds(0) && (std::chrono::steady_clock::now() - created_at_ > ttl_);
        }

        void reset(const PayloadType &payload, std::chrono::seconds &ttl) {
            reset(payload);
            reset(ttl);
        }

        void reset(std::chrono::seconds &ttl) {
            created_at_ = std::chrono::steady_clock::now();
            ttl_ = ttl;
        }

        void reset(const PayloadType &payload) {
            payload_ = payload;
        }

        PayloadType &payload() {
            return payload_;
        }

    private:
        PayloadType payload_;
        std::chrono::time_point<std::chrono::steady_clock> created_at_;
        std::chrono::seconds ttl_;
    };


    template<class KeyType, class PayloadType, class Hash = boost::hash<KeyType>, class Pred = std::equal_to<KeyType>>
    class Map {
    public:
        typedef std::pair<const KeyType, MappedValType<PayloadType> > ValueType;
        typedef bip::allocator<ValueType, SegmentManager> ValueTypeAllocator;
        typedef cuckoohash_map<KeyType, MappedValType<PayloadType>, Hash, Pred, ValueTypeAllocator> MapImpl;

        Map() {};

        explicit Map(const std::string &name) : map_name_(name) {
            const uint64_t est_shmem_size = get_memory_size() / 1.01;
            if (segment_ == nullptr) {
                if (init(est_shmem_size) != est_shmem_size) {
                    remove();
                    if (init(est_shmem_size) != est_shmem_size) {
                        abort();
                    }
                }
            }
            assert(segment_ != nullptr);
            map_ = segment_->find_or_construct<MapImpl>(map_name_.data())(
                    LIBCUCKOO_DEFAULT_SIZE,
                    Hash(),
                    Pred(),
                    segment_->get_allocator<MappedValType<PayloadType>>());
            assert(map_ != nullptr);
            stats = segment_->find_or_construct<Stats>(std::string(name + "stats").data())();
            assert(stats != nullptr);

            info();
        }

        ~Map() {
        }

        void info() {
            fprintf(stdout, "shared memory segment of size %luMB (%luMB free)\n"
                            "    map %s (elements: %lu)\n",
                    segment_->get_size() / (1 * 1024 * 1024),
                    segment_->get_free_memory() / (1 * 1024 * 1024),
                    map_name_.c_str(),
                    map_->size());
            stats->info();
        }


        void destroy() {
            if (segment_ == nullptr) {
                // map was already destroyed from elsewhere, not a bug as segment_ is static
                return;
            }
            segment_->destroy<MapImpl>(map_name_.c_str());
            return;
        }

        void clear() {
            map_->clear();
            return;
        }

        bool set(const KeyType &k, const PayloadType &pl, bool create_only = true,
                 std::chrono::seconds expires = std::chrono::seconds(0)) {
            if (!map_->update_fn(k, [&](MappedValType<PayloadType> &val) {
                if (val.expired()) {
                    val.reset(pl, expires);
                    ++stats->write.insert.total;
                    if (expires != std::chrono::seconds(0)) {
                        ++stats->write.insert.expiring;
                    } else {
                        ++stats->write.insert.permanent;
                    }
                } else {
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
                if (expires != std::chrono::seconds(0)) {
                    ++stats->write.insert.expiring;
                } else {
                    ++stats->write.insert.permanent;
                }
            }
            return true;
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

        bool del(const KeyType &k) {
            return map_->erase(k);
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
            stats->write.purge.hit += purged_elements;
            return;
        }
    };

    template<class KeyType, class SetValType, class Hash = boost::hash<KeyType>, class Pred = std::equal_to<KeyType>>
    class MapSet : public Map<KeyType, bip::set<SetValType, std::less<SetValType>, bip::allocator<SetValType, SegmentManager>>, Hash, Pred> {
        typedef bip::set<SetValType, std::less<SetValType>, bip::allocator<SetValType, SegmentManager>> PayloadType;
        using Map<KeyType, PayloadType, Hash, Pred>::map_;
        using Map<KeyType, PayloadType, Hash, Pred>::stats;
        using ValueType = typename Map<KeyType, PayloadType, Hash, Pred>::ValueType;
        using Map<KeyType, PayloadType, Hash, Pred>::purge;

    public:

        MapSet() : Map<KeyType, PayloadType, Hash, Pred>() {};

        explicit MapSet(const std::string &name) : Map<KeyType, PayloadType, Hash, Pred>(name) {};

        ~MapSet() {};

        bool add(const KeyType &k, const SetValType &pl_elem, std::chrono::seconds expires = std::chrono::seconds(0)) {
            // add one or more members to a set
            if (!map_->update_fn(k, [&](MappedValType<PayloadType> &val) {
                if (val.expired()) {
                    val.payload().clear();
                    val.payload().insert(pl_elem);
                    val.reset(expires);
                    ++stats->write.insert.total;
                    if (expires != std::chrono::seconds(0)) {
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
                if (expires != std::chrono::seconds(0)) {
                    ++stats->write.insert.expiring;
                } else {
                    ++stats->write.insert.permanent;
                }
            }
            return true;
        }

        bool members(const KeyType &k, std::set<SetValType> *pl) {
            bool found = false;
            map_->update_fn(k, [&](MappedValType<PayloadType> &val) {
                if (!val.expired()) {
                    found = true;
                    for (auto it = val.payload().begin(); it != val.payload().end(); ++it) {
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
            map_->update_fn(k, [&](MappedValType<PayloadType> &val) {
                found = !val.expired() && (val.payload().find(pl_val) != val.payload().end());
            });
            ++stats->read.total;
            found ? ++stats->read.hit : ++stats->read.miss;
            return found;
        }
    };
}

#endif // SHMAPS_H
