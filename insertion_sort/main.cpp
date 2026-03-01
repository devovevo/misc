#include <utility>
#include <cstdio>
#include <vector>

using namespace std;

template<typename T>
void selection_sort(vector<T>& buff, size_t n) {
	for (size_t i = 0; i < n; i++) {
		T cur_min = buff[i];
		size_t cur_min_idx = i;

		for (size_t j = i + 1; j < n; j++) {
			if (buff[j] < cur_min) {
				cur_min = buff[j];
				cur_min_idx = j;
			}
		}

		swap(buff[i], buff[cur_min_idx]);
	}
}

int main() {
	constexpr size_t n = 10;
	vector<int> test = {1, 9, 3, 2, 10, -1, -50, 4, 4, 10};
	selection_sort(test, n);

	for (size_t i = 0; i < n; i++) {
		printf("test[%zu] = %i\n", i, test[i]);
	}

	return 0;
}
