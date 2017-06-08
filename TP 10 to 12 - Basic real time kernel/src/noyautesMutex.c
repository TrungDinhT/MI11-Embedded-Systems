/* NOYAUTEST.C */
/*--------------------------------------------------------------------------*
 *			      Programme de tests			    *
 *--------------------------------------------------------------------------*/

#include "serialio.h"
#include "noyau.h"

#define NB_LOOPS	20
#define PROD_wait_OLD	100000
#define CONS_wait_OLD	100000

/*
 ** Test du noyau preemptif. Lier noyautes.c avec noyau.c et noyaufil.c
 */

TACHE	tache0_OLD(void);
TACHE	producteur(void);
TACHE	consommateur(void);
void	wait_OLD(uint32_t);

TACHE	tache0_OLD(void)
{
  puts("------> EXEC tache A");
  active(cree(producteur));
  active(cree(consommateur));
  fin_tache();
}

TACHE	producteur(void)
{
  puts("------> DEBUT producteur");

  for (int n = 0; n < NB_LOOPS; n++) {
	  wait_OLD(PROD_wait_OLD);
	  int res = file_circ_push ( n );
	  if (res == -1) {
		  puts("Erreur : la file est pleine !");
	  } else {
		  printf("Producteur a ecrit %d\n", res);
	  }
	  affic_file_circ();

	  /*
	  if (get_nb_empty() == 0) {
		  puts("Prod s'endort");
		  dort();
	  }

	  if (get_nb_empty() == get_nb_max() - 1) {
		  printf("Prod reveille cons car empty = %d \n", get_nb_empty());
		  reveille(2);
	  }
	  */

	  if (file_full()) {
		  puts("Prod s'endort");
		  dort();
	  }

	  if (file_nearly_empty()) {
		  puts("Prod reveille cons");
		  reveille(2);
	  }
  }
  puts("------> FIN producteur");
  fin_tache();

  /**
     * prod(1)
     * attendre(Ts)
     *
     **/

  /**
   * si nbv ==0
   * 	dort();
   * si nbv == NB_MAX-1
   * 	réveiller(cons);
   *
   */
}

TACHE	consommateur(void)
{
  puts("------> DEBUT consommateur");
  puts("Cons s'endort");
  dort();
  while (1) {
	  wait_OLD(CONS_wait_OLD);
	  int res = file_circ_pop ();

	  if (res == -1) {
		  puts("Erreur : la file est vide");
	  } else {
		  printf("Consommateur a lu %d\n", res);
	  }
	  affic_file_circ();

	  if (file_empty()) {
		  puts("Cons s'endort");
		  dort();
	  }
	  if (file_nearly_full()) {
		  puts("Cons reveille prod");
		  reveille(1);
	  }
  }

  /**
   * cons(1)
   * attendre(Tr)
   * nb++
   * si nb= 1024
   * 	calcul... //simulé par attente
   */

  /**
   * si nbv == max
   * 	dort();
   * si nbv == 1
   * 	réveiller(prod);
   *
   */
}

void wait_OLD (uint32_t n) {
    for (int j=0; j<n; j++);
}


int main_OLD()
{
  serial_init(115200);
  puts("Test noyau MUTEX");

  file_circ_init();

  start(tache0_OLD);
  return(0);
}

