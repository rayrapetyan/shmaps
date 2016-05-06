#include "./shmaps.h"

namespace shared_memory {

    bip::managed_shared_memory *segment_ = nullptr;

    PrivateVoidAllocator *seg_alloc = nullptr;

    long init(long size) {
        //std::cout << "shmem init" << std::endl;
        if (segment_ == nullptr) {
            segment_ = new bip::managed_shared_memory(bip::open_or_create, shmem_seg_name.c_str(), size);
            assert (segment_ != nullptr);
            seg_alloc = new PrivateVoidAllocator(segment_->get_segment_manager());
            assert (seg_alloc != nullptr);
        }
        return segment_->get_size();
    }

    void remove() {
        if (segment_ == nullptr) {
            // segment was already destroyed from elsewhere
            return;
        }
        /*
        fprintf(stdout, "destroying shared memory segment of size %luMB (%luMB free)\n",
                segment_->get_size() / 1000000, segment_->get_free_memory() / 1000000);
        */

        bip::shared_memory_object::remove(shmem_seg_name.c_str());
        segment_ = nullptr;
        return;
    }

    long grow(long add_size) {
        std::cout << "shmem grow" << std::endl;
        assert(segment_);
        uint cur_seg_size = segment_->get_size();
        delete segment_;
        bip::managed_shared_memory::grow(shmem_seg_name.c_str(), add_size);
        segment_ = new bip::managed_shared_memory(bip::open_only, shmem_seg_name.c_str());
        assert(segment_->get_size() == cur_seg_size + add_size);
        return cur_seg_size + add_size;
    }

    long size() {
        assert(segment_);
        return segment_->get_size();
    }
}