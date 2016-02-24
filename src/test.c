#include <stdio.h>
#include <stdlib.h>

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
