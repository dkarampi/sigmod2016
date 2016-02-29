#include <stdio.h>
#include <stdlib.h>

/*
#define BUCKET_SIZE	10
#define FALSE 0
#define TRUE !FALSE

typedef struct edge_bucket
{
	int outedges[5];
} edge_bucket_t;

typedef struct graph
{
	int num_of_vetrices;
	edge_bucket_t **v;
} graph_t;


graph_t *graph_create(int n)
{
	int i;
	graph_t *g;

	if ((g = malloc(sizeof(graph_t))) == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	
	if ((g->v = malloc(n * sizeof(edge_bucket_t *))) == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < n; i++)
		g->v[i] = malloc(sizeof(edge_bucket_t));

	return g;
}

int main(void)
{
	graph_t *g = graph_create(10);

	g->v[0]->outedges[0] = 1;
	g->v[0]->outedges[1] = 2;
	g->v[0]->outedges[2] = 3;

	return 0;
}
*/

#include <stdint.h>
#include "khash.h"
#include "kvec.h"
KHASH_MAP_INIT_INT(32, char)
int main() {
	int ret, is_missing;
	khiter_t k;
	khash_t(32) *h = kh_init(32);
	k = kh_put(32, h, 5, &ret);		// insert 5
//	k = kh_get(32, h, 5);			// retrieve 5
//	is_missing = (k == kh_end(h));  // 0 if there, 1 if not there
	if (kh_get(32, h, 55))
		printf("fdsfsa\n");
	// printf("%d\n", kh_exist(h, 6));

	kh_destroy(32, h);

	printf("-----------------\n");
	/*
	kvec_t(uint32_t) queue;
	kv_init(queue);
//	printf("%d\n", kv_max(queue));
	for (uint32_t i = 0; i < 21; i++) {
		kv_push(uint32_t, queue, i);
	}
	for (uint32_t i = 0; i < 21; i++) {
		printf("%zu\n", kv_A(queue, i));
	}
*/

	return 0;
}
