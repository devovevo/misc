#include <atomic>
#include <cstdio>
#include <thread>

using namespace std;

constexpr size_t ALIGNMENT = 128;

template<typename T, size_t Size>
class RingBuffer {
	public:
		RingBuffer() : head(0), tail(0) {};

		bool push(T val) {
			size_t cur_head = head.load(memory_order_relaxed);
			size_t cur_tail = tail.load(memory_order_acquire);

			if (cur_head - cur_tail >= Size) return false;

			buff[cur_head % Size] = val; 
			head.store(cur_head + 1, memory_order_release);

			return true;
		};

		bool pop(T& val) {
			size_t cur_tail = tail.load(memory_order_relaxed);
			size_t cur_head = head.load(memory_order_acquire);

			if (cur_head == cur_tail) return false;

			val = buff[cur_tail % Size];
			tail.store(cur_tail + 1, memory_order_release);

			return true;
		};

	private:
		alignas(ALIGNMENT) atomic<size_t> head;
		alignas(ALIGNMENT) atomic<size_t> tail;
		T buff[Size];
};


constexpr size_t SIZE = 10;
constexpr size_t NUM_PRODUCED = 100;
constexpr size_t NUM_CONSUMED = 100;

void producer(RingBuffer<int, SIZE>& rb) {
	for (size_t i = 0; i < NUM_PRODUCED; i++) {
		while (!rb.push(i)) {}
		printf("Pushed %i to ring buffer\n", (int)i);
	}
};

void consumer(RingBuffer<int, SIZE>& rb) {
	int buff;
	for (size_t i = 0; i < NUM_CONSUMED; i++) {
		while (!rb.pop(buff)) {}
		printf("Popped %i from the ring buffer. Should be %i\n", buff, (int)i);
	}
};

int main() {
	RingBuffer<int, SIZE> rb;
	thread prod(producer, ref(rb));
	thread cons(consumer, ref(rb));

	prod.join();
	cons.join();

	return 0;
};
