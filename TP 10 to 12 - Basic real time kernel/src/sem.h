/*
 * sem.h
 *
 *  Created on: 6 juin 2017
 *      Author: mi11p001
 */

#ifndef SEM_H_
#define SEM_H_

#include "fifo.h"

#define MAX_SEM		10
#define INIT_SEM	-MAX_SEM

typedef struct {
	FIFO* file ;				/* File circulaire des tâches en attente */
	short valeur ; 			/* compteur du sémaphore e(s) */
} SEMAPHORE ;

SEMAPHORE _sem[MAX_SEM];

void s_init( void );		/* initialise le tableau des sémaphores _sem. */
/* crée un sémaphore dans un emplacement libre,
 * retourne le numéro d’index dans _sem du sémaphore créé.
 * La valeur initiale du compteur est v. */
ushort s_cree( short v );
void s_close( ushort n );	/* supprime le sémaphore n. */
void s_wait( ushort n ); 	/* Implémente P(s). Tente de prendre le sémaphore n. */
void s_signal ( ushort n );	/* Implémente V(s). Libère le semaphore n. */

#endif /* SEM_H_ */
