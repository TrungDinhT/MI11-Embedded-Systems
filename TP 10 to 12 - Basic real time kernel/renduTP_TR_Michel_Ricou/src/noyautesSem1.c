/* NOYAUTEST.C */
/*--------------------------------------------------------------------------*
 *			      Programme de tests pour sémaphores			    *
 *--------------------------------------------------------------------------*/

#include "serialio.h"
#include "noyau.h"
#include "sem.h"

#define NB_LOOPS	20
#define PROD_WAIT	50000
#define CONS_WAIT	100000
#define NB_PROD		2
#define NB_CONS		2

/*
 ** Test du noyau preemptif. Lier noyautes.c avec noyau.c et noyaufil.c
 */

TACHE	tache0(void);
TACHE	prod(void);
TACHE	consomm(void);
void	wait(uint32_t);

static short sem_id_prod_to_cons, sem_id_cons_to_prod;
int ugly_global_variable;

TACHE	tache0(void)
{
  puts("------> EXEC tache A");
  file_circ_init();
  sem_id_prod_to_cons = s_cree( 0 );
  sem_id_cons_to_prod = s_cree( 0 );
  int i;
  for(i=0; i<NB_PROD; i++){
	  active(cree(prod));
  }
  for(i=0; i<NB_CONS; i++){
	  active(cree(consomm));
  }
  fin_tache();
}

TACHE	prod()
{
	printf("------> DEBUT Prod%d\n", _tache_c);

  for (int n = _tache_c; n < NB_LOOPS; n += NB_PROD) {
	  wait(PROD_WAIT);
	  int res = file_circ_push ( n );
	  if (res == -1) {
		  printf("Erreur Prod%d : la file est pleine !\n", _tache_c);
		  n -= NB_PROD;
	  } else {
		  printf("[Prod%d > File] %d\n", _tache_c, res);
	  }
	  affic_file_circ();

	  if (file_full()) {
		  printf("Prod%d s'endort\n", _tache_c);
		  s_wait(sem_id_cons_to_prod);
		  printf("Prod%d se reveille\n", _tache_c);
	  }

	  if (file_nearly_empty()) {
		  printf("Prod%d reveille cons\n", _tache_c);
		  s_signal(sem_id_prod_to_cons);
	  }
  }
  printf("------> FIN Prod%d\n", _tache_c);
  fin_tache();
}

TACHE	consomm(void)
{
  printf("------> DEBUT Cons%d\n", _tache_c);
  while (1) {
	  wait(CONS_WAIT);
	  int res = file_circ_pop ();

	  if (res == -1) {
		  printf("Erreur Cons%d : la file est vide\n", _tache_c);
	  } else {
		  printf("[File > Cons%d] %d\n", _tache_c, res);
	  }
	  affic_file_circ();

	  if (file_empty()) {
		  printf("Cons%d s'endort\n", _tache_c);
		  s_wait(sem_id_prod_to_cons);
	  }
	  if (file_nearly_full()) {
		  printf("Cons%d reveille prod\n", _tache_c);
		  s_signal(sem_id_cons_to_prod);
		  s_signal(sem_id_cons_to_prod);
	  }
  }
  printf("------> FIN Cons%d\n", _tache_c);
  fin_tache();
}

void wait (uint32_t n) {
    for (int j=0; j<n; j++);
}


int main()
{
	serial_init(115200);
	puts("Test noyau Semaphore");

	s_init();

	start(tache0);
	return(0);
}

