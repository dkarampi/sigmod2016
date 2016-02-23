#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define BUF_LEN		2048
#define MAX_V		(1 << 20) /* 1M nodes */

// http://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)Graphs.html
typedef struct {
	uint32_t *v;	/* vetrices */
	uint32_t num_v;
} Graph;


Graph *graph_create(const uint32_t n)
{
	Graph *g = malloc(sizeof(g));

	if (g == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	
	g->v = malloc(n * sizeof(uint32_t));
	if (g->v == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	g->num_v = 0;

	return g;
}


int main(void)
{
	Graph *g = graph_create(MAX_V);


	char buf[BUF_LEN];

	/*
    while (fgets(buf, sizeof buf, stdin) != NULL)
	{

    }
	*/

	return 0;
}
