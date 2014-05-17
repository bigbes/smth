#ifndef INCLUDES_FUNNELSORT_H
#define INCLUDES_FUNNELSORT_H
#include <stddef.h>
#include <string.h>
typedef int ( cmp_t)(const void *, const void *);
typedef int (* cmpptr_t)(const void *, const void *);

#define IS_EXHAUSTED (0x001)
#define IS_LEAF      (0x010)

struct Circular {
	void    *start;
	void    *end;
	void    *begin;
	size_t   size;
	size_t   elem_size;
};

void _circular_init(struct Circular *c, size_t nmemb, size_t size) {
	c->begin = (void *)calloc(nmemb, size);
	c->start = c->begin;
	assert(c->begin);
	c->end = c->begin + nmemb * size;
	c->elem_size = size;
	c->size = 0;
}

void _circular_init_leaf(struct Circular *c, void *begin, void *end, size_t size) {
	c->begin = begin;
	c->start = begin;
	c->end = end;
	c->elem_size = size;
	c->size = (end - begin)/size;
}

int _circular_full(struct Circular *c) {
	return (c->size == (c->end - c->begin)/c->elem_size ? 1 : 0);
}

int _circular_empty(struct Circular *c) {
	return (c->size ? 0 : 1);
}

size_t _circular_size(struct Circular *c) {
	return c->size;
}

int _circular_put_right(struct Circular *c, void *elem) {
	if (_circular_full(c))
		return 1;
	size_t pos = c->size * c->elem_size + (c->start - c->begin);
	pos %= (c->end - c->begin);
	memcpy(c->begin + pos, elem, c->elem_size);
	c->size++;
	return 0;
}

int _circular_put_left(struct Circular *c, void *elem) {
	if (_circular_full(c))
		return 1;
	c->start = (c->start != c->begin ? c->start : c->end) - c->elem_size;
	memcpy(c->start, elem, c->elem_size);
	c->size++;
	return 0;
}

void *_circular_get_right(struct Circular *c) {
	size_t pos = c->size * c->elem_size + (c->start - c->begin);
	pos %= (c->end - c->begin);
	return (_circular_empty(c) ? NULL : c->begin + pos);
}
void *_circular_get_left(struct Circular *c) {
	return (_circular_empty(c) ? NULL : c->start);
}

int _circular_pop_right(struct Circular *c) {
	if( _circular_empty(c))
		return 1;
	c->size--;
	return 0;
}
int _circular_pop_left(struct Circular *c) {
	if( _circular_empty(c))
		return 1;
	c->start = ((c->start != c->end - c->elem_size) ? c->start + c->elem_size : c->begin);
	c->size--;
	return 0;
}

struct Funnel {
	struct	 Funnel		**funnels;
	size_t	 funnel_allocate;
	size_t	 funnel_cnt;
	struct	 Circular 	*buffer;
	char	 type;
	struct TFunnel *tFunnel;
};

struct TFunnel {
	cmp_t *cmp;
	size_t size;
};
void funnel_invoke(struct Funnel *);
#endif /* INCLUDES_FUNNELSORT_H */
