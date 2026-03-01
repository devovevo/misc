#include <atomic>
#include <new>
#include <optional>
#include <utility>	

using namespace std;

constexpr size_t ALIGNMENT = hardware_destructive_interference_size;

template <typename T, size_t Size>
class ObjectPool {
	struct alignas(ALIGNMENT) Slot {
		union {
			T data;
		};

		size_t next_idx;

		Slot() {};
		~Slot() {};
	};

	struct alignas(16) Head {
		size_t idx;
		size_t tag;
	};
	
	public:
		ObjectPool() {
			for (size_t i = 0; i < Size - 1; i++) {
				slots[i].next_idx = i + 1;	
			}
			slots[Size - 1].next_idx = -1;

			head.store({0, 0});
		}

		template <typename... Args>
		T* allocate(Args&&... args) {
			Head cur_head = head.load(memory_order_acquire);

			while (true) {
				if (cur_head.idx == -1) return nullptr;

				Head new_head = {slots[cur_head.idx].next_idx, cur_head.tag + 1};
				if (head.compare_exchange_weak(cur_head, new_head, memory_order_release, memory_order_relaxed)) {
					T* cur_data_ptr = &slots[cur_head.idx].data;
					new (cur_data_ptr) T(forward<Args>(args)...);
					return cur_data_ptr;
				}
			}
		}

		void deallocate(T* ptr) {
			if (!ptr || ptr < slots || ptr >= slots + Size) return;
			
			size_t idx = (reinterpret_cast<Slot*>(ptr) - slots);
			Head cur_head = head.load(memory_order_acquire);

			slots[idx].data.~T();

			while (true) {
				slots[idx].next_idx = cur_head.idx;
				Head new_head = {idx, cur_head.tag + 1};

				if (head.compare_exchange_weak(cur_head, new_head, memory_order_release, memory_order_relaxed)) {
					return;
				}
			}
		}
		

	private:
		atomic<Head> head;
		Slot slots[Size];		
};
