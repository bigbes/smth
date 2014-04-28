#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "funnelsort.h"
#include "circular.h"

void storage_init(struct Storage *s, size_t nmemb) {
	_storage_init(s, nmemb);
}


void funnel_init_non_leaf(struct Funnel *f, size_t k, struct TFunnel *tFunnel, size_t nfun) {
	f->type = k;
	f->exhausted = 0;
	f->tFunnel = tFunnel;
	f->fun.funnels = calloc(nfun, sizeof(struct Funnel *));
	f->fun.funnel_allocate = nfun;
	f->fun.funnel_size = 0;
	size_t buffer = ceil();
	storage_init(&f->fun.storage, );
}

void funnel_init_leaf(struct Funnel *f, void *buffer, struct TFunnel *tFunnel) {
	f->type = 0;
	f->exhausted = 0;
	f->tFunnel = tFunnel;
}

struct Funnel *funnel_create_binary_top(struct Funnel *f, void *buffer, size_t buf_size, struct TFunnel *tFunnel) {
	size_t leafs = (size_t )pow((double )2, ceil(log2(pow((double )size, (double )1/3))));
	assert(leafs < 2);
	struct Funnel *this = (struct Funnel *)calloc(1, sizeof(struct Funnel));
	assert(this);
	funnel_init_non_leaf(f, leafs, tFunnel);
}
