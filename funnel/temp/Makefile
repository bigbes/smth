SORT_SIZE=67108864
export SORT_SIZE
BUFFER_SIZE=32
export BUFFER_SIZE

all: funnel
timeit: timeit_funnel timeit_omp timeit_qsort

funnel:
	gcc --std=c99 funnelsort.c -lm -o funnel.out -g
funnel_rel:
	gcc --std=c99 funnelsort.c -lm -o funnel.out -O3
	strip funnel.out

omp:
	gcc --std=c99 funnelsort_omp.c -lm -o omp.out -fopenmp -g
omp_rel:
	gcc --std=c99 funnelsort_omp.c -lm -o omp.out -fopenmp -O3
	strip funnel.out

qsort:
	gcc --std=c99 qsort.c -lm -o qsort.out -O3
	strip qsort.out


timeit_funnel:
	time ./funnel.out > /dev/null
timeit_omp:
	time ./omp.out > /dev/null
timeit_qsort:
	time ./qsort.out > /dev/null

vg:
	valgrind --leak-check=full -v ./funnel.out > /dev/null 2> funnel.vg
	valgrind --leak-check=full -v ./qsort.out > /dev/null 2> qsort.vg
cg:
	valgrind --tool=cachegrind ./funnel.out > /dev/null 2> funnel.cg
	valgrind --tool=cachegrind ./qsort.out > /dev/null 2> qsort.cg

clean:
	rm -rf *.vg *.cg *.out
