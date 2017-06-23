#include <stdint.h>
#include "serialio.h"
#include "fifo.h"
#include "noyau.h"

#define NB_MAX  8
#define FILE_VIDE  -1

static int16_t  _file_mutex[NB_MAX];
static uint16_t  _tete;
static uint16_t  _queue;

void	file_circ_init();
char file_empty();
char file_full();
char	file_nearly_full();
char	file_nearly_empty();
uint16_t file_next_idx( uint16_t );
int16_t	file_circ_push ( uint16_t );
int16_t	file_circ_pop();
void affic_file_circ();

void file_circ_init()
{
	_tete = 0;
	_queue = 0;
	for(int i = 0; i < NB_MAX; i++) {
		_file_mutex[i] = FILE_VIDE;
	}
}

char file_empty() {
	if (_file_mutex[_queue] == FILE_VIDE) {
		//printf("File vide, _queue = %d, f[q] = %d, FILE_VIDE = %d.", _queue, _file_mutex[_queue], FILE_VIDE);
		return 1;
	} else {
		return 0;
	}
}

char file_full() {
	if (_file_mutex[_tete] != FILE_VIDE) {
		return 1;
	} else {
		return 0;
	}
}

char	file_nearly_full()
{
	return (_tete == (_queue - 1))
			|| ((_tete == (NB_MAX - 1))
					&& (_queue == 0));
}

char	file_nearly_empty()
{
	return (_tete == (_queue + 1))
			|| ((_queue == (NB_MAX - 1))
					&& (_tete == 0));
}

uint16_t file_next_idx(uint16_t idx) {
	if(idx+1 < NB_MAX) {
		return idx+1;
	} else {
		return 0;
	}
}

int16_t	file_circ_push ( uint16_t n )
{
	if(!file_full()) {
		_file_mutex[_tete] = n;
		_tete = file_next_idx(_tete);
		return n;
	} else {
		return -1;
	}
}

int16_t	file_circ_pop()
{
	int16_t res = -1;
	if(!file_empty()) {
		res = _file_mutex[_queue];

		_file_mutex[_queue] = FILE_VIDE;

		_queue = file_next_idx(_queue);
	}

	return res;
}

void affic_file_circ( void )
{
	if(file_empty()){
		puts("Fifo vide");
		return;
	}

	uint16_t  idx = _queue;
	while (idx != _tete) {
		printf("[%d] -> %d\n", idx, _file_mutex[idx]);
		idx = file_next_idx(idx);
	}
}

/*
static uint16_t  _file_mutex[NB_MAX];
static int16_t  _queue;

void	file_circ_init( void )
{
	_queue = -1;
	for(int i = 0; i < NB_MAX; i++) {
		_file_mutex[i] = FILE_VIDE;
	}
}

int16_t	file_circ_push ( int16_t n )
{
	if(_queue + 1 < NB_MAX) {
		_queue++;
		_file_mutex[_queue] = n;
		return n;
	} else {
		return -1;
	}
}

int16_t	file_circ_pop()
{
	int16_t res = -1;
	if(_queue > -1) {
		res = _file_mutex[0];

		for(int i = 0; i < _queue; i++) {
			_file_mutex[i] = _file_mutex[i+1];
		}

		for(int i = _queue; i < NB_MAX; i++) {
			_file_mutex[i] = FILE_VIDE;
		}

		_queue--;
	}

	return res;
}

uint16_t	get_nb_empty()
{
	return NB_MAX - _queue - 1;
}

uint16_t	get_nb_max()
{
	return NB_MAX;
}

void affic_file_circ( void )
{
	if(_queue == -1){
		printf("Fifo vide\n");
		return;
	}

	for(int i = 0; i <= _queue; i++) {
		printf("[%d] -> %d\n", i, _file_mutex[i]);
	}
}
*/
