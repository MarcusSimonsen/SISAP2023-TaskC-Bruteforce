#include <vector>

using namespace std;

template <class T>
class NHeap {
		std::vector<T*> heap;
		size_t N;
	public:
		NHeap(size_t n) {
			N = n;
			heap.push_back(nullptr);
		}

		bool less(int i, int j) {
			return heap[i] < heap[j];
		}

		void exch(int i, int j) {
			T *tmp = heap[i];
			heap[i] = heap[j];
			heap[j] = tmp;
		}
		
		void sink(int k) {
			while (2*k <= N) {
				int j = 2*k;
				if (j < N && less(j, j+1)) j++;
				if (!less(k, j)) break;
				exch(k, j);
				k = j;
			}
		}

		void swim(int k) {
			while (k > 1 && less(k/2, k)) {
				exch(k, k/2);
				k /= 2;
			}
		}

		void poppush(T item) {
			if (heap.size() < N+2) {
				T *place = (T*)malloc(sizeof(T));
				*place = item;
				heap.push_back(place);
				swim(heap.size());
				return;
			}
			*heap[1] = item;
			sink(1);
		}

		vector<T*> get_vector() {
			return heap;
		}
};
