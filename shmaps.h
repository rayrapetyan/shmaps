#ifndef SHARED_MAP_H
#define SHARED_MAP_H


#include <unistd.h>
#include <iostream>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/named_sharable_mutex.hpp>
#include <boost/unordered_map.hpp>
#include <boost/utility.hpp>
#include <boost/interprocess/detail/managed_memory_impl.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/allocators/private_node_allocator.hpp>
#include <boost/interprocess/allocators/node_allocator.hpp>

namespace bip = boost::interprocess;

namespace shared_memory {

    typedef bip::managed_shared_memory::segment_manager SegmentManager;
    typedef bip::node_allocator<void, SegmentManager> VoidAllocator;
    typedef bip::private_node_allocator<void, SegmentManager> PrivateVoidAllocator;
    typedef bip::private_node_allocator<char, SegmentManager> PrivateCharAllocator;
    typedef bip::basic_string<char, std::char_traits<char>, PrivateCharAllocator> String;

    typedef bip::interprocess_mutex MutexType;
    typedef bip::scoped_lock<MutexType> LockType;

    //const uint shmem_seg_size = 1000000;
    const std::string shmem_seg_name = "SharedMemorySegment";
    //static bip::managed_shared_memory *segment_ = new bip::managed_shared_memory(bip::open_or_create, shmem_seg_name.c_str(), shmem_seg_size);
    extern bip::managed_shared_memory *segment_;
    extern PrivateVoidAllocator *seg_alloc;

    const uint purge_min_recs = 100000;
    const uint purge_recs = 2;
    const uint purge_every = 1;
    const uint purge_range = 10;

    long init(long size);
    void remove();
    long grow(long add_size);
    long size();

    struct Stats {
        struct {
            struct {
                unsigned long long total;
                unsigned long long expiring;
                unsigned long long permanent;
            } insert;
            struct {
                unsigned long long total;
                unsigned long long hit;
                unsigned long long miss;
            } purge;
            unsigned long long update;
        } write;

        struct {
            unsigned long long total;
            unsigned long long hit;
            unsigned long long miss;
        } read;

        void info() {
            fprintf(stdout, "    stats\n"
                            "        inserts: %llu (%d%% expiring)\n"
                            "        updates: %llu\n"
                            "        purges: %llu/%llu (%d%% hits)\n"
                            "        reads: %llu/%llu (%d%% hits)\n",
                    write.insert.total,
                    write.insert.total ? static_cast<int>(write.insert.expiring * 100 / write.insert.total) : 0,
                    write.update,
                    write.purge.hit,
                    write.purge.total,
                    write.purge.total ? static_cast<int>(write.purge.hit * 100 / write.purge.total) : 0,
                    read.hit,
                    read.total,
                    read.total ? static_cast<int>(read.hit * 100 / read.total) : 0);
        }
    };





    template<class PayloadType>  // PayloadType could be a set or int or complex struct (with strings)
    class MappedValType {
    public:
        MappedValType() { }

        MappedValType(time_t expires_at, const PrivateVoidAllocator &void_alloc) : payload(void_alloc),
                                                                                   expires_at(expires_at) { }

        MappedValType(const PayloadType &payload, const time_t &expires_at) : payload(payload),
                                                                              expires_at(expires_at) { }

        ~MappedValType() { }

        PayloadType payload;
        time_t expires_at;
    };


    template<class KeyType, class PayloadType>
    class Map {

    public:
        Map() { };

        explicit Map(const std::string &name) : map_name_(name) {
            //std::cout << map_name_ << " ctor" << std::endl << std::flush;

            if (segment_ == nullptr) {
                segment_ = new bip::managed_shared_memory(bip::open_only, shmem_seg_name.c_str());
            }
            assert(segment_ != nullptr);
            map_ = segment_->find_or_construct<MapImpl>(map_name_.data())(3,
                                                                          boost::hash<KeyType>(),
                                                                          std::equal_to<KeyType>(),
                                                                          VoidAllocator(segment_->get_segment_manager()));
            assert(map_ != nullptr);
            mutex_ = segment_->find_or_construct<MutexType>(std::string(name + "mutex").data())();
            assert(mutex_ != nullptr);
            stats = segment_->find_or_construct<Stats>(std::string(name + "stats").data())();
            assert(stats != nullptr);

            info();
        }

        ~Map() {
            //std::cout << map_name_ << " dtor" << std::endl;
        }

        MutexType& mutex() {
            return *mutex_;
        }


        void info() {
            fprintf(stdout, "shared memory segment of size %luMB (%luMB free)\n"
                            "    map %s (elements: %lu)\n",
                    segment_->get_size() / 1000000,
                    segment_->get_free_memory() / 1000000,
                    map_name_.c_str(),
                    map_->size());
            stats->info();
        }


        void destroy() {
            if (segment_ == nullptr) {
                // map was already destroyed from elsewhere, not a bug as segment_ is static
                return;
            }

            /*
            fprintf(stdout, "removing map %s from segment of size %luMB (%luMB free)\n",
                    map_name_.c_str(), segment_->get_size() / 1000000,
                    segment_->get_free_memory() / 1000000);
            */

            segment_->destroy<MutexType>((map_name_ + "mutex").c_str());
            segment_->destroy<MapImpl>(map_name_.c_str());
            return;
        }

        void clear() {
            LockType lock(*mutex_);
            map_->clear();
            return;
        }

        bool set(const char *k, const PayloadType &val, bool create_only = true, int expires = 0) {
            time_t unix_now = time(NULL), expires_at = 0;
            if (expires) {
                expires_at = unix_now + expires;
            }
            String k_(k, PrivateVoidAllocator(segment_->get_segment_manager()));
            auto it = map_->find(k_);
            if (it == map_->end()) {
                auto ins_res = map_->insert(ValueType(k_, MappedValType<PayloadType>(val, expires_at)));
                ++stats->write.insert.total;
                if (expires_at != 0) {
                    ++stats->write.insert.expiring;
                } else {
                    ++stats->write.insert.permanent;
                }
                it = ins_res.first;
            } else {
                if ((*it).second.expires_at && ((*it).second.expires_at < unix_now)) { // expired
                    (*it).second.payload = val;
                    (*it).second.expires_at = expires_at;
                    ++stats->write.insert.total;
                    if (expires_at != 0) {
                        ++stats->write.insert.expiring;
                    } else {
                        ++stats->write.insert.permanent;
                    }
                } else {
                    if (create_only)
                        return false;
                    (*it).second.payload = val;
                    ++stats->write.update;
                }
            }
            if (stats->write.insert.total % purge_every == 0) {
                purge_expired(boost::next(it), purge_recs);
            }
            return true;
        }

        bool get(const char *k, PayloadType *val) {
            String k_(k, PrivateVoidAllocator(segment_->get_segment_manager()));
            auto it = map_->find(k_);
            if (it != map_->end()) {
                if ((*it).second.expires_at && (*it).second.expires_at < time(NULL)) {
                    map_->erase(it);
                    ++stats->write.purge.total;
                    ++stats->write.purge.hit;
                    ++stats->read.total;
                    ++stats->read.miss;
                    return false;
                }
                *val = (*it).second.payload;
                ++stats->read.total;
                ++stats->read.hit;
                return true;
            } else {
                ++stats->read.total;
                ++stats->read.miss;
                return false;
            }
        }

        bool set(const KeyType &k, const PayloadType &val, bool create_only = true, int expires = 0) {
            time_t unix_now = time(NULL), expires_at = 0;
            if (expires) {
                expires_at = unix_now + expires;
            }
            auto it = map_->find(k);
            if (it == map_->end()) {
                auto ins_res = map_->insert(ValueType(k, MappedValType<PayloadType>(val, expires_at)));
                ++stats->write.insert.total;
                if (expires_at != 0) {
                    ++stats->write.insert.expiring;
                } else {
                    ++stats->write.insert.permanent;
                }
                it = ins_res.first;
            } else {
                if ((*it).second.expires_at && ((*it).second.expires_at < unix_now)) { // expired
                    (*it).second.payload = val;
                    (*it).second.expires_at = expires_at;
                    ++stats->write.insert.total;
                    if (expires_at != 0) {
                        ++stats->write.insert.expiring;
                    } else {
                        ++stats->write.insert.permanent;
                    }
                } else {
                    if (create_only)
                        return false;
                    (*it).second.payload = val;
                    ++stats->write.update;
                }
            }
            if (stats->write.insert.total % purge_every == 0) {
                purge_expired(boost::next(it), purge_recs);
            }
            return true;
        }

        bool get(const KeyType &k, PayloadType *val) {
            auto it = map_->find(k);
            if (it != map_->end()) {
                if ((*it).second.expires_at && (*it).second.expires_at < time(NULL)) {
                    map_->erase(it);
                    ++stats->write.purge.total;
                    ++stats->write.purge.hit;
                    ++stats->read.total;
                    ++stats->read.miss;
                    return false;
                }
                *val = (*it).second.payload;
                ++stats->read.total;
                ++stats->read.hit;
                return true;
            } else {
                ++stats->read.total;
                ++stats->read.miss;
                return false;
            }
        }
        Stats *stats;

    protected:
        typedef std::pair<const KeyType, MappedValType<PayloadType> > ValueType;
        typedef bip::node_allocator<ValueType, SegmentManager> ShmemAllocator;
        //typedef map<KeyType, MappedTypeExp<ValPayloadType>, std::less<KeyType>, ShmemAllocator> MapImpl;
        typedef boost::unordered_map<KeyType, MappedValType<PayloadType>, boost::hash<KeyType>, std::equal_to<KeyType>, ShmemAllocator> MapImpl;
        typedef typename MapImpl::iterator MapImplIterator;

        MapImpl *map_;
        std::string map_name_;
        MutexType *mutex_;



        void purge_expired(MapImplIterator it, const uint count) {
            // tries to purge expired entries
            // called on each successfull "set" and "add" with a current it + 1
            // purge_expired(map_.begin(), map_.size()) will purge all expired records

            if (map_->size() < purge_min_recs) {
                return;
            }

            if (it == map_->end()) {
                return;
            }
            time_t unix_now = time(NULL);
            MapImplIterator cur_it = it;
            uint del = 0;
            for (int i = 0; (i < purge_range) && (del < count) && (cur_it != map_->end()); ++i) {
                if ((*cur_it).second.expires_at < unix_now) {
                    cur_it = map_->erase(cur_it);
                    ++del;
                    ++stats->write.purge.total;
                    ++stats->write.purge.hit;
                } else {
                    ++stats->write.purge.total;
                    ++stats->write.purge.miss;
                    cur_it++;
                }
            }
            return;
        }
    };

    template<class KeyType, class ValType>
    class MapSet : public Map<KeyType, bip::set<ValType, std::less<ValType>, bip::private_node_allocator<ValType, SegmentManager>>> {

        typedef bip::set<ValType, std::less<ValType>, bip::private_node_allocator<ValType, SegmentManager>> PayloadType;

        using Map<KeyType, PayloadType>::mutex_;
        using Map<KeyType, PayloadType>::map_;
        using Map<KeyType, PayloadType>::stats;
        using ValueType = typename Map<KeyType, PayloadType>::ValueType;
        using Map<KeyType, PayloadType>::purge_expired;

    public:
        MapSet() : Map<KeyType, PayloadType>() { };

        explicit MapSet(const std::string &name) : Map<KeyType, PayloadType>(name) { };

        ~MapSet() { };

        bool add(const KeyType &k, const ValType &val, int expires = 0) {
            // add one or more members to a set
            time_t unix_now = time(NULL), expires_at = 0;
            if (expires) {
                expires_at = unix_now + expires;
            }
            auto it = map_->find(k);
            if (it == map_->end()) {
                MappedValType<PayloadType> mapped_val(expires_at, PrivateVoidAllocator(segment_->get_segment_manager()));
                mapped_val.payload.insert(val);
                auto ins_res = map_->insert(ValueType(k, mapped_val));
                ++stats->write.insert.total;
                if (expires_at != 0) {
                    ++stats->write.insert.expiring;
                } else {
                    ++stats->write.insert.permanent;
                }
                it = ins_res.first;
            } else {
                if ((*it).second.expires_at && ((*it).second.expires_at < unix_now)) { // expired
                    (*it).second.payload.clear();
                    (*it).second.payload.insert(val);
                    (*it).second.expires_at = expires_at;
                    ++stats->write.insert.total;
                    if (expires_at != 0) {
                        ++stats->write.insert.expiring;
                    } else {
                        ++stats->write.insert.permanent;
                    }
                } else {
                    (*it).second.payload.insert(val);
                    ++stats->write.update;
                }
            }
            if (stats->write.insert.total % purge_every == 0) {
                purge_expired(it, purge_recs);
            }
            return true;
        }

        bool add(const std::string &k, const ValType val, int expires = 0) {
            // add one or more members to a set
            time_t unix_now = time(NULL), expires_at = 0;
            if (expires) {
                expires_at = unix_now + expires;
            }
            PrivateVoidAllocator alloc_inst(segment_->get_segment_manager());
            String k_(k.c_str(), alloc_inst);
            auto it = map_->find(k_);
            if (it == map_->end()) {
                MappedValType<PayloadType> mapped_val(expires_at, alloc_inst);
                mapped_val.payload.insert(val);
                auto ins_res = map_->insert(ValueType(k_, mapped_val));
                ++stats->write.insert.total;
                if (expires_at != 0) {
                    ++stats->write.insert.expiring;
                } else {
                    ++stats->write.insert.permanent;
                }
                it = ins_res.first;
            } else {
                if ((*it).second.expires_at && ((*it).second.expires_at < unix_now)) { // expired
                    (*it).second.payload.clear();
                    (*it).second.payload.insert(val);
                    (*it).second.expires_at = expires_at;
                    ++stats->write.insert.total;
                    if (expires_at != 0) {
                        ++stats->write.insert.expiring;
                    } else {
                        ++stats->write.insert.permanent;
                    }
                } else {
                    (*it).second.payload.insert(val);
                    ++stats->write.update;
                }
            }
            if (stats->write.insert.total % purge_every == 0) {
                purge_expired(it, purge_recs);
            }
            return true;
        }


        bool add(const std::string &k, const std::string &val, int expires = 0) {
            // add one or more members to a set
            time_t unix_now = time(NULL), expires_at = 0;
            if (expires) {
                expires_at = unix_now + expires;
            }
            PrivateVoidAllocator alloc_inst(segment_->get_segment_manager());
            String k_(k.c_str(), alloc_inst);
            String val_(val.c_str(), alloc_inst);
            auto it = map_->find(k_);
            if (it == map_->end()) {
                MappedValType<PayloadType> mapped_val(expires_at, alloc_inst);
                mapped_val.payload.insert(val_);
                auto ins_res = map_->insert(ValueType(k_, mapped_val));
                ++stats->write.insert.total;
                if (expires_at != 0) {
                    ++stats->write.insert.expiring;
                } else {
                    ++stats->write.insert.permanent;
                }
                it = ins_res.first;
            } else {
                if ((*it).second.expires_at && ((*it).second.expires_at < unix_now)) { // expired
                    (*it).second.payload.clear();
                    (*it).second.payload.insert(val_);
                    (*it).second.expires_at = expires_at;
                    ++stats->write.insert.total;
                    if (expires_at != 0) {
                        ++stats->write.insert.expiring;
                    } else {
                        ++stats->write.insert.permanent;
                    }
                } else {
                    (*it).second.payload.insert(val_);
                    ++stats->write.update;
                }
            }
            if (stats->write.insert.total % purge_every == 0) {
                purge_expired(it, purge_recs);
            }
            return true;
        }

        bool members(const KeyType &k, std::set<ValType> *val) {
            auto it = map_->find(k);
            if (it != map_->end()) {
                if ((*it).second.expires_at && (*it).second.expires_at < time(NULL)) {
                    map_->erase(it);
                    ++stats->write.purge.total;
                    ++stats->write.purge.hit;
                    ++stats->read.total;
                    ++stats->read.miss;
                    return false;
                }
                for (auto it_ = (*it).second.payload.begin(); it_ != (*it).second.payload.end(); ++it_) {
                    val->insert((*it_));
                }
                ++stats->read.total;
                ++stats->read.hit;
                return true;
            } else {
                ++stats->read.total;
                ++stats->read.miss;
                return false;
            }
        }


        bool members(const std::string &k, std::set<ValType> *val) {
            String k_(k.c_str(), PrivateVoidAllocator(segment_->get_segment_manager()));
            auto it = map_->find(k_);
            if (it != map_->end()) {
                if ((*it).second.expires_at && (*it).second.expires_at < time(NULL)) {
                    map_->erase(it);
                    ++stats->write.purge.total;
                    ++stats->write.purge.hit;
                    ++stats->read.total;
                    ++stats->read.miss;
                    return false;
                }
                //*val = (*it).second.payload;
                for (auto it_ = (*it).second.payload.begin(); it_ != (*it).second.payload.end(); ++it_) {
                    val->insert((*it_));
                }
                ++stats->read.total;
                ++stats->read.hit;
                return true;
            } else {
                ++stats->read.total;
                ++stats->read.miss;
                return false;
            }
        }

        bool members(const std::string &k, std::set<std::string> *val) {
            String k_(k.c_str(), PrivateVoidAllocator(segment_->get_segment_manager()));
            auto it = map_->find(k_);
            if (it != map_->end()) {
                if ((*it).second.expires_at && (*it).second.expires_at < time(NULL)) {
                    map_->erase(it);
                    ++stats->write.purge.total;
                    ++stats->write.purge.hit;
                    ++stats->read.total;
                    ++stats->read.miss;
                    return false;
                }
                //*val = (*it).second.payload;
                for (auto it_ = (*it).second.payload.begin(); it_ != (*it).second.payload.end(); ++it_) {
                    val->insert((*it_).c_str());
                }
                ++stats->read.total;
                ++stats->read.hit;
                return true;
            } else {
                ++stats->read.total;
                ++stats->read.miss;
                return false;
            }
        }

        bool is_member(const std::string &k, const ValType val) {
            String k_(k.c_str(), PrivateVoidAllocator(segment_->get_segment_manager()));
            auto it = map_->find(k_);
            if (it != map_->end()) {
                if ((*it).second.expires_at && (*it).second.expires_at < time(NULL)) {
                    map_->erase(it);
                    ++stats->write.purge.total;
                    ++stats->write.purge.hit;
                    ++stats->read.total;
                    ++stats->read.miss;
                    return false;
                }
                bool res = (*it).second.payload.find(val) != (*it).second.payload.end();
                if (res) {
                    ++stats->read.total;
                    ++stats->read.hit;
                } else {
                    ++stats->read.total;
                    ++stats->read.miss;
                }
                return res;
            } else {
                ++stats->read.total;
                ++stats->read.miss;
                return false;
            }
        }

        bool is_member(const KeyType &k, const ValType val) {
            auto it = map_->find(k);
            if (it != map_->end()) {
                if ((*it).second.expires_at && (*it).second.expires_at < time(NULL)) {
                    map_->erase(it);
                    ++stats->write.purge.total;
                    ++stats->write.purge.hit;
                    ++stats->read.total;
                    ++stats->read.miss;
                    return false;
                }
                bool res = (*it).second.payload.find(val) != (*it).second.payload.end();
                if (res) {
                    ++stats->read.total;
                    ++stats->read.hit;
                } else {
                    ++stats->read.total;
                    ++stats->read.miss;
                }
                return res;
            } else {
                ++stats->read.total;
                ++stats->read.miss;
                return false;
            }
        }
    };

}
#endif // SHARED_MAP_H
