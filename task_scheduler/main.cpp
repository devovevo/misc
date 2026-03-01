#include <thread>
#include <queue>
#include <functional>
#include <utility>
#include <random>

using namespace std;

template<typename T, size_t n>
class WorkStealingQueue {

	public:
		WorkStealingQueue() : head(0), tail(0) {};

		bool push(const T& item) {
			size_t cur_head = head.load(memory_order_relaxed);
			size_t cur_tail = tail.load(memory_order_acquire);

			if (cur_head - cur_tail >= n) return false;
			
			buff[cur_head % n] = item;
			head.store(cur_head + 1, memory_order_release);

			return true;
		}

		optional<T> pop() {
			size_t cur_head = head.load(memory_order_relaxed);
			if (cur_head == 0) return {};

			head.store(cur_head - 1, memory_order_relaxed);
			atomic_thread_fence(memory_order_seq_cst);

			size_t cur_tail = tail.load(memory_order_acquire);
			if (cur_head <= cur_tail) {
				head.store(cur_head, memory_order_relaxed);
				return {};
			}

			T item = buff[(cur_head - 1) % n];

			if (cur_head == cur_tail + 1) {
				if (!tail.compare_exchange_strong(cur_tail, cur_tail + 1, memory_order_seq_cst, memory_order_relaxed)) {
					head.store(cur_head, memory_order_relaxed);
					return {};
				}

				head.store(cur_head, memory_order_relaxed);
			}  

			return item;
		}


		optional<T> steal() {
			size_t cur_tail = tail.load(memory_order_acquire);
			size_t cur_head = head.load(memory_order_acquire);
		
			if (cur_tail == cur_head) return {};

			T cur_item = buff[cur_tail % n];
			if (tail.compare_exchange_weak(cur_tail, cur_tail + 1, memory_order_seq_cst, memory_order_relaxed)) {
				return cur_item;
			}

			return {};
		}

	private:
		T buff[n];
		alignas(128) atomic<size_t> head;
		alignas(128) atomic<size_t> tail;
};

template<size_t n>
class TaskScheduler {
	public:
		TaskScheduler() : round_robin_idx(0) {
			for (size_t i = 0; i < n; i++) {
				pool[i] = thread([i, &queues]() {
					random_device rd;
					mt19937 gen(rd());

					uniform_int_distribution<size_t> distrib(0, n - 1);

					while (true) {
						optional<function<void()>> task = queues[i].pop();
						if (!task) {
							size_t rand_idx = distrib(gen);
							task = queues[rand_idx].steal();
						}


						if (task) {
							(*task)();
						}
					}
				});
			}
		}

		bool enqueue(function<void()> task) {
			for (size_t i = 0; i < n; i++) {
				size_t tidx = (round_robin_idx + i) % n;
				round_robin_idx++;
				
				bool succ = queues[tidx].push(task);
				if (succ) return true;
			}

			return false;
		}
		
	private:
		thread pool[n];
		WorkStealingQueue<function<void()>> queues[n];
		size_t round_robin_idx;
};
