#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

/* Comparing function for qsort */
int compare_int64(const void *p1, const void *p2) {
	return (*(const int64_t *)p1 > *(const int64_t *)p2);
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
	qsort(a, size, sizeof(int64_t), compare_int64);
	return 0;
}
