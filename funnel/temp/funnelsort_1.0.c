#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "circular.h"

#define BUF_SIZE 8

struct Funnel {
	union {
		struct {
			/* 1. In case of non-leaf funnel */
			struct   Funnel **funnels;
			size_t   funnel_allocate;
			size_t   funnel_size;

			/* 1. Storage for temporary results */
			struct   Storage storage;
		} fun;
		struct {
			/* 2. In case of leaf funnel (Initial Buffers) */
			int64_t *buffer_begin;
			int64_t *buffer_end;
		} buf;

	};
	char     exhausted;
	int	 type;
};

/*
 * N - number of elements in the array, we sorting;
 * In the beggining we split input array into K = pow(N, 1/3) buffers with size
 * pow(N, 2/3) (for last array - less).
 * Then we use quicksort for sorting each of this buffers;
 * For 1 billion elements it's 1000 buffers with 1000000 elements;
 * Then there's two kinds of Funnel Sorts:
 *
 * 1) Funnels of 2-/3-mergers. Usual, Classic Funnelsort algorithm. (Not our
 * way). We must create sqrt(K) sqrt(K)-mergers, and, we'll merge all
 * this way, but this's the hard so we'll skip this part.
 *
 * 2) Funnels consists only of 2-mergers. Lazy Funnelsort algorithm insists on
 * this structure;
 * For first we take K = pow(2, ceil(log2(pow(size, 1/3)))), not pow(N, 1/3). Depth of our
 * tree will be ceil(lg(pow(N, 1/3))).
 * In each Funnel we have buffer of size 2*K^(3/2), for 2-mergers it's 6.
*/

int64_t funnel_get(struct Funnel *f);
void funnel_pop(struct Funnel *f);
void funnel_printer(struct Funnel *f);

/* Comparing function for qsort */
int compare_int64(const void *p1, const void *p2) {
	return (*(const int64_t *)p1 > *(const int64_t *)p2);
}

/* Sort leaf buffers */
void funnel_sort_leafs(struct Funnel *f) {

	qsort(f->buf.buffer_begin, (size_t )(f->buf.buffer_end - f->buf.buffer_begin),
			sizeof(int64_t), compare_int64);
}

/* Create leaf elements */
void funnel_create_leaf(struct Funnel *f, int64_t *buffer, size_t size) {
	f->type         = 0;
	f->exhausted    = 0;
	f->buf.buffer_begin  = buffer;
	f->buf.buffer_end    = buffer + size;
//	printf("Before sort: Begin - %"PRIi64, *f->buf.buffer_begin);
//	printf(", End - %"PRIi64"\n", *(f->buf.buffer_end - 1));
	funnel_sort_leafs(f);
//	printf("After sort: Begin - %"PRIi64, *f->buf.buffer_begin);
//	printf(", End - %"PRIi64"\n", *(f->buf.buffer_end - 1));
}

/* Create non-leaf elements */
void funnel_create_non_leaf(struct Funnel *f, size_t allocate) {
	static int x = 1;
	f->type                = x++;
	f->fun.funnels         = calloc(allocate, sizeof(struct Funnel *));
	f->fun.funnel_size     = 0;
	f->fun.funnel_allocate = allocate;
	_storage_init(&f->fun.storage, (size_t )atoll(getenv("BUFFER_SIZE")));
}
void funnel_add_child(struct Funnel *f, struct Funnel *child) {
	(f->type != 0);
	if (f->fun.funnel_size == f->fun.funnel_allocate) {
		f->fun.funnel_allocate *= 2;
		f->fun.funnels = realloc(f->fun.funnels,
				sizeof(struct Funnel *) * (f->fun.funnel_allocate));
		assert(f->fun.funnels);
	}
	f->fun.funnels[f->fun.funnel_size] = child;
	f->fun.funnel_size++;
}

int64_t buffer_get(struct Funnel *f) {
	assert(f->type == 0);
	if (f->buf.buffer_begin == f->buf.buffer_end)
		return -1;
	return *f->buf.buffer_begin;
}

void buffer_pop(struct Funnel *f) {
	assert(f->type == 0);
	assert(f->buf.buffer_begin != f->buf.buffer_end);
	f->buf.buffer_begin++;
}

void storage_fill_single(struct Funnel *f) {
	assert(f->type != 0);
	int64_t min = funnel_get(f->fun.funnels[0]);
	size_t  funnel = 0;
	for (int i = 1; i < f->fun.funnel_size; ++i) {
		int64_t elem = funnel_get(f->fun.funnels[i]);
		if ((elem < min && elem != -1) || min == -1) {
			min = elem;
			funnel = i;
		}
	}
	if (min == -1) {
		f->exhausted = 1;
		return;
	}
	_storage_put(&f->fun.storage, min);
	funnel_pop(f->fun.funnels[funnel]);
}

void storage_fill(struct Funnel *f) {
	assert(f->type != 0);
	while(!_storage_full(&f->fun.storage) && !f->exhausted)
		storage_fill_single(f);
}

int64_t storage_get(struct Funnel *f) {
	assert(f->type != 0);
	if (_storage_empty(&f->fun.storage)) {
		if (f->exhausted)
			return -1;
		storage_fill(f);
	}
	return _storage_get(&f->fun.storage);
}

void storage_pop(struct Funnel *f) {
	assert(f->type != 0);
	assert(!_storage_empty(&f->fun.storage));
	_storage_pop_left(&f->fun.storage);
}

inline int64_t funnel_get(struct Funnel *f) {
	return (f->type != 0 ? storage_get(f) : buffer_get(f));
}

inline void funnel_pop(struct Funnel *f) {
	(f->type != 0 ? storage_pop(f) : buffer_pop(f));
}

struct Funnel *funnel_create(int64_t *buffer, size_t size) {
	size_t leafs = (size_t )pow((double )2, ceil(log2(pow((double )size, (double )1/3))));
	size_t size_per_leaf = ceil(size / leafs);
	struct Funnel *funnels = (struct Funnel *)calloc(2 * leafs - 1,
		sizeof(struct Funnel));
	for (int i = leafs - 1; i < 2*leafs - 1; ++i) {
		int64_t *_begin = buffer + (i - leafs + 1)*size_per_leaf;
		size_t _size  = size_per_leaf;
		if (i == 2*leafs - 1)
			_size = size - (i - leafs + 1)*size_per_leaf;

		funnel_create_leaf(funnels + i,
		        _begin,
			_size);
	}
	for (int i = leafs - 2; i >= 0; --i) {
		funnel_create_non_leaf(funnels + i, 2);
		funnel_add_child(funnels + i, funnels + i*2 + 1);
		funnel_add_child(funnels + i, funnels + i*2 + 2);
	}
//	for (int i = leafs; i < 2 * leafs - 1; ++i) {
//		funnel_printer(funnels + i);
//	}
	return funnels;
}

void storage_printer(struct Storage *s) {
	printf("--------------------------------------------------\n");
	size_t allocated = s->alloc_end - s->alloc_begin;
	size_t first = s->start - s->alloc_begin;
	size_t last  = (s->start - s->alloc_begin) + s->size;
	last %= allocated;
	printf("allocated: %zd, used: %zd, first: %zd, last: %zd\n",
			allocated, s->size, first, last);
	printf("--------------------------------------------------\n");
	for(int i = 0; i < allocated; ++i) {
		if (_storage_empty(s) && i == first && i == last) {
			printf("[[ ]]");
		} else if (_storage_full(s) && i == first && i == last) {
			printf("]] [[");
		} else if (i == first) {
			printf("[[");
		} else if (i == last) {
			printf("]]");
		}
		printf("%"PRIi64, s->alloc_begin[i]);
		if (i != allocated - 1)
			printf(",");
	}
	printf("\n--------------------------------------------------\n");
}

void funnel_iterate(struct Funnel *f, void (*cb)(struct Funnel *f)) {
	cb(f);
	if (f->type != 0) {
		for (int i = 0; i < f->fun.funnel_size; ++i)
			funnel_iterate(f->fun.funnels[i], cb);
	}
}

void funnel_printer(struct Funnel *f) {
	printf("------------------------------------------------------------\n");
	printf("Funnel Type: %s, ", (f->type == 0 ? "Leaf" : "Non-Leaf"));
	printf("Funnel Number: %d, ", f->type);
	printf("Exhausted: %s\n", (f->exhausted == 0 ? "Not" : "Yes"));
	printf("--------------------------------------------------\n");
	if (f->type == 0) {
		for(int64_t *i = f->buf.buffer_begin; i < f->buf.buffer_end; ++i)
			printf("%"PRIi64"%s",
			       *i,
			       (i != f->buf.buffer_end - 1 ? ", " : ""));
		printf("\n============================================================\n");
		return;
	} else {
		for(size_t i = 0; i < f->fun.funnel_size; ++i)
			printf("%i, ", f->fun.funnels[i]->type);
		printf("\n");
		storage_printer(&f->fun.storage);
	}
	printf("------------------------------------------------------------\n");
}

int main(int argc, char *argv[], char *arge[]) {
//	size_t size = 1048576;
	size_t size = (size_t )atoll(getenv("SORT_SIZE"));
	int64_t *a = calloc(size, sizeof(int64_t));
	assert(a);
	for (int i = 0; i < size; ++i) {
		int64_t temp = (int64_t )rand();
		if (temp < 0) temp *= -1;
		a[i] = temp + 1;
	}
	struct Funnel *f = funnel_create(a, size);
	assert(f);
	int64_t b = 0;
	int64_t b_prev = -1;
	while((b = funnel_get(f)) != -1) {
		assert (b_prev <= b);
		printf("%"PRIi64"\n", b);
		funnel_pop(f);
		b_prev = b;
	}
//	funnel_iterate(f, funnel_printer);
	return 0;
}
