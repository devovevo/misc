#include <utility>
#include <atomic>

template<typename T, size_t n>
class MPMCRingBuffer {

	
	private:
		atomic<size_t> head;
		atomic<size_t> tail;

		T buff[n];
		atomic<size_t> seq[n];

	public:
		MPMCRingBuffer() : head(0), tail(0) {
			for (size_t i = 0; i < n; i++) {
				seq[i].store(i, memory_order_release);
			}
		}

		bool push(T item) {
			size_t cur_head = head.load(memory_order_relaxed);
			while (true) {
				size_t idx = cur_head % n;
				size_t cur_seq = seq[idx].load(memory_order_acquire);

				if (cur_seq == cur_head) {
					if (head.compare_exchange_weak(cur_head, cur_head + 1, memory_order_relaxed, memory_order_relaxed)) {
						buff[idx] = item;
						seq[idx].store(cur_seq + 1, memory_order_release);
						return true;
					}
				} else if (cur_seq < head) {
					// The seq is old meaning someone needs to pop it for us to put here
					return false;
				} else {
					// Someone else incremented slot before we got there, so we need to look at current head since it was probs stolen
					cur_head = head.load(memory_order_acquire);
				}
			}
		}

		optional<T> pop() {
			size_t cur_tail = tail.load(memory_order_relaxed);
			while (true) {
				size_t idx = cur_tail % n;
				size_t cur_seq = seq[idx].load(memory_order_acquire);

				if (cur_seq == cur_tail + 1) {
					if (tail.compare_exchange_weak(cur_tail, cur_tail + 1, memory_order_relaxed, memory_order_relaxed)) {
						T item = buff[idx];
						seq[idx].store(cur_tail + n, memory_order_release);
						return item;
					}
				} else if (cur_seq < cur_tail + 1) {
					return {};
				} else {
					cur_tail = tail.load(memory_order_relaxed);
				}
			}
		}
};
