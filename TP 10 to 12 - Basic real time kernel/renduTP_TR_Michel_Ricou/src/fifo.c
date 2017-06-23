#include <stdint.h>
#include "serialio.h"
#include "fifo.h"

void fifo_circ_init(FIFO* f)
{
	f->tete = 0;
	f->queue = 0;
	for(int i = 0; i < NB_MAX; i++) {
		f->fifo[i] = FILE_VIDE;
	}
}

char fifo_empty(FIFO* f) {
	if (f->fifo[f->queue] == FILE_VIDE) {
		//printf("File vide, f->queue = %d, f[q] = %d, fifo_VIDE = %d.", f->queue, _fifo_mutex[f->queue], fifo_VIDE);
		return 1;
	} else {
		return 0;
	}
}

char fifo_full(FIFO* f) {
	if (f->fifo[f->tete] != FILE_VIDE) {
		return 1;
	} else {
		return 0;
	}
}

char	fifo_nearly_full(FIFO* f)
{
	return (f->tete == (f->queue - 1))
			|| ((f->tete == (NB_MAX - 1))
					&& (f->queue == 0));
}

char	fifo_nearly_empty(FIFO* f)
{
	return (f->tete == (f->queue + 1))
			|| ((f->queue == (NB_MAX - 1))
					&& (f->tete == 0));
}

uint16_t fifo_next_idx(FIFO* f, uint16_t idx) {
	if(idx+1 < NB_MAX) {
		return idx+1;
	} else {
		return 0;
	}
}

int16_t	fifo_circ_push (FIFO* f, uint16_t n )
{
	if(!fifo_full(f)) {
		f->fifo[f->tete] = n;
		f->tete = fifo_next_idx(f, f->tete);
		return n;
	} else {
		return -1;
	}
}

int16_t	fifo_circ_pop(FIFO* f)
{
	int16_t res = -1;
	if(!fifo_empty(f)) {
		res = f->fifo[f->queue];

		f->fifo[f->queue] = FILE_VIDE;

		f->queue = fifo_next_idx(f, f->queue);
	}

	return res;
}

void affic_fifo_circ(FIFO* f)
{
	if(fifo_empty(f)){
		puts("Fifo vide");
		return;
	}

	uint16_t  idx = f->queue;
	while (idx != f->tete) {
		printf("[%d] -> %d\n", idx, f->fifo[idx]);
		idx = fifo_next_idx(f, idx);
	}
}
