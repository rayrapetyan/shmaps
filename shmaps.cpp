#include "./shmaps.h"

namespace shared_memory {

    bip::managed_shared_memory *segment_ = nullptr;

    VoidAllocator *seg_alloc = nullptr;

    uint64_t init(uint64_t size) {
        if (segment_ == nullptr) {
            segment_ = new bip::managed_shared_memory(bip::open_or_create, SHMEM_SEG_NAME, size);
            assert(segment_ != nullptr);
            seg_alloc = new VoidAllocator(segment_->get_segment_manager());
            assert(seg_alloc != nullptr);
        }
        return segment_->get_size();
    }

    void remove() {
        if (segment_ == nullptr) {
            return;
        }
        bip::shared_memory_object::remove(SHMEM_SEG_NAME);
        segment_ = nullptr;
        return;
    }

    uint64_t grow(uint64_t add_size) {
        assert(segment_);
        uint cur_seg_size = segment_->get_size();
        delete segment_;
        bip::managed_shared_memory::grow(shmem_seg_name.c_str(), add_size);
        segment_ = new bip::managed_shared_memory(bip::open_only, shmem_seg_name.c_str());
        assert(segment_->get_size() == cur_seg_size + add_size);
        return cur_seg_size + add_size;
    }

    uint64_t size() {
        assert(segment_);
        return segment_->get_size();
    }

    uint64_t get_memory_size() {
        long pages = sysconf(_SC_PHYS_PAGES);
        long page_size = sysconf(_SC_PAGE_SIZE);
        return pages * page_size;
    }
}
