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


typedef	uint32_t vid_t;

typedef struct graph
{
	int num_v;
	kvec_t(vid_t) **v; /* Each vertex points to a set of (outgoing) edges. */
} graph_t;

graph_t * graph_create(const int n)
{
	graph_t *g;
	if ((g = malloc(sizeof(graph_t))) == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	if ((g->v = malloc(n * sizeof(kvec_t(vid_t) *))) == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	for (int i = 0; i < n; i++) { /* TODO: calloc ? */
		g->v[i] = malloc(sizeof(kvec_t(vid_t)));
		kv_init(*(g->v[i])); 
	}
	return g;
}

void graph_destroy(graph_t *g)
{
	for (int i = 0; i < MAX_V; i++) {
		kv_destroy(*(g->v[i]));
		free(g->v[i]);
		g->v[i] = NULL; 
	}
	free(g);
	g = NULL;
}

void graph_add_edge(graph_t *g, vid_t v1, vid_t v2)
{
	kvec_t(vid_t) *vec = (void *) g->v[v1]; /* TODO: get rid of casting */
	for (size_t i = 0; i < kv_size(*vec); i++) {
		if (kv_A(*vec, i) == v2) {
			fprintf(stdout, "edge %" PRIu32 
					" --> %" PRIu32 " exists already\n", v1, v2);
			return;
		}
	}

	kv_push(vid_t, *vec, v2);
	g->num_v++;
}

//void graph_del_edge(graph_t *g, vid_t v1, vid_t v2)
//{
//	/* 
//	 * Current implementation allows pages with 0 edges on them. We don't
//	 * free() empty pages neither move edges from one page to the other, yet
//	 * we make sure each page is not fragmented (empty slots)
//	 */
//	for (e_page_t *p = g->v[v1]; p != NULL; p = p->next) {
//		for (uint32_t i = 0; i < p->num_e; i++) {
//			if (p->vid[i] == v2) {
//				/* replace the current edge with the last one */
//				p->vid[i] = p->vid[p->num_e-1];
//				p->num_e--;
//				return;
//			}
//		}
//	}
//
//	return;
//}
//
///* For testing */
//uint32_t num_outgoing_edges(graph_t *g, vid_t v)
//{
//	uint32_t num_e = 0; /* total number of outgoing edges */
//	for (e_page_t *p = g->v[v]; p != NULL; p = p->next)
//		num_e += p->num_e;
//
//	return num_e;
//}
//
//KHASH_SET_INIT_INT(m32)
//int shortest_path_length(graph_t *g, vid_t src, vid_t dest)
//{
//	/* bfs */
//	if (src == dest)
//		return 0;
//	/* 
//	 * OPT:
//	 * - store pointers instead of vids (?) ... How do I go back to vid ?
//	 * - use bitmap instead of hastable (?)
//	 */
//	int not_visited;
//	kvec_t(vid_t) queue;
//	kv_init(queue);
//	kv_push(vid_t, queue, src);
//	uint32_t head = 0;	/* head idx */
//
//	khash_t(m32) *h = kh_init(m32);
//	kh_put(m32, h, src, &not_visited); /* ignore */
//
//	int dist = 0;
//	while (head < kv_size(queue)) {
//		++dist;
//		uint32_t size = kv_size(queue);
//		for (; head < size; head++) {
//			vid_t v = kv_A(queue, head); /* "dequeue" */
//			/* TODO: implement an iterator over all outgoing edges */
//			for (e_page_t *p = g->v[v]; p != NULL; p = p->next) {
//				for (uint32_t k = 0; k < p->num_e; k++) {
//					uint32_t u = p->vid[k];
//					if (u == dest)
//						return dist;
//					kh_put(m32, h, u, &not_visited);
//					if (not_visited)
//						kv_push(vid_t, queue, u);
//				}
//			}
//		}
//	}
//
//	kh_destroy(m32, h);
//	kv_destroy(queue);
//	return -1;
//}
//
//#define WORD_BITS (8 * sizeof(unsigned int))
//
//static inline void set_bit(unsigned int * bitmap, size_t idx)
//{
//    bitmap[idx / WORD_BITS] |= (1 << (idx % WORD_BITS));
//}
//
//static inline bool is_bit_set(unsigned int * bitmap, size_t idx)
//{
//	return !!(bitmap[idx / WORD_BITS] & (1 << (idx % WORD_BITS)));
//}
//
//
//int shortest_path_length_bitmap(graph_t *g, vid_t src, vid_t dest)
//{
//	/* bfs */
//	if (src == dest)
//		return 0;
//	/* 
//	 * OPT:
//	 * - store pointers instead of vids (?) ... How do I go back to vid ?
//	 * - use bitmap instead of hastable (?)
//	 */
//	kvec_t(vid_t) queue; 
//	kv_init(queue);
//	kv_push(vid_t, queue, src);
//	uint32_t head = 0;	/* head idx */
//
//	unsigned int * bitmap = calloc((1 << 21) / 8 + 1, sizeof(unsigned int));
//	set_bit(bitmap, src);
//
//	int dist = 0;
//	while (head < kv_size(queue)) {
//		++dist;
//		uint32_t size = kv_size(queue);
//		for (; head < size; head++) {
//			vid_t v = kv_A(queue, head); /* "dequeue" */
//			/* TODO: implement an iterator over all outgoing edges */
//			for (e_page_t *p = g->v[v]; p != NULL; p = p->next) {
//				for (uint32_t k = 0; k < p->num_e; k++) {
//					uint32_t u = p->vid[k];
//					if (u == dest)
//						return dist;
//
//					if (!is_bit_set(bitmap, u)) { /* not visited */
//						set_bit(bitmap, u);
//						kv_push(vid_t, queue, u);
//					}
//				}
//			}
//		}
//	}
//
//	free(bitmap);
//	kv_destroy(queue);
//	return -1;
//}
//
//
//void graph_dump(const graph_t * const g, const char *fname)
//{
//	FILE *fp;
//	if ((fp = fopen(fname, "w")) == NULL) {
//    	perror("fopen");
//		exit(EXIT_FAILURE);
//	}
//
//	for (uint32_t i = 0; i < MAX_V; i++) {
//		for (e_page_t *p = g->v[i]; p != NULL; p = p->next) {
//			for (uint32_t j = 0; j < p->num_e; j++) {
//				fprintf(fp, "%u\t%u\n", i, p->vid[j]);
//			}
//		}
//	}
//
//	fclose(fp);
//}
//
//
int main(void)
{
	vid_t v1, v2;
	char *p, buf[128];
	graph_t *g = graph_create(MAX_V);

	/* Read the initial graph */
    while (fgets(buf, sizeof(buf), stdin) != NULL) {
		if (buf[0] == 'S')
			break;
		v1 = strtoul(buf, &p, 10);
		++p;
		v2 = strtoul(p, NULL, 10);
		graph_add_edge(g, v1, v2);
    }

	graph_destroy(g);
	return 0;
}

///*
//	while ((ssize_t n = read(0, buf, sizeof(buf))) > 0) {
//		;
//		// I need to maintain state between subsequent read()s
//	}
//	return 0;
//*/
//
//	/* Fire up the workload! */
//	fprintf(stdout, "R\n");
//
//	while (fgets(buf, sizeof(buf), stdin) != NULL) {
//		switch (buf[0]) {
//			case 'A':
//				v1 = strtoul(buf+2, &p, 10);
//				++p;
//				v2 = strtoul(p, NULL, 10);
//				graph_add_edge(g, v1, v2);	
//				break;
//			case 'D':
//				v1 = strtoul(buf+2, &p, 10);
//				++p;
//				v2 = strtoul(p, NULL, 10);
//				graph_del_edge(g, v1, v2);	
//				break;
//			case 'Q':
//				v1 = strtoul(buf+2, &p, 10);
//				++p;
//				v2 = strtoul(p, NULL, 10);
//				int dist = shortest_path_length_bitmap(g, v1, v2);
//				fprintf(stdout, "%d\n", dist);
//				break;
//			case 'F': /* For the moment, ignore this */
//				break;
//			default:
//				fprintf(stderr, "Unsupported operation. Exiting...\n");
//				exit(EXIT_FAILURE);
//		}
//	}
//
//
///*
//#ifdef DEBUG
//	// Sanity checks
//	uint32_t src[] = { 340279, 1445297, 1151721, 309345, 822018 };
//	uint32_t dest[] = { 519122, 146499, 401121, 696543, 458677 };
//
//	for (int i = 0; i < 5; i++) {
//		int dist = shortest_path_length(g, src[i], dest[i]);
//		assert(dist == -1);
//	}
//
//	assert(num_outgoing_edges(g, 1) == 46473);
//	assert(num_outgoing_edges(g, 11) == 8);
//	assert(num_outgoing_edges(g, 111) == 0);
//	assert(num_outgoing_edges(g, 1111) == 0);
//	assert(num_outgoing_edges(g, 11111) == 0);
//	assert(num_outgoing_edges(g, 111111) == 2);
//	assert(num_outgoing_edges(g, 1111111) == 1);
//	assert(num_outgoing_edges(g, 7) == 323);
//	assert(num_outgoing_edges(g, 77) == 14);
//	assert(num_outgoing_edges(g, 777) == 0);
//	assert(num_outgoing_edges(g, 999) == 82);
//	assert(num_outgoing_edges(g, 9999) == 82);
//	assert(num_outgoing_edges(g, 2) == 18951);
//	assert(num_outgoing_edges(g, 16964) == 10042);
//	assert(num_outgoing_edges(g, 44275) == 4073);
//	assert(num_outgoing_edges(g, 105898) == 5365);
//	assert(num_outgoing_edges(g, 112128) == 1924);
//	graph_del_edge(g, 16964, 383147);
//	graph_del_edge(g, 16964, 106326);
//	graph_del_edge(g, 16964, 106506);
//	graph_del_edge(g, 16964, 383071);
//	graph_del_edge(g, 16964, 107111);
//	graph_del_edge(g, 16964, 103629);
//	graph_del_edge(g, 16964, 106259);
//	graph_del_edge(g, 16964, 106958);
//	graph_del_edge(g, 16964, 110050);
//	graph_del_edge(g, 16964, 108742);
//	graph_del_edge(g, 16964, 383095);
//	assert(num_outgoing_edges(g, 16964) == 10031);
//	fprintf(stdout, "Successfully passed all tests\n");
//#endif
//*/
//
//	return 0;
//}


// NOTES:
// When adding a new page, I should maybe prepend it to the rest instead of
// appending it. This should be decided based on the additions/deletions ratio.
// http://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)Graphs.html
