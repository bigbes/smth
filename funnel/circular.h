#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

struct Storage {
	int64_t *alloc_begin;
	int64_t *alloc_end;
	int64_t *start;
	size_t  size;
};

void _storage_init(struct Storage *b, size_t len) {
	b->alloc_begin = calloc(len, sizeof(int64_t));
	assert(b->alloc_begin);
	b->alloc_end = b->alloc_begin + len;
	b->start = b->alloc_begin;
	b->size = 0;
}

int _storage_full(struct Storage *b) {
	return (b->size == b->alloc_end - b->alloc_begin ? 1 : 0);
}

int _storage_empty(struct Storage *b) {
	return (b->size == 0 ? 1 : 0);
}

int _storage_put(struct Storage *b, uint64_t elem) {
	if (_storage_full(b))
		return -1;
	size_t pos = b->size + (b->start - b->alloc_begin);
	pos %= (b->alloc_end - b->alloc_begin);
	b->alloc_begin[pos] = elem;
	b->size++;
	return 0;
}

int _storage_put_left(struct Storage *b, uint64_t elem) {
	if (_storage_full(b))
		return -1;
	b->start = (b->start != b->alloc_begin ? b->start : b->alloc_end) - 1;
	*b->start = elem;
	b->size++;
	return 0;
}

int64_t _storage_get(struct Storage *b) {
	return (_storage_empty(b) ? -1 : *b->start);
}

int64_t _storage_get_right(struct Storage *b) {
	if (_storage_empty(b))
		return -1;
	size_t pos = (b->start - b->alloc_begin) + b->size - 1;
	pos %= (b->alloc_end - b->alloc_begin);
	return b->alloc_begin[pos];
}

int _storage_pop_left(struct Storage *b) {
	if (_storage_empty(b))
		return -1;
	b->start = (b->start != (b->alloc_end - 1) ? (b->start + 1) : b->alloc_begin);
	b->size--;
	return 0;
}

int _storage_pop_right(struct Storage *b) {
	if (_storage_empty(b))
		return -1;
	b->size--;
	return 0;
}

void print_storage(struct Storage *b) {
	printf("--------------------\n");
	size_t allocated = b->alloc_end - b->alloc_begin;
	printf("allocated: %zd\n", allocated);
	printf("used: %zd\n", b->size);
	size_t first = b->start - b->alloc_begin;
	size_t last  = (b->start - b->alloc_begin) + b->size;
	last %= allocated;
	printf("first: %zd, last: %zd\n", first, last);
	printf("--------------------\n");
	for(int i = 0; i < allocated; ++i) {
		if (_storage_empty(b) && i == first && i == last) {
			printf("[[ ]]");
		} else if (_storage_full(b) && i == first && i == last) {
			printf("]] [[");
		} else if (i == first) {
			printf("[[");
		} else if (i == last) {
			printf("]]");
		}
		printf("%"PRIi64, b->alloc_begin[i]);
		if (i != allocated - 1)
			printf(",");
	}
	printf("\n");
	printf("--------------------\n");
}

int test_storage() {
	struct Storage b;
	_storage_init(&b, 8);
	print_storage(&b);
	_storage_put(&b, 1);
	_storage_put(&b, 2);
	_storage_put_left(&b, 3);
	_storage_put(&b, 4);
	_storage_put_left(&b, 5);
	_storage_put(&b, 6);
	_storage_put_left(&b, 7);
	_storage_put(&b, 8);
	_storage_put_left(&b, 9);
	_storage_put(&b, 10);
	print_storage(&b);
	printf("%"PRIi64"\n", _storage_get_right(&b));
	printf("%"PRIi64"\n", _storage_get_right(&b));
	printf("%"PRIi64"\n", _storage_get(&b));
	printf("%"PRIi64"\n", _storage_get(&b));
	_storage_pop_right(&b);
	_storage_pop_right(&b);
	_storage_pop_right(&b);
	_storage_pop_right(&b);
	print_storage(&b);
	_storage_pop_left(&b);
	_storage_pop_left(&b);
	_storage_pop_left(&b);
	print_storage(&b);
	_storage_pop_left(&b);
	print_storage(&b);
	_storage_pop_left(&b);
	_storage_pop_right(&b);
	print_storage(&b);
	printf("%"PRIi64"\n", _storage_get_right(&b));
	printf("%"PRIi64"\n", _storage_get_right(&b));
	printf("%"PRIi64"\n", _storage_get(&b));
	printf("%"PRIi64"\n", _storage_get(&b));
	_storage_pop_right(&b);
	return 0;
}
