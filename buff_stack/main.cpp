#include <utility>
#include <atomic>

template<typename T, size_t n>
class Stack {
	struct TaggedIndex {
		size_t idx;
		size_t tag;
	}

	private:
		atomic<TaggedIndex> head;
		T buff[n];
		atomic<size_t> seq[n];

	public:
		Stack() : head({0, 0}) {
			for(size_t i = 0; i < n; i++) {
				seq[i].store(i, memory_order_release);
			}
		}

		bool push(T data) {
			TaggedIndex cur_head = head.load(memory_order_relaxed);
			while (true) {	
				size_t idx = cur_head.idx;
				if (idx >= n) return false;
				
				size_t cur_seq = seq[idx].load(memory_order_acquire);

				TaggedIndex next_head = {idx + 1, cur_head.tag + 1};
				if (cur_seq == idx) {
					if (head.compare_exchange_weak(cur_head, next_head, memory_order_relaxed, memory_order_relaxed)) {
						buff[idx] = data;
						seq[idx].store(idx + 1, memory_order_release);
						return true;
					}
				} else {
					// Someone else must have pushed here. We really want to check cur_seq > idx, but since cur_seq < idx can't happen we just do an else
					cur_head = head.load(memory_order_relaxed);
				}
			}
		}

		optional<T> pop() {
			TaggedIndex cur_head = head.load(memory_order_relaxed);
			while (true) {
				
			}
		}
}
