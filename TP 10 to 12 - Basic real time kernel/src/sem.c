/*
 * sem.c
 *
 *  Created on: 6 juin 2017
 *      Author: mi11p001
 */

#include "noyau.h"
#include "sem.h"
#include "serialio.h"

/* initialise le tableau des sémaphores _sem. */
void s_init( void ) {
	for(short i = 0; i < MAX_SEM; i++) {
		FIFO new_fifo;
		_sem[i].valeur = INIT_SEM;
		_sem[i].file = &new_fifo;
	}
}

/* crée un sémaphore dans un emplacement libre,
 * retourne le numéro d’index dans _sem du sémaphore créé.
 * La valeur initiale du compteur est v. */
ushort s_cree( short v ) {
	short i = 0;
	while (i <= MAX_SEM
			&& _sem[i].valeur != INIT_SEM) {
		i++;
	}

	if(i<MAX_SEM) {
		_sem[i].valeur = v;
		fifo_circ_init(_sem[i].file);
		return i;
	} else {
		return MAX_SEM;
	}
}

/* supprime le sémaphore n. */
void s_close( ushort n ) {
	if (_sem[n].valeur < 0) {
		_sem[n].valeur = INIT_SEM;
	}
}

/* Implémente P(s). Tente de prendre le sémaphore n. */
void s_wait( ushort n ){
	//printf("Wait : Le semaphore a pour valeur %d -> --\n", _sem[n].valeur);
	_sem[n].valeur--;

	if (_sem[n].valeur < 0) {
		// Bloquer la tâche et la mettre dans la file
		fifo_circ_push(_sem[n].file, _tache_c);
		//puts("S'endort en attendant le semaphore");
		dort();
	} else {
		//puts("J'ai le semaphore !!! :D");
	}
}

/* Implémente V(s). Libère le semaphore n. */
void s_signal ( ushort n ) {
	//printf("Signal : Le semaphore a pour valeur %d -> ++\n", _sem[n].valeur);
	_sem[n].valeur++;

	if (_sem[n].valeur <= 0) {
		// Libérer (délivrer) la prochaine tâche de la fifo
		int16_t tacheId = fifo_circ_pop(_sem[n].file);
		//printf("Donne le semaphore au suivant : %d\n", tacheId);
		reveille(tacheId);
	}
}
