#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "funnelsort.h"
#include "circular2.h"

void storage_init(struct Storage *s, size_t nmemb) {
	_storage_init(s, nmemb);
}

int compare_int64(const void *p1, const void *p2) {
	return (*(const int64_t *)p1 > *(const int64_t *)p2);
}

void funnel_init_non_leaf(
		struct Funnel *f,
		size_t k,
		struct TFunnel *tFunnel,
		size_t nfun)
{
	f->type = k;
	f->exhausted = 0;
	f->tFunnel = tFunnel;
	f->fun.funnels = calloc(nfun, sizeof(struct Funnel *));
	f->fun.funnel_allocate = nfun;
	f->fun.funnel_size = 0;
}

void funnel_init_leaf(
		struct Funnel *f,
		void *buffer,
		size_t size,
		struct TFunnel *tFunnel) {
	f->type = 0;
	f->exhausted = 0;
	f->tFunnel = tFunnel;
	f->buf.buffer_begin = buffer;
	f->buf.buffer_end = buffer + size * tFunnel->size;
	qsort(buffer, size, f->tFunnel->size, f->tFunnel->cmp);
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

size_t funnel_get_buf_size(struct Funnel *f, size_t lvl, size_t ov_lvl) {
	if (lvl == ov_lvl) {
		size_t a = 0;
		funnel_aiterate(f, funnel_counter, (void *)&a);
		return a + 1;
	}
	size_t buf_size = 0;
	if (ov_lvl == 3) {
		if (lvl == 3) {
			return 8;
		} else if (lvl == 2) {
			return 3;
		} else if (lvl == 1) {
			return 2;
		}
	} else if (ov_lvl == 2) {
		if (lvl == 2) {
			return 3;
		} if (lvl == 1) {
			return 2;
		}
	} else if (ov_lvl == 1) {
		return 2;
	}
	size_t D_i = ceil(ov_lvl/2);
	if (lvl == D_i) {
		return ceil(pow(pow(2, ov_lvl), 0.5));
	} else if (lvl > D_i) {
		return funnel_get_buf_size(f, lvl - D_i, ov_lvl - D_i);
	} else if (lvl < D_i) {
		return funnel_get_buf_size(f, lvl, ov_lvl - D_i);
	}
	return 8;
}

void funnel_init_buffers(struct Funnel *f, size_t lvl, size_t ov_lvl) {
	size_t buf_size = funnel_get_buf_size(f, lvl, ov_lvl);
	storage_init(&f->fun.storage, buf_size);
}

struct Funnel *funnel_create_binary(struct Funnel *parent,
				    void *buffer,
				    size_t buf_size,
				    size_t leafs,
				    size_t ov_lvl) {
	struct Funnel *this = (struct Funnel *)malloc(sizeof(struct Funnel));
	assert(this);
	if (leafs == 1) {
		funnel_init_leaf(this, buffer, buf_size, parent->tFunnel);
		return this;
	} else {
		funnel_init_non_leaf(this, leafs, parent->tFunnel, 2);
		funnel_add_child(this, funnel_create_binary(this,
							buffer,
							buf_size/2,
							leafs/2,
							ov_lvl));
		funnel_add_child(this, funnel_create_binary(this,
							buffer + (buf_size/2 * parent->tFunnel->size),
							buf_size - buf_size/2,
							leafs/2,
							ov_lvl));
		size_t lvl = log2((double )leafs);
		funnel_init_buffers(this, lvl, ov_lvl);
	}
	return this;
}

struct Funnel *funnel_create_binary_top(void *buffer,
					size_t buf_size,
					struct TFunnel *tFunnel) {
	size_t leafs = (size_t )pow((double )2, ceil(log2(pow((double )buf_size,
							      (double )1/3))));
	assert(leafs > 2);
	struct Funnel *this = (struct Funnel *)malloc(sizeof(struct Funnel));
	assert(this);
	funnel_init_non_leaf(this, leafs, tFunnel, 2);
	size_t ov_lvl = log2((double )leafs);
	funnel_add_child(this, funnel_create_binary(this,
						buffer,
						buf_size/2,
						leafs/2,
						ov_lvl));
	funnel_add_child(this, funnel_create_binary(this,
						buffer + (buf_size/2 * tFunnel->size),
						buf_size - buf_size/2,
						leafs/2,
						ov_lvl));
	size_t lvl = log2((double )leafs);
	funnel_init_buffers(this, lvl, lvl);
	return this;
}

void storage_fill_single(struct Funnel *f) {
	assert(f->type != 0);
	void *min = funnel_get(f->fun.funnels[0]);
	size_t funnel = 0;
	if (min)
	for (int i = 1; i < f->fun.funnel_size; ++i) {
		void *elem = funnel_get(f->fun.funnels[i]);
		if (!min || ((elem && min) && !(f->tFunnel->cmp)(elem, min))) {
			min = elem;
			funnel = i;
		}
	}
	if (!min) {
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

inline void *funnel_get(struct Funnel *f) {
	return (f->type != 0 ? storage_get(f) : buffer_get(f));
}

inline void funnel_pop(struct Funnel *f) {
	(f->type != 0 ? storage_pop(f) : buffer_pop(f));
}

void *buffer_get(struct Funnel *f) {
	assert(f->type == 0);
	if (f->buf.buffer_begin == f->buf.buffer_end)
		return NULL;
	return f->buf.buffer_begin;
}

void buffer_pop(struct Funnel *f) {
	assert(f->type == 0);
	assert(f->buf.buffer_begin != f->buf.buffer_end);
	f->buf.buffer_begin += f->tFunnel->size;
}

void *storage_get(struct Funnel *f) {
	assert(f->type != 0);
	if (_storage_empty(&f->fun.storage)) {
		if (f->exhausted)
			return NULL;
		storage_fill(f);
	}
	return _storage_get(&f->fun.storage);
}

void storage_pop(struct Funnel *f) {
	assert(f->type != 0);
	assert(!_storage_empty(&f->fun.storage));
	_storage_pop_left(&f->fun.storage);
}

void funnel_aiterate(struct Funnel *f, cbptr_t cb, void *a) {
	if (f->type != 0) {
		for (int i = 0; i < f->fun.funnel_size; ++i)
			funnel_aiterate(f->fun.funnels[i], cb, a);
	}
	cb(f, a);
}

void funnel_biterate(struct Funnel *f, cbptr_t cb, void *a) {
	cb(f, a);
	if (f->type != 0) {
		for (int i = 0; i < f->fun.funnel_size; ++i)
			funnel_biterate(f->fun.funnels[i], cb, a);
	}
}

void int64_printer(const void *i) {
	printf("%"PRIi64, *(int64_t *)i);
}

void funnel_counter(struct Funnel *f, void *size) {
	if (f->type == 0) {
		*(size_t *)size += (f->buf.buffer_end - f->buf.buffer_begin) / f->tFunnel->size;
	}
}

void funnel_printer(struct Funnel *f, void *_unused) {
	(void *)_unused;
	assert(f);
	printf("------------------------------------------------------------\n");
	printf("Funnel Type: %s, ", (f->type == 0 ? "Leaf" : "Non-Leaf"));
	printf("Funnel Number: %d, ", f->type);
	printf("Exhausted: %s\n", (f->exhausted == 0 ? "Not" : "Yes"));
	printf("--------------------------------------------------\n");
	if (f->type == 0) {
		for(int64_t *i = (int64_t *) f->buf.buffer_begin;
		    i < (int64_t *) f->buf.buffer_end;
		    ++i) {
			int64_printer(i);
			printf(" ");
		}
		printf("\n============================================================\n");
		return;
	} else {
		printf("Buffer Size = %zd\n", _storage_size(&f->fun.storage));
	}
	printf("------------------------------------------------------------\n");
}

void storage_printer(struct Funnel *f) {
	void *prev = NULL;
	void *next = NULL;
	do {
		prev = storage_get(f);
		if (!prev)
			break;
		printf("%zd, ", *(int64_t *)prev);
		storage_pop(f);
		if (next)
			assert(*(int64_t *)prev >= *(int64_t *)next);
		next = prev;
	} while (prev);
}

int main() {
	size_t size = 1024;
	int64_t *a = calloc(size, sizeof(int64_t));
	assert(a);
	for (int i = 0; i < size; ++i)
		a[i] = (int64_t )rand();
	struct TFunnel tFunnel = {compare_int64, sizeof(int64_t)};
	struct Funnel *f = funnel_create_binary_top(a, size, &tFunnel);
	assert(f);
	storage_fill(f);
	storage_printer(f);
}
