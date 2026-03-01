#include <utility>
#include <vector>
#include <cstdio>

using namespace std;

template<typename T>
void merge(vector<T>& arr, size_t left, size_t mid, size_t right) {
	size_t n = mid - left + 1;
	size_t m = right - mid;
	
	vector<T> l(arr.begin() + left, arr.begin() + mid + 1);
	vector<T> r(arr.begin() + mid + 1, arr.begin() + right + 1);

	size_t i = 0;
	size_t j = 0;
	size_t k = left;

	while (i < n && j < m) {
		if (l[i] < r[j]) {
			arr[k] = l[i];
			i++;
		} else {
			arr[k] = r[j];
			j++;
		}

		k++;
	}

	while (i < n) {
		arr[k] = l[i];
		i++;
		k++;
	}

	while (j < m) {
		arr[k] = r[j];
		j++;
		k++;
	}
}

template<typename T>
void merge_buffer(vector<T>& arr, vector<T>& buff, size_t left, size_t mid, size_t right) {
	size_t n = mid - left + 1;
	size_t m = right - mid;

	copy(arr.begin() + left, arr.begin() + right + 1, buff.begin());

	size_t i = 0;
	size_t j = 0;
	size_t k = left;

	while (i < n && j < m) {
		size_t buff_i = i;
		size_t buff_j = n + j;

		if (buff[buff_i] <= buff[buff_j]) {
			arr[k] = buff[buff_i];
			i++;
		} else {
			arr[k] = buff[buff_j];
			j++;
		}

		k++;
	}


	while (i < n) {
		size_t buff_i = i;
		arr[k] = buff[buff_i];
		k++;
		i++;
	}

	while (j < m) {
		size_t buff_j = n + j;
		arr[k] = buff[buff_j];
		k++;
		j++;
	}
}

template<typename T>
void mergeSort(vector<T>& arr, vector<T>& buff, size_t left, size_t right) {
	if (left >= right) return;

	size_t mid = (left + right) / 2;
	mergeSort(arr, buff, left, mid);
	mergeSort(arr, buff, mid + 1, right);
	merge_buffer(arr, buff, left, mid, right);
}

int main() {
	constexpr size_t n = 9;
	vector<int> buff(n, 0);
	vector<int> test = {2, -1, 4, 10, 2, 4, 9, 49, 50};
	mergeSort(test, buff, 0, n - 1);

	for (size_t i = 0; i < n; i++) {
		printf("test[%zu] = %i\n", i, test[i]);
	}

	return 0;
}
