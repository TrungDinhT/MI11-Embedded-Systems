/*
 * fifo.h
 *
 *  Created on: 6 juin 2017
 *      Author: mi11p001
 */

#ifndef FIFO_H_
#define FIFO_H_

#include "noyau.h"

#define NB_MAX  8
#define FILE_VIDE  -1

typedef struct {
	int16_t  fifo[NB_MAX];	/* Liste des tâches en attentes (dans le cas d'un sémaphore */
	uint16_t  tete;			/* Dernière tâche ajoutée */
	uint16_t  queue;		/* Prochaine tâche à relancer */
} FIFO ;

void	fifo_circ_init( FIFO* f );
char fifo_empty( FIFO* f );
char fifo_full( FIFO* f );
char	fifo_nearly_full( FIFO* f );
char	fifo_nearly_empty( FIFO* f );
uint16_t fifo_next_idx( FIFO* f, uint16_t );
int16_t	fifo_circ_push ( FIFO* f, uint16_t );
int16_t	fifo_circ_pop( FIFO* f );
void affic_fifo_circ( FIFO* f );

#endif /* FIFO_H_ */
