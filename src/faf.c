#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>

#include "khash.h"
#include "kvec.h"

#define BUF_LEN			128
#define MAX_V			(1 << 21) /* ~2M vertices */
#define EDGES_PER_EPAGE	128


typedef	uint32_t vid_t;

/* TODO: Consider using a vector instead of pages */
typedef struct e_page
{
	uint32_t num_e;
	struct e_page *next;

	/* vertex ids correspoding to each outgoing edge. */
	vid_t vid[EDGES_PER_EPAGE];
} e_page_t;

typedef struct graph
{
	e_page_t **v; /* Each vertex points to a page of edges. */
	uint32_t num_v; /* XXX Do I need this ? If yes, update when changed. */
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

/* Allocate space for an e_page_t object and add an initial vertex to it. */
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
	}

	if (g->v[v1] == NULL) {
		/* v1 has no outgoing edges, add a new page. */
		g->v[v1] = create_page_with_vertex(v2);
		return;
	}

	/* 
	 * XXX Does it make any sense to prepend the list with empty pages when
	 * needed instead of appending them ?
	 */

	e_page_t *p = g->v[v1];
	while (p->num_e == EDGES_PER_EPAGE && p->next != NULL)
		p = p->next;
	/* 
	 * At this point, we 've either reached the last page and all pages are
	 * full, or we 're somewhere in between the chain and found an empty slot.
	 */
	if (p->num_e == EDGES_PER_EPAGE) {
		p->next = create_page_with_vertex(v2);
		return;
	}

	p->vid[p->num_e++] = v2;
	return;
}

void graph_del_edge(graph_t *g, vid_t v1, vid_t v2)
{
	/* 
	 * Current implementation allows pages with 0 edges on them. We don't
	 * free() empty pages neither move edges from one page to the other, yet
	 * we make sure each page is not fragmented (empty slots)
	 */
	for (e_page_t *p = g->v[v1]; p != NULL; p = p->next) {
		for (uint32_t i = 0; i < p->num_e; i++) {
			if (p->vid[i] == v2) {
				/* replace the current edge with the last one */
				p->vid[i] = p->vid[p->num_e-1];
				p->num_e--;
				return;
			}
		}
	}

	return;
}

/* For testing */
uint32_t num_outgoing_edges(graph_t *g, vid_t v)
{
	uint32_t num_e = 0; /* total number of outgoing edges */
	for (e_page_t *p = g->v[v]; p != NULL; p = p->next)
		num_e += p->num_e;

	return num_e;
}

/* Hashtable: key->int, value->char (ignore this in our case */
KHASH_MAP_INIT_INT(32, char)
int shortest_path_length(graph_t *g, vid_t src, vid_t dest)
{
	/* bfs */
	if (src == dest)
		return 0;

	/* 
	 * OPT:
	 * - store pointers instead of vids (?)
	 * - use bitmap instead of hastable (?)
	 */
	int ignore, not_visited;
	kvec_t(vid_t) queue; 
	kv_init(queue);
	kv_push(vid_t, queue, src);
	uint32_t head = 0;	/* head idx */
	khiter_t k = 0;
	k = k; /* XXX why does it complain ? */
	khash_t(32) *h = kh_init(32);
	kh_put(32, h, src, &ignore); /* ignore also the return value */

	int dist = 0;
	while (head < kv_max(queue)) {
		++dist;
		int len = kv_max(queue) - head;
		for (int i = 0; i < len; i++) {
			vid_t v = kv_A(queue, head); /* dequeue */
			++head;
			/* TODO: implement an iterator over all outgoing edges */
			for (e_page_t *p = g->v[v]; p != NULL; p = p->next) {
				for (uint32_t k = 0; k < p->num_e; k++) {
					uint32_t u = p->vid[k];
					if (u == dest)
						return dist;
					/* Check if node has been visited in past */
					k = kh_get(32, h, u);
					not_visited = (k == kh_end(h));
					if (not_visited)
						kh_put(32, h, u, &ignore);
				}
			}
		}
	}

	kh_destroy(32, h);
	kv_destroy(queue);
	return -1;
}


int main(void)
{
	graph_t *g = graph_create(MAX_V);
	char buf[BUF_LEN];

	/* Exec before run: cp init-file.txt to /dev/shm */
	FILE *fp = fopen("/dev/shm/init-file.txt", "r");
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
/*
	vid_t v1, v2;
	char *p;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		switch (buf[0]) {
			case 'Q':
				v1 = strtoul(buf+2, &p, 10);
				++p;
				v2 = strtoul(p, NULL, 10);
				shortest_path_distance(g, v1, v2);	
				break;
			case 'A':
				v1 = strtoul(buf+2, &p, 10);
				++p;
				v2 = strtoul(p, NULL, 10);
				shortest_path_distance(g, v1, v2);	
				break;
			case 'D':
				v1 = strtoul(buf+2, &p, 10);
				++p;
				v2 = strtoul(p, NULL, 10);
				shortest_path_distance(g, v1, v2);	
				break;
			default:
				fprintf(stderr, "Unsupported operation. Exiting...\n");
				exit(EXIT_FAILURE);
		}
	}
*/

#ifdef DEBUG
	/* Sanity checks */
	uint32_t src[] = { 340279, 1445297, 1151721, 309345, 822018 };
	uint32_t dest[] = { 519122, 146499, 401121, 696543, 458677 };

	for (int i = 0; i < 5; i++) {
		int dist = shortest_path_length(g, src[i], dest[i]);
		assert(dist == -1);
	}

	assert(num_outgoing_edges(g, 1) == 46473);
	assert(num_outgoing_edges(g, 11) == 8);
	assert(num_outgoing_edges(g, 111) == 0);
	assert(num_outgoing_edges(g, 1111) == 0);
	assert(num_outgoing_edges(g, 11111) == 0);
	assert(num_outgoing_edges(g, 111111) == 2);
	assert(num_outgoing_edges(g, 1111111) == 1);
	assert(num_outgoing_edges(g, 7) == 323);
	assert(num_outgoing_edges(g, 77) == 14);
	assert(num_outgoing_edges(g, 777) == 0);
	assert(num_outgoing_edges(g, 999) == 82);
	assert(num_outgoing_edges(g, 9999) == 82);
	assert(num_outgoing_edges(g, 2) == 18951);
	assert(num_outgoing_edges(g, 16964) == 10042);
	assert(num_outgoing_edges(g, 44275) == 4073);
	assert(num_outgoing_edges(g, 105898) == 5365);
	assert(num_outgoing_edges(g, 112128) == 1924);
	graph_del_edge(g, 16964, 383147);
	graph_del_edge(g, 16964, 106326);
	graph_del_edge(g, 16964, 106506);
	graph_del_edge(g, 16964, 383071);
	graph_del_edge(g, 16964, 107111);
	graph_del_edge(g, 16964, 103629);
	graph_del_edge(g, 16964, 106259);
	graph_del_edge(g, 16964, 106958);
	graph_del_edge(g, 16964, 110050);
	graph_del_edge(g, 16964, 108742);
	graph_del_edge(g, 16964, 383095);
	assert(num_outgoing_edges(g, 16964) == 10031);
	fprintf(stdout, "Successfully passed all tests\n");
#endif

	return 0;
}


// NOTES:
// When adding a new page, I should maybe prepend it to the rest instead of
// appending it. This should be decided based on the additions/deletions ratio.
// http://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)Graphs.html
