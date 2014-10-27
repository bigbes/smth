#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <inttypes.h>

#include "funnelsort.h"

#define LEAF_SIZE 64
#define MIXED_BUF_SIZE 128

void funnel_init_non_leaf(
		struct Funnel *f,
		struct TFunnel *tFunnel,
		size_t nfun)
{
	f->type = 0;
	f->tFunnel = tFunnel;
	f->funnels = calloc(nfun, sizeof(struct Funnel *));
	f->buffer = (struct Circular *) malloc(sizeof(struct Circular));
	f->funnel_allocate = nfun;
	f->funnel_cnt = 0;
}

void funnel_init_leaf(
		struct Funnel *f,
		void *buffer,
		size_t size,
		struct TFunnel *tFunnel) {
	if (size > LEAF_SIZE) {
		funnel_init_non_leaf(f, tFunnel, 1);
		f->buffer = NULL;
		SET_MIXED_LEAF(f);
		funnel_add_child(f, funnel_create_binary_top(buffer, size, tFunnel, MIXED_BUF_SIZE));
	} else {
		f->tFunnel = tFunnel;
		f->buffer = (struct Circular *) malloc(sizeof(struct Circular));
		SET_LEAF(f);
		qsort(buffer, size, f->tFunnel->size, f->tFunnel->cmp);
		_circular_init_leaf(f->buffer, buffer, buffer + size * tFunnel->size, tFunnel->size);
	}
}

void funnel_add_child(struct Funnel *f, struct Funnel *child) {
	assert(!CHECK_LEAF(f));
	if (f->funnel_cnt == f->funnel_allocate) {
		f->funnel_allocate *= 2;
		f->funnels = realloc(f->funnels, sizeof(struct Funnel *) * f->funnel_allocate);
		assert(f->funnels);
	}
	f->funnels[f->funnel_cnt++] = child;
}

size_t funnel_get_buf_size(size_t lvl, size_t height, size_t top_flag) {
	if (lvl == height && top_flag) {
		return top_flag;
	}
	if (height < 3) {
		if (lvl == 3)
			return 8;
		if (lvl == 2)
			return 5;
		if (lvl == 1)
			return 2;
	}
	size_t D_i = ceil(height/2);
	if (lvl == D_i)
		return ceil(pow(pow(2, height), 0.5));
	else if (lvl > D_i)
		return funnel_get_buf_size(lvl - D_i, height - D_i, top_flag);
	else if (lvl < D_i)
		return funnel_get_buf_size(lvl, height - D_i, top_flag);
}

void funnel_init_buffers(struct Funnel *f, size_t lvl, size_t height, size_t top_flag) {
	_circular_init(f->buffer, funnel_get_buf_size(lvl, height, top_flag), f->tFunnel->size);
}

void *funnel_get(struct Funnel *f) {
	if (CHECK_MIXED_LEAF(f))
		return funnel_get(f->funnels[0]);
	if (_circular_empty(f->buffer)) {
		if (CHECK_EXHAUSTED(f))
			return NULL;
		if (!CHECK_LEAF(f))
			funnel_invoke(f);
	}
	return _circular_get_left(f->buffer);
}

void funnel_pop(struct Funnel *f) {
	if (CHECK_MIXED_LEAF(f))
		return funnel_pop(f->funnels[0]);
	assert(!_circular_empty(f->buffer));
	_circular_pop_left(f->buffer);
}
void funnel_invoke(struct Funnel *f) {
	assert(!CHECK_LEAF(f));
	while(!_circular_full(f->buffer) && !CHECK_EXHAUSTED(f)) {
		void *min = funnel_get(f->funnels[0]);
		size_t funnel = 0;
		for (int i = 1; i < f->funnel_cnt; ++i) {
			void *elem = funnel_get(f->funnels[i]);
			if (!min || ((elem && min) && !(f->tFunnel->cmp)(elem, min))) {
				min = elem;
				funnel = i;
			}
		}
		if (!min) {
			SET_EXHAUSTED(f);
			continue;
		}
		_circular_put_right(f->buffer, min);
		funnel_pop(f->funnels[funnel]);
	}
}

struct Funnel *funnel_create_binary(struct Funnel *parent,
				    void  *buffer,
				    size_t buf_size,
				    size_t leafs,
				    size_t tree_height) {
	struct Funnel *this = (struct Funnel *)malloc(sizeof(struct Funnel));
	assert(this);
	if (leafs == 1) {
		funnel_init_leaf(this, buffer, buf_size, parent->tFunnel);
		return this;
	} else {
		funnel_init_non_leaf(this, parent->tFunnel, 2);
		funnel_add_child(this, funnel_create_binary(this,
							buffer,
							buf_size/2,
							leafs/2,
							tree_height));
		funnel_add_child(this, funnel_create_binary(this,
							buffer + (buf_size/2 * parent->tFunnel->size),
							buf_size - buf_size/2,
							leafs/2,
							tree_height));
		size_t lvl = log2((double )leafs);
		funnel_init_buffers(this, lvl, tree_height, 0);
	}
	return this;
}

struct Funnel *funnel_create_binary_top(void  *buffer,
					size_t buf_size,
					struct TFunnel *tFunnel,
					size_t top_flag) {
	size_t leafs = (size_t )pow((double )2, ceil(log2(pow((double )buf_size,
							      (double )1/3))));
	assert(leafs > 2);
	struct Funnel *this = (struct Funnel *)malloc(sizeof(struct Funnel));
	assert(this);
	funnel_init_non_leaf(this, tFunnel, 2);
	size_t tree_height = log2((double )leafs);
	funnel_add_child(this, funnel_create_binary(this,
						buffer,
						buf_size/2,
						leafs/2,
						tree_height));
	funnel_add_child(this, funnel_create_binary(this,
						buffer + (buf_size/2 * tFunnel->size),
						buf_size - buf_size/2,
						leafs/2,
						tree_height));
	size_t lvl = log2((double )leafs);
	funnel_init_buffers(this, lvl, lvl, (top_flag ? top_flag : buf_size));
	return this;
}

int compare_int64(const void *p1, const void *p2) {
	return (*(const int64_t *)p1 > *(const int64_t *)p2);
}

int main() {
	size_t size = 1200000;
	int64_t *a = calloc(size, sizeof(int64_t));
	assert(a);
	for (int i = 0; i < size; ++i)
		a[i] = (int64_t )rand();
	struct TFunnel tFunnel = {compare_int64, sizeof(int64_t)};
	struct Funnel *f = funnel_create_binary_top(a, size, &tFunnel, 0);
	assert(f);
	funnel_invoke(f);
	for(size_t i = 0; i < size; ++i) {
		printf("%"PRId64"\n", *(int64_t *)funnel_get(f));
		funnel_pop(f);
	}
}
