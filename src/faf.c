#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#define FALSE 0
#define TRUE !FALSE

#define BUF_LEN		128
#define MAX_V		(1 << 21) /* ~2M vertices */
#define E_PER_EPAGE	14

typedef	uint32_t vid_t;

typedef struct e_page
{
	uint32_t num_e;
	struct e_page *next;

	/* vertex ids correspoding to each outgoing edge */
	vid_t vid[E_PER_EPAGE];
} e_page_t;

typedef struct graph
{
	e_page_t **v; /* each vertex points to a page of edges */
	uint32_t num_v;
} graph_t;

graph_t *graph_create(const uint32_t n)
{
	graph_t *g;
	
	if ((g = malloc(sizeof(graph_t))) == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	
	if ((g->v = malloc(n * sizeof(e_page_t *))) == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	for (uint32_t i = 0; i < n; i++)
		g->v[i] = NULL;
	g->num_v = 0;

	return g;
}

void graph_destroy(graph_t *g)
{
	for (uint32_t i = 0; i < g->num_v; i++)
		free(g->v[i]);
	free(g->v);
	free(g);
	g = NULL;
}

void graph_add_edge(graph_t *g, vid_t v1, vid_t v2)
{
	e_page_t *edges = g->v[v1];

	/* Check if edge exists already. XXX Consider removing this step. */
	for (e_page_t *p = edges; p != NULL; p = p->next) {
		/* Iterate over all outgoing edges of this page */
		for (int i = 0; i < p->num_e; i++) {
			if (p->vid[i] == v2) {
				fprintf(stdout, "edge %" PRIu32
						" --> %" PRIu32 " exists already\n", v1, v2);
				return;
			}
		}
		p = p->next;
	}

	if (edges == NULL) {
		/* v1 has no outgoing edges, add a new page */
		e_page_t *p;
		if ((p = malloc(sizeof(e_page_t))) == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		p->vid[0] = v2;
		p->num_e = 1;
		p->next = NULL;
		g->v[v1].edges = p;
		return;
	}

	e_page_t *p = NULL;
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

// http://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)Graphs.html
