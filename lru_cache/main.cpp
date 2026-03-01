#include <unordered_map>
#include <list>
#include <atomic>
#include <utility>
#include <optional>
#include <mutex>
#include <cstdio>

using namespace std;

template<typename K, typename V>
class LRUCache {

	public:
		LRUCache(size_t capacity) : capacity(capacity) {};

		optional<V> get(K key) {
			lock_guard<mutex> lock(global_mut);

			auto it = index.find(key);
			if (it == index.end()) return {};
		
			auto list_it = it->second;
			V value = list_it->second;

			items.splice(items.begin(), items, list_it);
			return value;
		}

		void put(K key, V value) {
			lock_guard<mutex> lock(global_mut);

			auto existing_it = index.find(key);
			if (existing_it == index.end()) {
				items.push_front({key, value});
				index.insert({key, items.begin()});

				if (items.size() > capacity) {
					auto cur_back = items.back();
					size_t num_erased = index.erase(cur_back.first);
					items.pop_back();
				}
			} else {
				auto existing_list_it = existing_it->second;
				existing_list_it->second = value;

				items.splice(items.begin(), items, existing_list_it);
			}
		}

	private:
		size_t capacity;
		mutex global_mut;
		
		list<pair<K, V>> items;
		
		using ListIterator = typename list<pair<K, V>>::iterator;
		unordered_map<K, ListIterator> index;
};

int main() {
	LRUCache<int, int> cache(10);

	for (size_t i = 0; i < 10; i++) {
		cache.put(i, i);
		printf("Put (%i, %i) into map\n", (int)i, (int)i);
	}

	for (size_t i = 0; i < 10; i++) {
		optional<int> result = cache.get(i);
		if (result) {
			printf("Looked up %i and got %i, expected %i\n", (int)i, *result, (int)i);
		} else {
			printf("Looked up %i and got nothing, expected %i\n", (int)i, (int)i);
		}
	}

	cache.put(10, 10);
	optional<int> result = cache.get(0);
	if (result) {
		printf("Looked up 0, got %i, expected nothing\n", *result);
	} else {
		printf("Looked up 0, got nothing\n");
	}

	result = cache.get(9);
	if (result) {
		printf("Looked up 9, got %i, expected 9", *result);
	} else {
		printf("Looked up 9, got nothing, expected 9");
	}

	return 0;
}
