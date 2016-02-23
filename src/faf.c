#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#define FALSE 0
#define TRUE !FALSE

#define BUF_LEN		128
#define MAX_V		(1 << 20) /* ~1M vertices */
#define E_PER_EPAGE	14

typedef	uint32_t vid_t;

typedef struct e_page
{
	/* header */
	uint32_t num_e;
	struct e_page *next;

	/* payload */
	vid_t vid[E_PER_EPAGE]; /* outgoing edges */
} e_page_t;

typedef struct vertex
{
	e_page_t *edges;
} vertex_t;

// http://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)Graphs.html
typedef struct graph
{
	vertex_t *v;		/* vetrices */
	uint32_t num_v;
} graph_t;

graph_t *graph_create(const uint32_t n)
{
	graph_t *g = malloc(sizeof(g));

	if (g == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	
	g->v = malloc(n * sizeof(vertex_t));
	if (g->v == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	g->num_v = 0;

	return g;
}

void graph_destroy(graph_t *g)
{
	free(g);
	g = NULL;
}

void graph_add_edge(graph_t *g, vid_t v1, vid_t v2)
{
	e_page_t *edges = g->v[v1]->edges;

	/* Check if edge exists already. XXX Consider removing this step. */
	for (e_page_t *p = edges; p != NULL; p = p->next) {
		/* Iterate over all outgoing edges of this page */
		for (int i = 0; i < p->num_e; i++) {
			if (vid[i] == v2) {
				fprintnf(stdout, "edge %" PRIu32 +
						" --> %" PRIu32 " exists already\n", v1, v2);
				return;
			}
		}
		p = p->next;
	}

	if (edges == NULL)

	for () {

	}
	p = g->v[v1]->edges;
	while (p !=NULL && ) {
	}
	// if there is no empty spot, create a new page
}

int main(void)
{
	graph_t *g = graph_create(MAX_V);
	char buf[BUF_LEN];

	FILE *fp = fopen("init-file.txt", "r");
	if (fp == NULL) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	int cnt = 0;
    while (fgets(buf, sizeof(buf), fp) != NULL) {
		vid_t vid1, vid2;
		char *p;
		vid1 = strtoul(buf, &p, 10);
		++p;
		vid2 = strtoul(p, NULL, 10);
    }

	return 0;
}
