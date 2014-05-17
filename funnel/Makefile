all: funnel

funnel:
	gcc --std=c99 funnelsort.c -lm -o funnel.out -g
funnel_rel:
	gcc --std=c99 funnelsort.c -lm -o funnel.out -O3
	strip funnel.out
