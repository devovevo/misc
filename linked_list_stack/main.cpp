#include <optional>
#include <atomic>

using namespace std;

template<typename T>
class Stack {
	struct Node {
		T data;
		Node* next;
	};

	struct TaggedHead {
		Node* head;
		size_t seq;
	};

	private:
		atomic<TaggedHead> head;
		atomic<TaggedHead> free_list_head;

	public:
		Stack() : head({nullptr, 0}), free_list_head({nullptr, 0}) {}

		void push(T item) {
			Node* n = new Node(item, nullptr);
			TaggedHead cur_head = head.load(memory_order_relaxed);
			while (true) {
				n->next = cur_head.head;
				TaggedHead next_head = {n, cur_head.seq + 1};
				if (head.compare_exchange_weak(cur_head, next_head, memory_order_release, memory_order_relaxed)) return;
			}
		}

		optional<T> pop() {
			TaggedHead cur_head = head.load(memory_order_relaxed);
			while (true) {
				Node* n = cur_head.head;
				if (n == nullptr) return {};

				Node* next = n->next;
				TaggedHead next_head = {next, cur_head.seq + 1};
				if (head.compare_exchange_weak(cur_head, next_head, memory_order_acquire, memory_order_relaxed)) return n->data;
			}
		}
};
