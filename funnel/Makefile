SORT_SIZE=1048576
export SORT_SIZE
BUFFER_SIZE=15
export BUFFER_SIZE

all:
	gcc --std=c99 funnelsort.c -lm -o funnel.out -O3
	strip funnel.out
	gcc --std=c99 qsort.c -lm -o qsort.out -O3
	strip qsort.out
timeit:
	time ./funnel.out > /dev/null
	time ./qsort.out > /dev/null
vg:
	valgrind --leak-check=full -v ./funnel.out > /dev/null 2> funnel.vg
	valgrind --leak-check=full -v ./qsort.out > /dev/null 2> qsort.vg
cg:
	valgrind --tool=cachegrind ./funnel.out > /dev/null 2> funnel.cg
	valgrind --tool=cachegrind ./qsort.out > /dev/null 2> qsort.cg
clean:
	rm -rf *.vg *.cg *.out
