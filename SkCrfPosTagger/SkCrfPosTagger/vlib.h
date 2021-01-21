// author: Márius Šajgalík
// customized by: Dalibor Mészáros

#ifdef _WIN32

#define FSEEK64 _fseeki64
#define FTELL64 _ftelli64

#else

#define FSEEK64 fseeko64
#define FTELL64 ftello64

#endif

// linker-options -lOpenCL
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <atomic>
#include <thread>
#include <algorithm>
#include <tuple>
#include <deque>
#include <queue>
#include <chrono>
#ifdef USE_OPENCL
#include <CL/cl.h>
#endif

#ifndef _VLIB_H
#define _VLIB_H

#define sc scanf
#define pp printf

using namespace std;
using namespace std::chrono;

// global variables
int words, vsize, max_w;
char *vocab = NULL;
float *M = NULL;
atomic<unsigned int> no_threads;
high_resolution_clock::time_point time_point1, time_point2;
float* _dummy = new float[1];
long long *match_count = NULL, *volume_count = NULL;
const long long total_match_count = 117631602601;
const long long total_page_count = 602551512;
const long long total_volume_count = 1136254;
int **cache_results;
float **cache_distances;

struct Phrase {
	int beginPosition,
		endPosition;
	char *text;
	char *tags;
	Phrase() {
		text = tags = NULL;
	}
	~Phrase() {
		if (text) free(text);
		if (tags) free(tags);
	}
};

void err(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	exit(1);
}

char *readFile(const char*filename, size_t *_size = NULL) {
	FILE *f = fopen(filename, "rb");
	if (!f) { fprintf(stderr, "cannot open file %s\n", filename); exit(1); }
#ifdef _WIN32
	long long size = FSEEK64(f, 0, SEEK_END);
#elif linux
	fseeko64(f, 0, SEEK_END);
	long long size = ftello64(f);
#endif
	long long chunk = size, ret;
	char *content = (char*)malloc(size + 1);
#ifdef _WIN32
	FSEEK64(f, 0, SEEK_SET);
#elif linux
	fseeko64(f, 0, SEEK_SET);
#endif

	for (long long i = 0;i<size;i += ret) {
		ret = fread(content + i, sizeof(char), chunk, f);
		if (ret<0) { fprintf(stderr, "Read error: %lld\n", ret); exit(1); }
		if (!ret) {
			if (!chunk) { fprintf(stderr, "Read error: %lld\n", ret); exit(1); }
			chunk /= 2;
		}
		else if (i + chunk<size) chunk = min(chunk * 2, size - i);
	}
	content[size] = 0;
	fclose(f);
	if (_size) *_size = size;
	return content;
}

void read_vectors(const char *filename = "corpus.bin", int *_words = &words, int *_size = &vsize, int *_max_w = &max_w, char **_vocab = &vocab, float **_M = &M) {
	long long ret;
	FILE *f = fopen(filename, "rb");
	if (!f) err("Cannot open file: %s\n", filename);
	if (FSEEK64(f, 0, SEEK_END)) {
		fprintf(stderr, "Cannot get file size: %s\n", filename);
	}
	size_t file_size = FTELL64(f);
	if (FSEEK64(f, 0, SEEK_SET)) {
		fprintf(stderr, "Cannot seek to offset 0\n");
		exit(1);
	}
	clock_t last = clock();
	fprintf(stderr, "Reading file %s...\r", filename);
	if ((ret = fread(_words, sizeof(int), 1, f))<1) err("Read error: %d\n", ret);
	if ((ret = fread(_size, sizeof(int), 1, f))<1) err("Read error: %d\n", ret);
	if ((ret = fread(_max_w, sizeof(int), 1, f))<1) err("Read error: %d\n", ret);
	long long total = *_words*(long long)*_size, chunk = total;
	*_M = (float*)malloc(total*sizeof(float));
	if (!_M) {
		fprintf(stderr, "Failed to allocate %.2f MB\n", total / 1048576.);
		exit(1);
	}
	for (long long i = 0;i<total;i += ret) {
		ret = fread(*_M + i, sizeof(float), chunk, f);
		if (ret<0) err("Read error: %d\n", ret);
		if (!ret) {
			if (!chunk) err("Read error: %d\n", ret);
			chunk /= 2;
		}
		else if (i + chunk<total) chunk = min(chunk * 2, total - i);
		if (clock() - last>CLOCKS_PER_SEC*.5) {
			size_t pos = FTELL64(f);
			fprintf(stderr, "Reading file %s...%.2f%%\r", filename, pos*100. / file_size);
			last = clock();
		}
	}
	*_vocab = (char*)malloc(*_words * *_max_w *sizeof(char));
	for (int i = 0;i<*_words;++i) {
		fscanf(f, "%s\n", *_vocab + i * *_max_w);
		if (i % 100000 == 0 && clock() - last>CLOCKS_PER_SEC*.5) {
			size_t pos = FTELL64(f);
			fprintf(stderr, "Reading file %s...%.2f%%\r", filename, pos*100. / file_size);
			last = clock();
		}
	}
	fclose(f);
	fprintf(stderr, "Reading file %s...100.00%%\n", filename);
	words = *_words;
	vsize = *_size;
	max_w = *_max_w;
	vocab = *_vocab;
	M = *_M;
	cache_results = (int**)calloc(words, sizeof(int*));
	cache_distances = (float**)calloc(words, sizeof(float*));
}

void read_vocab(const char*filename = "corpus.bin", int *_words = &words, int *_size = &vsize, int *_max_w = &max_w, char **_vocab = &vocab) {
	int ret;
	FILE *f = fopen(filename, "rb");
	if (!f) err("Cannot open file: %s\n", filename);
	if ((ret = fread(_words, sizeof(int), 1, f))<1) err("Read error: %d\n", ret);
	if ((ret = fread(_size, sizeof(int), 1, f))<1) err("Read error: %d\n", ret);
	if ((ret = fread(_max_w, sizeof(int), 1, f))<1) err("Read error: %d\n", ret);
	long long floats = *_words*(long long)*_size;
	if ((ret = FSEEK64(f, 3 * sizeof(int) + floats*sizeof(float), SEEK_SET))) err("Error %d: Cannot set position in file\n", ret);
	fprintf(stderr, "Reading file %s...", filename);
	*_vocab = (char*)malloc(*_words * *_max_w *sizeof(char));
	for (int i = 0;i<*_words;++i) {
		fscanf(f, "%s\n", *_vocab + i * *_max_w);
	}
	fclose(f);
	fprintf(stderr, "ok\n");
	words = *_words;
	vsize = *_size;
	max_w = *_max_w;
	vocab = *_vocab;
	M = NULL;
}

void read_counts(const char *filename = "counts2.bin") {
	int ret;
	FILE *f = fopen(filename, "rb");
	if (!f) err("Cannot open file: %s\n", filename);
	fprintf(stderr, "Reading file %s...", filename);
	match_count = (long long*)malloc(words*sizeof(long long));
	if (!match_count) err("Failed to allocate memory for match_count\n");
	volume_count = (long long*)malloc(words*sizeof(long long));
	if (!volume_count) err("Failed to allocate memory for volume_count\n");
	for (int i = 0;i<words;++i) {
		if ((ret = fread(match_count + i, sizeof(long long), 1, f))<1) err("Read error: %d\n", ret);
	}
	for (int i = 0;i<words;++i) {
		if ((ret = fread(volume_count + i, sizeof(long long), 1, f))<1) err("Read error: %d\n", ret);
	}
	fclose(f);
	fprintf(stderr, "ok\n");
}

inline float imf(int v) {
	return log(total_match_count / (1 + match_count[v]));
}

inline float idf(int v) {
	return log(total_volume_count / (1 + volume_count[v]));
}

inline float dot(int v1, int v2) {
	float res = 0, *a = M + v1*vsize, *b = M + v2*vsize;
	for (int i = 0;i<vsize;++i) {
		res += *a++* *b++;
	}
	return res;
}

inline float dot(const int v1, float *v2) {
	float res = 0, *a = M + v1*vsize;
	for (int i = 0;i<vsize;++i) {
		res += *a++* *v2++;
	}
	return res;
}

inline float dot(const float *v1, const float *v2) {
	float res = 0;
	for (int i = 0;i<vsize;++i) {
		res += v1[i] * v2[i];
	}
	return res;
}

inline char* get_word(int id, char *s = NULL) {
	if (s) strcpy(s, vocab + id*max_w);
	else return vocab + id*max_w;
	return s;
}

inline int get_word_index(const char *s, float* &vector = _dummy) {
	int a = 0, b = words, mid, cmp;
	while (a<b) {
		mid = (a + b) / 2;
		cmp = strcmp(s, vocab + mid*max_w);
		if (!cmp) {
			if (vector != _dummy) {
				if (!vector) vector = (float*)malloc(vsize*sizeof(float));
				for (int i = 0;i<vsize;++i)
					vector[i] = M[i + mid*vsize];
			}
			return mid;
		}
		if (cmp>0) a = mid + 1;
		else b = mid;
	}
	return -1;
}

inline int get_word_indexi(const char *s, float* &vector = _dummy) {
	int a = 0, b = words, mid, cmp;
	while (a<b) {
		mid = (a + b) / 2;
		cmp = _strcmpi(s, vocab + mid*max_w);
		if (!cmp) {
			if (vector != _dummy) {
				if (!vector) vector = (float*)malloc(vsize*sizeof(float));
				for (int i = 0;i<vsize;++i)
					vector[i] = M[i + mid*vsize];
			}
			return mid;
		}
		if (cmp>0) a = mid + 1;
		else b = mid;
	}
	return -1;
}

inline const float* get_word_vector(int index, float* &vector = _dummy) {
	if (!vector || vector == _dummy) vector = (float*)malloc(vsize*sizeof(float));
	for (int i = 0;i<vsize;++i)
		vector[i] = M[i + index*vsize];
	return vector;
}

inline int get_word_vector(const char *s, float* &vector = _dummy) {
	int a = 0, b = words, mid, cmp;
	while (a<b) {
		mid = (a + b) / 2;
		cmp = strcmp(s, vocab + mid*max_w);
		if (!cmp) {
			if (vector != _dummy) {
				if (!vector) vector = (float*)malloc(vsize*sizeof(float));
				for (int i = 0;i<vsize;++i)
					vector[i] = M[i + mid*vsize];
			}
			return 1;
		}
		if (cmp>0) a = mid + 1;
		else b = mid;
	}
	return 0;
}

int get_word_vectori(const char *s, float* &vector = _dummy) {
	int a = 0, b = words, mid, cmp;
	while (a<b) {
		mid = (a + b) / 2;
		cmp = _strcmpi(s, vocab + mid*max_w);
		if (!cmp) {
			if (vector != _dummy) {
				if (!vector) vector = (float*)malloc(vsize*sizeof(float));
				for (int i = 0;i<vsize;++i)
					vector[i] = M[i + mid*vsize];
			}
			return 1;
		}
		if (cmp>0) a = mid + 1;
		else b = mid;
	}
	return 0;
}

namespace _vp_tree {
	int *indices;
	atomic<int> processed;

	struct _vp_tree_node {
		int id;
		float radius;
		_vp_tree_node *child[2];
		_vp_tree_node(int _id) : id(_id) { child[0] = child[1] = NULL; }
		~_vp_tree_node() {
			delete child[0];
			delete child[1];
		}
	} *root = NULL;

	inline float distance(const int a, const int b) {
		return 1 - dot(a, b);
	}

	inline float distance(const int a, float* b) {
		return 1 - dot(a, b);
	}

	struct cmp {
		const int item;
		cmp(const int item) :item(item) {}
		int operator()(const int a, const int b) {
			return distance(item, a)<distance(item, b);
		}
	};

	void build(_vp_tree_node** node, int a, int b) {
		if (a == b) {
			*node = NULL;
			return;
		}
		*node = new _vp_tree_node(indices[a]);
		++processed;
		if (b - a>1) {
			int x = (int)((float)rand() / RAND_MAX*(b - a - 1)) + a;
			if (x != a) swap(_vp_tree::indices[a], _vp_tree::indices[x]);
			int median = (a + 1 + b) / 2;
			nth_element(indices + a + 1, indices + median, indices + b, cmp(indices[a]));
			(*node)->radius = distance(indices[a], indices[median]);
			for (int i = a + 1;i<b;++i) {
				if (indices[a] == indices[i]) err("zle");
			}
			if (no_threads<thread::hardware_concurrency()) {
				++no_threads;
				// thread does not support passing by reference ?
				thread t1(build, (*node)->child, a + 1, median);
				thread t2(build, (*node)->child + 1, median, b);
				t1.join();
				t2.join();
				--no_threads;
			}
			else {
				build((*node)->child, a + 1, median);
				build((*node)->child + 1, median, b);
			}
		}
	}

	typedef tuple<float, int> T;

	void search(_vp_tree_node *node, int target, unsigned int k, priority_queue<T, deque<T>> &heap, float &tau) {
		if (node == NULL) return;
		float dist = distance(node->id, target);
		if (dist<tau) {
			if (heap.size() == k) heap.pop();
			pp("add %d:%s\n", node->id, get_word(node->id));
			heap.push(make_tuple(dist, node->id));
			if (heap.size() == k) tau = get<0>(heap.top());
		}
		if (node->child[0] == NULL&&node->child[1] == NULL)
			return;
		if (dist<node->radius) {
			if (dist - tau <= node->radius)
				search(node->child[0], target, k, heap, tau);
			if (dist + tau >= node->radius)
				search(node->child[1], target, k, heap, tau);
		}
		else {
			if (dist + tau >= node->radius)
				search(node->child[1], target, k, heap, tau);
			if (dist - tau <= node->radius)
				search(node->child[0], target, k, heap, tau);
		}
	}

	void k_nearest(int a, int b, int target, unsigned int k, priority_queue<T, deque<T>> *heap) {
		for (int i = a;i<b;++i) {
			heap->push(make_tuple(distance(i, target), i));
			if (heap->size()>k) heap->pop();
		}
	}

	void k_nearest_v(int a, int b, float* target, unsigned int k, priority_queue<T, deque<T>> *heap) {
		for (int i = a;i<b;++i) {
			heap->push(make_tuple(distance(i, target), i));
			if (heap->size()>k) heap->pop();
		}
	}

	void k_nearest_v_idf(int a, int b, float* target, unsigned int k, priority_queue<T, deque<T>> *heap) {
		for (int i = a;i<b;++i) {
			heap->push(make_tuple(idf(i)*distance(i, target), i));
			if (heap->size()>k) heap->pop();
		}
	}

	void k_nearest_v_imf(int a, int b, float* target, unsigned int k, priority_queue<T, deque<T>> *heap) {
		for (int i = a;i<b;++i) {
			heap->push(make_tuple(imf(i)*distance(i, target), i));
			if (heap->size()>k) heap->pop();
		}
	}
}

void build_vp_tree() {
	if (_vp_tree::root) return;
	thread t([] {
		while (_vp_tree::processed<words) {
			this_thread::sleep_for(milliseconds(100));
			fprintf(stderr, "Building vantage-point tree...%2.2f%%\r", _vp_tree::processed / (float)words * 100);
		}
	});
	fprintf(stderr, "Building vantage-point tree...\r");
	time_point1 = high_resolution_clock::now();
	_vp_tree::indices = new int[words];
	_vp_tree::processed = 0;
	for (int i = 0;i<words;++i) _vp_tree::indices[i] = i;
	build(&_vp_tree::root, 0, words);
	delete[] _vp_tree::indices;
	duration<double> time_span = duration_cast<duration<double>>(high_resolution_clock::now() - time_point1);
	t.join();
	fprintf(stderr, "Building vantage-point tree...ok (%lf seconds)\n", time_span.count());
}

void k_nearest(int target, int k, int* &results, float* &distances) {
	if (!_vp_tree::root) build_vp_tree();
	priority_queue<_vp_tree::T, deque<_vp_tree::T>> heap;
	float tau = FLT_MAX;
	if (!results) results = new int[k];
	if (!distances) distances = new float[k];
	_vp_tree::search(_vp_tree::root, target, k, heap, tau);
	int n = heap.size();
	if (n<k) memset(results + n, -1, sizeof(int)*(k - n));
	for (int i = n - 1;i >= 0;--i) {
		tie(distances[i], results[i]) = heap.top();
		heap.pop();
	}
}

void k_nearest2(int target, unsigned int k, int* &results, float* &distances) {
	priority_queue<_vp_tree::T, deque<_vp_tree::T>> heap;
	if (!results) results = new int[k];
	if (!distances) distances = new float[k];
	for (int i = 0;i<words;++i) {
		heap.push(make_tuple(_vp_tree::distance(i, target), i));
		if (heap.size()>k) heap.pop();
	}
	unsigned int n = heap.size();
	if (n<k) memset(results + n, -1, sizeof(int)*(k - n));
	for (int i = n - 1;i >= 0;--i) {
		tie(distances[i], results[i]) = heap.top();
		heap.pop();
	}
}

int k_nearest2(float* target, unsigned int k, int* &results, float* &distances, int id = -1) {
	if (id != -1 && cache_results&&cache_distances&&cache_results[id]) {
		memcpy(results, cache_results[id], sizeof(int)*k);
		memcpy(distances, cache_distances[id], sizeof(float)*k);
		return k;
	}
	priority_queue<_vp_tree::T, deque<_vp_tree::T>> heap;
	if (!results) results = new int[k];
	if (!distances) distances = new float[k];
	for (int i = 0;i<words;++i) {
		heap.push(make_tuple(_vp_tree::distance(i, target), i));
		if (heap.size()>k) heap.pop();
	}
	unsigned int n = heap.size();
	if (n<k) memset(results + n, -1, sizeof(int)*(k - n));
	for (int i = n - 1;i >= 0;--i) {
		tie(distances[i], results[i]) = heap.top();
		heap.pop();
	}
	if (id != -1 && cache_results&&cache_distances) {
		cache_results[id] = new int[k];
		cache_distances[id] = new float[k];
		memcpy(cache_results[id], results, sizeof(int)*k);
		memcpy(cache_distances[id], distances, sizeof(float)*k);
	}
	return n;
}

int k_nearest2f(float* target, unsigned int k, int* &results, float* &distances, float *metric, int id = -1) {
	if (id != -1 && cache_results&&cache_distances&&cache_results[id]) {
		memcpy(results, cache_results[id], sizeof(int)*k);
		memcpy(distances, cache_distances[id], sizeof(float)*k);
		return k;
	}
	priority_queue<_vp_tree::T, deque<_vp_tree::T>> heap;
	if (!results) results = new int[k];
	if (!distances) distances = new float[k];
	for (int i = 0;i<words;++i) {
		heap.push(make_tuple(_vp_tree::distance(i, target)*(1 - metric[i]), i));
		if (heap.size()>k) heap.pop();
	}
	unsigned int n = heap.size();
	if (n<k) memset(results + n, -1, sizeof(int)*(k - n));
	for (int i = n - 1;i >= 0;--i) {
		tie(distances[i], results[i]) = heap.top();
		heap.pop();
	}
	if (id != -1 && cache_results&&cache_distances) {
		cache_results[id] = new int[k];
		cache_distances[id] = new float[k];
		memcpy(cache_results[id], results, sizeof(int)*k);
		memcpy(cache_distances[id], distances, sizeof(float)*k);
	}
	return n;
}

int k_nearest3(float* target, int k, int* &results, float* &distances, int id = -1) {
	//if (!results) results=new int[k];
	//if (!distances) distances=new float[k];
	if (id != -1 && cache_results&&cache_distances&&cache_results[id]) {
		memcpy(results, cache_results[id], sizeof(int)*k);
		memcpy(distances, cache_distances[id], sizeof(float)*k);
		return k;
	}
	int tc = thread::hardware_concurrency() - no_threads;
	if (!tc) {
		fprintf(stderr, "Thread deficit!!!\n");
		tc = 1;
	}
	using namespace _vp_tree;
	vector<priority_queue<T, deque<T>>> heap(tc);
	vector<thread> t(tc);
	for (int i = 0, a = 0, b;i<tc;++i, a = b) {
		if (i + 1 == tc) b = words;
		else b = words / tc*(i + 1);
		t[i] = thread(k_nearest_v, a, b, target, k, &heap[i]);
	}
	for (int i = 0;i<tc;++i) {
		t[i].join();
	}
	priority_queue<T, deque<T>, greater<T>> q;
	for (int i = 0;i<tc;++i) {
		for (int j = 0;j<k&&heap[i].size();++j) {
			q.push(heap[i].top());
			heap[i].pop();
		}
	}
	int n;
	for (n = 0;n<k&&q.size();++n) {
		tie(distances[n], results[n]) = q.top();
		q.pop();
	}
	if (n<k) memset(results + n, -1, sizeof(int)*(k - n));
	t.clear();
	heap.clear();
	if (id != -1 && cache_results&&cache_distances) {
		cache_results[id] = new int[k];
		cache_distances[id] = new float[k];
		memcpy(cache_results[id], results, sizeof(int)*k);
		memcpy(cache_distances[id], distances, sizeof(float)*k);
	}
	return n;
}

int k_nearest3_idf(float* target, int k, int* &results, float* &distances) {
	if (!results) results = new int[k];
	if (!distances) distances = new float[k];
	int tc = thread::hardware_concurrency() - no_threads;
	using namespace _vp_tree;
	priority_queue<T, deque<T>, greater<T>> q;
	vector<priority_queue<T, deque<T>>> heap(tc);
	vector<thread> t(tc);
	for (int i = 0, a = 0, b;i<tc;++i, a = b) {
		if (i + 1 == tc) b = words;
		else b = words / tc*(i + 1);
		t[i] = thread(k_nearest_v_idf, a, b, target, k, &heap[i]);
	}
	for (int i = 0;i<tc;++i) {
		t[i].join();
		for (int j = 0;j<k&&heap[i].size();++j) {
			q.push(heap[i].top());
			heap[i].pop();
		}
	}
	int n;
	for (n = 0;n<k&&q.size();++n) {
		tie(distances[n], results[n]) = q.top();
		q.pop();
	}
	if (n<k) memset(results + n, -1, sizeof(int)*(k - n));
	t.clear();
	heap.clear();
	return n;
}

int k_nearest3_imf(float* target, int k, int* &results, float* &distances) {
	if (!results) results = (int*)malloc(k*sizeof(int));
	if (!distances) distances = (float*)malloc(k*sizeof(float));
	int tc = thread::hardware_concurrency() - no_threads;
	using namespace _vp_tree;
	priority_queue<T, deque<T>, greater<T>> q;
	vector<priority_queue<T, deque<T>>> heap(tc);
	vector<thread> t(tc);
	for (int i = 0, a = 0, b;i<tc;++i, a = b) {
		if (i + 1 == tc) b = words;
		else b = words / tc*(i + 1);
		t[i] = thread(k_nearest_v_imf, a, b, target, k, &heap[i]);
	}
	for (int i = 0;i<tc;++i) {
		t[i].join();
		for (int j = 0;j<k&&heap[i].size();++j) {
			q.push(heap[i].top());
			heap[i].pop();
		}
	}
	int n;
	for (n = 0;n<k&&q.size();++n) {
		tie(distances[n], results[n]) = q.top();
		q.pop();
	}
	if (n<k) memset(results + n, -1, sizeof(int)*(k - n));
	t.clear();
	heap.clear();
	return n;
}

void k_nearest(const char *target, int k, int* &results, float* &distances) {
	k_nearest(get_word_index(target), k, results, distances);
}

void k_nearest2(const char *target, int k, int* &results, float* &distances) {
	k_nearest2(get_word_index(target), k, results, distances);
}

#ifdef USE_OPENCL
namespace opencl {
	cl_platform_id platform;
	cl_device_id gpu;
	cl_context context;
	cl_command_queue commandQueue;
	cl_kernel kernel;
	cl_mem k_nearest_output;
	cl_mem k_nearest_heap;
	struct pair_dist_id *k_nearest_top;
	int k_nearest_last_k = -1;
	int _cl_init_ok;

	void _cl_init() {
		if (_cl_init_ok) return;
		_cl_init_ok = 1;
		if (clGetPlatformIDs(1, &platform, NULL) != CL_SUCCESS) {
			fprintf(stderr, "Cannot get OpenCL platform\n");
			exit(1);
		}
		if (clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &gpu, 0) != CL_SUCCESS) {
			fprintf(stderr, "Cannot find GPU\n");
			exit(1);
		}
		context = clCreateContext(0, 1, &gpu, 0, 0, 0);
		commandQueue = clCreateCommandQueue(context, gpu, 0, 0);
		size_t _size;
		const char *source = readFile("vlib.cl", &_size);
		cl_program program = clCreateProgramWithSource(context, 1, &source, &_size, 0);
		cl_int err = clBuildProgram(program, 1, &gpu, 0, 0, 0);
		size_t log_size;
		clGetProgramBuildInfo(program, gpu, CL_PROGRAM_BUILD_LOG, 0, 0, &log_size);
		char *log = (char*)malloc(log_size);
		clGetProgramBuildInfo(program, gpu, CL_PROGRAM_BUILD_LOG, log_size, log, 0);
		if (err != CL_SUCCESS) {
			fprintf(stderr, "Cannot compile OpenCL program.\n");
			puts(log);
			exit(1);
		}
		free(log);
		kernel = clCreateKernel(program, "k_nearest", 0);
		cl_mem cl_M = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, words*size*sizeof(float), (void*)M, 0);
		if (!cl_M) {
			fprintf(stderr, "Memory allocation failed.\n");
			exit(1);
		}
		//printf("M needs %lld MB\n",words*size*sizeof(float)/1048576);
		clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&cl_M);
		clSetKernelArg(kernel, 1, sizeof(int), (void*)&words);
		clSetKernelArg(kernel, 2, sizeof(int), (void*)&size);
	}

	// must be the same as in the OpenCL program
	struct pair_dist_id {
		float dist;
		int id;
	};

	// must be the same as in the OpenCL program
	int pair_dist_id_cmp(const struct pair_dist_id &a, const struct pair_dist_id &b) {
		if (a.dist != b.dist) return a.dist<b.dist;
		return a.id<b.id;
	}
};

int k_nearest4(float*target, int k, int* &results, float* &distances) {
	if (!results) results = new int[k];
	if (!distances) distances = new float[k];
	size_t tc = max(1, (int)sqrt(words));
	using namespace opencl;
	if (k_nearest_last_k != k) {
		if (k_nearest_last_k == -1) {
			fprintf(stderr, "Using %llu GPU threads\n", tc);
			_cl_init();
		}
		else {
			clReleaseMemObject(k_nearest_output);
			clReleaseMemObject(k_nearest_heap);
			free(k_nearest_top);
		}
		k_nearest_last_k = k;
		k_nearest_top = (struct pair_dist_id*)malloc((k + 1)*tc*sizeof(struct pair_dist_id));
		k_nearest_heap = clCreateBuffer(context, 0, (k + 1)*tc*sizeof(struct pair_dist_id), 0, 0);
		clSetKernelArg(kernel, 3, sizeof(int), (void*)&k);
		clSetKernelArg(kernel, 4, sizeof(cl_mem), (void*)&k_nearest_heap);
	}
	cl_mem _target = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, size*sizeof(float), (void*)target, 0);
	if (!_target) {
		fprintf(stderr, "Memory allocation failed.\n");
		exit(1);
	}
	clSetKernelArg(kernel, 5, sizeof(cl_mem), (void*)&_target);
	cl_int ret = clEnqueueNDRangeKernel(commandQueue, kernel, 1, 0, &tc, 0, 0, 0, 0);
	if (ret != CL_SUCCESS) {
		fprintf(stderr, "Program failed: exit code %d.\n", ret);
		exit(1);
	}
	//clEnqueueReadBuffer(commandQueue,k_nearest_heap,CL_TRUE,0,(k+1)*tc*sizeof(struct pair_dist_id),k_nearest_top,0,0,0);
	//sort(k_nearest_top,k_nearest_top+(k+1)*tc,pair_dist_id_cmp); // could use binomial heaps
	//for(int i=0;i<k;++i){
	//results[i]=k_nearest_top[i].id;
	//distances[i]=k_nearest_top[i].dist;
	//}
	return k;
}
#endif

int get_phrase_vector(char *s, float* &vector = _dummy) {
	int ok = 0, len = strlen(s), a, b, c;
	float *tmp, *score, **v;
	tmp = (float*)malloc(sizeof(float)*vsize);
	score = (float*)calloc(len + 1, sizeof(float));
	v = (float**)calloc(len + 1, sizeof(float*));
	v[0] = (float*)calloc(vsize, sizeof(float));
	for (a = 0;a<len;++a) {
		if (!s[a]) s[a] = ' ';
		if (!v[a]) continue;
		for (b = a;b<len;++b) {
			if (s[b] == ' ') s[b] = '_';
		}
		s[b] = '_';
		ok = 0;
		for (;b>a;--b) {
			if (s[b] == '_') {
				s[b] = 0;
				if (b == len) --b;
				ok = get_word_vector(s + a, tmp);
				//if (!ok) ok=get_word_vectori(s+a,tmp);
				if (ok) {
					float _score;
					if (!score[a]) _score = 0;
					else _score = dot(v[a], tmp);
					if (!v[b + 1] || score[a] + _score <= score[b + 1]) {
						if (!v[b + 1]) v[b + 1] = (float*)malloc(sizeof(float)*vsize);
						score[b + 1] = score[a] + _score;
						for (c = 0;c<vsize;++c) v[b + 1][c] = v[a][c] + tmp[c];
					}
				}
			}
		}
		if (!ok) {
			for (b = a;b<len;++b) {
				if (s[b] == ' ') break;
			}
			if (b<len) ++b;
			if (v[b]) continue;
			score[b] = score[a] + 1e6;
			v[b] = (float*)malloc(sizeof(float)*vsize);
			memcpy(v[b], v[a], sizeof(float)*vsize);
		}
	}
	if (ok) {
		if (vector != _dummy) {
			if (!vector) vector = new float[vsize];
			memcpy(vector, v[len], sizeof(float)*vsize);
		}
	}
	else {
		if (!strchr(s, ' ')) {
			//fprintf(stderr,"Cannot get the sense of \"%s\"\n",s);
		}
		//exit(1);
	}
	for (a = 0;a<len;++a) {
		if (v[a]) free(v[a]);
	}
	free(v);
	free(tmp);
	free(score);
	return ok;
}

int get_phrase_vector2(char *s, float* &vector = _dummy) {
	int ok = 0, len = strlen(s);
	float *tmp = new float[vsize];
	int *score = new int[len + 1];
	memset(score, -1, (len + 1)*sizeof(int));
	float *v = new float[(len + 1)*vsize];
	memset(v, 0, vsize*sizeof(float));
	score[0] = 0;
	for (int a = 0, b;a<len;++a) {
		if (!s[a]) s[a] = ' ';
		if (score[a] == -1) continue;
		for (b = a;b<len;++b) {
			if (!s[b] || s[b] == ' ') s[b] = '_';
		}
		s[b] = '_';
		for (;b>a;--b) {
			if (s[b] == '_') {
				s[b] = 0;
				if (b == len) --b;
				ok = get_word_vector(s + a, tmp);
				if (ok) {
					if (score[b + 1] == -1 || score[a] + 1<score[b + 1]) {
						score[b + 1] = score[a] + 1;
						for (int c = 0;c<vsize;++c) v[c + (b + 1)*vsize] = v[a*vsize + c] + tmp[c];
					}
				}
			}
		}
	}
	if (score[len] != -1) {
		if (vector != _dummy) {
			if (!vector) vector = new float[vsize];
			memcpy(vector, v + len*vsize, vsize*sizeof(float));
		}
	}
	else {
		//if (!strchr(s,' ')){
		//fprintf(stderr,"Cannot get the sense of \"%s\"\n",s);
		//}
		//exit(1);
	}
	delete[] v;
	delete[] score;
	delete[] tmp;
	return ok;
}

int get_phrase_vector3(char *s, float* &vector = _dummy) {
	int ok = 0, len = strlen(s);
	float *tmp = new float[vsize];
	int *score = new int[len + 1];
	memset(score, -1, (len + 1)*sizeof(int));
	float *v = new float[(len + 1)*vsize];
	for (int i = 0;i<vsize;++i) v[i] = 1;
	score[0] = 0;
	for (int a = 0, b;a<len;++a) {
		if (!s[a]) s[a] = ' ';
		if (score[a] == -1) continue;
		for (b = a;b<len;++b) {
			if (!s[b] || s[b] == ' ') s[b] = '_';
		}
		s[b] = '_';
		for (;b>a;--b) {
			if (s[b] == '_') {
				s[b] = 0;
				if (b == len) --b;
				ok = get_word_vector(s + a, tmp);
				if (ok) {
					if (score[b + 1] == -1 || score[a] + 1<score[b + 1]) {
						score[b + 1] = score[a] + 1;
						float sum = 0;
						for (int c = 0;c<vsize;++c) {
							float x = v[c + (b + 1)*vsize] = v[a*vsize + c] * tmp[c];
							sum += x*x;
						}
						sum = sqrt(sum);
						for (int c = 0;c<vsize;++c) {
							v[c + (b + 1)*vsize] /= sum;
						}
					}
				}
			}
		}
	}
	if (score[len] != -1) {
		if (vector != _dummy) {
			if (!vector) vector = new float[vsize];
			memcpy(vector, v + len*vsize, vsize*sizeof(float));
		}
	}
	else {
		//if (!strchr(s,' ')){
		//fprintf(stderr,"Cannot get the sense of \"%s\"\n",s);
		//}
		//exit(1);
	}
	delete[] v;
	delete[] score;
	delete[] tmp;
	return ok;
}

int k_nearest3(char *target, int k, int* &results, float* &distances) {
	float* v = NULL;
	if (get_phrase_vector(target, v)) {
		int ret = k_nearest3(v, k, results, distances);
		free(v);
		return ret;
	}
	return 0;
}

#endif
