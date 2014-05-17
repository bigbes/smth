#ifndef INCLUDES_FUNNELSORT_H
#define INCLUDES_FUNNELSORT_H
#include <stddef.h>

#include "circular2.h"

typedef int ( cmp_t)(const void *, const void *);
typedef int (* cmpptr_t)(const void *, const void *);

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
			void *buffer_begin;
			void *buffer_end;
		} buf;
	};
	char     exhausted;
	int	 type;
	struct TFunnel *tFunnel;
};

struct TFunnel {
	cmp_t *cmp;
	size_t size;
};

typedef void (cb_t)(struct Funnel *, void *);
typedef void (* cbptr_t)(struct Funnel *, void *);

void funnel_aiterate(struct Funnel *, cb_t *, void *);
void funnel_biterate(struct Funnel *, cb_t *, void *);

void funnel_counter(struct Funnel *, void *);
void funnel_printer(struct Funnel *, void *);

void *funnel_get(struct Funnel *f);
void *storage_get(struct Funnel *f);
void *buffer_get(struct Funnel *f);

void funnel_pop(struct Funnel *f);
void storage_pop(struct Funnel *f);
void buffer_pop(struct Funnel *f);
#endif /* INCLUDES_FUNNELSORT_H */
