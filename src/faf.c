#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#define FALSE 0
#define TRUE !FALSE

#define BUF_LEN			128
#define MAX_V			(1 << 21) /* ~2M vertices */
#define EDGES_PER_EPAGE	13

typedef	uint32_t vid_t;

typedef struct e_page
{
	uint32_t num_e;
	struct e_page *next;

	/* vertex ids correspoding to each outgoing edge */
	vid_t vid[EDGES_PER_EPAGE];
} e_page_t;

typedef struct graph
{
	e_page_t **v; /* each vertex points to a page of edges */
	uint32_t num_v;
} graph_t;

graph_t * graph_create(const uint32_t n)
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

/* Allocate space for an e_page_t object and add an initial vertex to it */
e_page_t * create_page_with_vertex(vid_t v)
{
	e_page_t *p;
	if ((p = malloc(sizeof(e_page_t))) == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	p->vid[0] = v;
	p->num_e = 1;
	p->next = NULL;

	return p;
}


void graph_add_edge(graph_t *g, vid_t v1, vid_t v2)
{
	/* Check if edge exists already. XXX Consider removing this step. */
	for (e_page_t *p = g->v[v1]; p != NULL; p = p->next) {
		/* Iterate over all outgoing edges of this page. */
		for (uint32_t i = 0; i < p->num_e; i++) {
			if (p->vid[i] == v2) {
				fprintf(stdout, "edge %" PRIu32
						" --> %" PRIu32 " exists already\n", v1, v2);
				return;
			}
		}
		p = p->next;
	}

	if (g->v[v1] == NULL) {
		/* v1 has no outgoing edges, add a new page. */
		g->v[v1] = create_page_with_vertex(v2);
		return;
	}

	e_page_t *p = g->v[v1];
	while (p->num_e == EDGES_PER_EPAGE && p->next != NULL)
		p = p->next;
	/* 
	 * Reached the final page where either there is space to add
	 * one more edge, or there is no and another page must be added.
	 */
	if (p->num_e == EDGES_PER_EPAGE) {
		p->next = create_page_with_vertex(v2);
		return;
	}

	p->vid[p->num_e++] = v2;
	return;
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

    while (fgets(buf, sizeof(buf), fp) != NULL) {
		vid_t v1, v2;
		char *p;
		v1 = strtoul(buf, &p, 10);
		++p;
		v2 = strtoul(p, NULL, 10);
		graph_add_edge(g, v1, v2);
    }

	return 0;
}


// NOTES:
// When adding a new page, I should maybe prepend it to the rest instead of
// appending it. This should be decided based on the additions/deletions ratio.
// http://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)Graphs.html
