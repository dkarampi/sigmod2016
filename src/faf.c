#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>

#include "kvec.h"
#include "khash.h"

#define BUF_LEN			128
#define MAX_V			(1 << 21) /* ~2M vertices */



KHASH_SET_INIT_INT(m32)
typedef	uint32_t vid_t;

typedef struct graph
{
	int num_v;
	khash_t(m32) **v; /* Each vertex points to a set of (outgoing) edges. */
} graph_t;

graph_t * graph_create(const int n)
{
	graph_t *g;
	if ((g = malloc(sizeof(graph_t))) == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	if ((g->v = malloc(n * sizeof(khash_t(m32) *))) == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	for (int i = 0; i < n; i++)
		g->v[i] = kh_init(m32);
	return g;
}

void graph_destroy(graph_t *g)
{
	for (int i = 0; i < MAX_V; i++)
		kh_destroy(m32, g->v[i]);
	free(g->v);
	free(g);
}

void graph_add_edge(graph_t *g, vid_t v1, vid_t v2)
{
	khash_t(m32) *h = g->v[v1];
	if (kh_get(m32, h, v2) != kh_end(h)) /* edge exists */
		return;
	int ignore;
	kh_put(m32, h, v2, &ignore);
	g->num_v++;
}

void graph_del_edge(graph_t *g, vid_t v1, vid_t v2)
{
	khash_t(m32) *h = g->v[v1];
	khint_t k = kh_get(m32, h, v2);
	kh_del(m32, h, k);
	g->num_v--;
}

#define WORD_BITS (8 * sizeof(unsigned int))

static inline void set_bit(unsigned int *bitmap, size_t idx)
{
    bitmap[idx / WORD_BITS] |= (1 << (idx % WORD_BITS));
}

static inline bool is_bit_set(unsigned int *bitmap, size_t idx)
{
	return !!(bitmap[idx / WORD_BITS] & (1 << (idx % WORD_BITS)));
}

int shortest_path_length_bitmap(graph_t *g, graph_t *ig, vid_t src, vid_t dest)
{
	/* bfs */
	if (src == dest)
		return 0;

	/* 
	 * OPT:
	 * - store pointers instead of vids (?) ... How do I go back to vid ?
	 * - use memmove to copy the edge vectors to the queue
	 * - do not copy edge vectors, use pointers
	 */
	size_t slen, dlen;
	uint32_t shead = 0, dhead = 0; /* head idx */
	kvec_t squeue;
	kv_init(squeue);
	kv_push(vid_t, squeue, src);
	kvec_t dqueue;
	kv_init(dqueue);
	kv_push(vid_t, dqueue, dest);

	unsigned int *svisited = calloc((1 << 21) / 8 + 1, sizeof(unsigned int));
	set_bit(svisited, src);
	unsigned int *dvisited = calloc((1 << 21) / 8 + 1, sizeof(unsigned int));
	set_bit(dvisited, dest);

	/*
	 * Apply BFS both at src and dest nodes alternately.
	 * Return when a node has been visited by both expansions.
	 */
	int dist = 0;
	while ((slen = kv_size(squeue) - shead) > 0 &&
			(dlen = kv_size(dqueue) - dhead > 0)) {
		++dist;

		uint32_t *head, size;
		kvec_t *queue;
		unsigned int *cvisited = NULL; /* Nodes current exp. has visited */
		unsigned int *ovisited = NULL; /* Nodes the other exp. has visited */
		graph_t *t;
		if (slen <= dlen) { /* Expand from the source side */
			head = &shead;
			queue = &squeue;
			cvisited = svisited;
			ovisited = dvisited;
			t = g;
		} else {
			head = &dhead;
			queue = &dqueue;
			cvisited = dvisited;
			ovisited = svisited;
			t = ig;
		}
		size = kv_size(*queue);
		for (; *head < size; ++*head) {
			vid_t v = kv_A(*queue, *head); /* "dequeue" */
			khash_t(m32) *h = t->v[v];
			for (khint_t k = kh_begin(h); k != kh_end(h); ++k) {

				vid_t next = kh_key(h, k+1);
				__builtin_prefetch(cvisited+next/WORD_BITS);
				__builtin_prefetch(ovisited+next/WORD_BITS);


				if (!kh_exist(h, k))
					continue;
				vid_t u = kh_key(h, k);
				if (is_bit_set(ovisited, u)) {
					/* Node has been visited by the other expansion as well */
					free(svisited);
					free(dvisited);
					kv_destroy(squeue);
					kv_destroy(dqueue);
					return dist;
				}
				if (!is_bit_set(cvisited, u)) { /* not visited */
					set_bit(cvisited, u);
					kv_push(vid_t, *queue, u);
				}
			}
		}
		/* 
		 * This is crucial and has been the source of a nasty bug.
		 * realloc might have changed the initial pointer value. 
		 * Depending our initial branch we should reset the queues.
		 */
		if (slen <= dlen)
			squeue = *queue;
		else
			dqueue = *queue;
	}


	free(svisited);
	free(dvisited);
//	fprintf(stderr, "squeue: %p\n", (void *) &squeue);
	kv_destroy(squeue);
//	fprintf(stderr, "dqueue: %p\n", (void *) &dqueue);
	kv_destroy(dqueue);
	return -1;
}

int main(void)
{
	vid_t v1, v2;
	char *p, buf[256];
	graph_t *g = graph_create(MAX_V);
	graph_t *ig = graph_create(MAX_V); /* inverse graph */

	FILE *fp = fopen("init-file.txt", "r");

	/* Read the initial graph */
    while (fgets(buf, sizeof(buf), fp) != NULL) {
		if (buf[0] == 'S')
			break;
		v1 = strtoul(buf, &p, 10);
		++p;
		v2 = strtoul(p, NULL, 10);
		graph_add_edge(g, v1, v2);
		graph_add_edge(ig, v2, v1);
    }

	fclose(fp);

///*
//	while ((ssize_t n = read(0, buf, sizeof(buf))) > 0) {
//		;
//		// I need to maintain state between subsequent read()s
//	}
//	return 0;
//*/

//	/* Fire up the workload! */
	fprintf(stderr, "R\n");


	// fp = fopen("555Qheadworkload-file.txt", "r");
	fp = fopen("workload-file.txt.orig", "r");

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		switch (buf[0]) {
			case 'A':
				v1 = strtoul(buf+2, &p, 10);
				++p;
				v2 = strtoul(p, NULL, 10);
				graph_add_edge(g, v1, v2);
				/* OPT: avoid look up both edge vectors when v1 -> v2 exists already */
				graph_add_edge(ig, v2, v1);
				break;
			case 'D':
				v1 = strtoul(buf+2, &p, 10);
				++p;
				v2 = strtoul(p, NULL, 10);
				graph_del_edge(g, v1, v2);
				graph_del_edge(ig, v2, v1);
				break;
			case 'Q':
				v1 = strtoul(buf+2, &p, 10);
				++p;
				v2 = strtoul(p, NULL, 10);
				int dist = shortest_path_length_bitmap(g, ig, v1, v2);
//				fprintf(stdout, "%d\n", dist);
				break;
			case 'F': /* For the moment, ignore this */
				break;
			default:
				fprintf(stderr, "Unsupported operation. Exiting...\n");
				exit(EXIT_FAILURE);
		}
	}

	fclose(fp);

	graph_destroy(g);
	graph_destroy(ig);

	return 0;
}
