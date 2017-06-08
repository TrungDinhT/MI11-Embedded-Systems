/* NOYAU.C */
/*--------------------------------------------------------------------------*
 *               Code C du noyau preemptif qui tourne sur ARM                *
 *                                 NOYAU.C                                  *
 *--------------------------------------------------------------------------*/

#include <stdint.h>

#include "serialio.h"
#include "imx_timers.h"
#include "imx_aitc.h"
#include "noyau.h"

/*--------------------------------------------------------------------------*
 *            Variables internes du noyau                                   *
 *--------------------------------------------------------------------------*/
static int compteurs[MAX_TACHES]; /* Compteurs d'activations */
CONTEXTE _contexte[MAX_TACHES];   /* tableau des contextes */
volatile uint16_t _tache_c;       /* num�ro de tache courante */
uint32_t  _tos;                   /* adresse du sommet de pile */
int  _ack_timer = 1;              /* = 1 si il faut acquitter le timer */

/*--------------------------------------------------------------------------*
 *                        Fin de l'execution                                *
 *----------------------------------------------------------------------- --*/
void	noyau_exit(void)
{
  int j;
  _irq_disable_();                /* D�sactiver les interruptions */
  printf("Sortie du noyau\n");
  for (j=0; j < MAX_TACHES; j++)
    printf("\nActivations tache %d : %d", j, compteurs[j]);
  for(;;);                        /* Terminer l'ex�cution */
}

/*--------------------------------------------------------------------------*
 *                        --- Fin d'une tache ---                           *
 * Entree : Neant                                                           *
 * Sortie : Neant                                                           *
 * Descrip: Cette proc. doit etre appelee a la fin des taches               *
 *                                                                          *
 *----------------------------------------------------------------------- --*/
void  fin_tache(void)
{
  /* on interdit les interruptions */
  _irq_disable_();
  /* la tache est enlevee de la file des taches */
  _contexte[_tache_c].status = CREE;
  retire(_tache_c);
  schedule();

}

/*--------------------------------------------------------------------------*
 *                        --- Creer une tache ---                           *
 * Entree : Adresse de la tache                                             *
 * Sortie : Numero de la tache creee                                        *
 * Descrip: Cette procedure cree une tache en lui allouant                  *
 *    - une pile                                                            *
 *    - un numero                                                           *
 * Err. fatale: priorite erronnee, depassement du nb. maximal de taches     *
 *	                                                                        *
 *--------------------------------------------------------------------------*/
uint16_t cree( TACHE_ADR adr_tache )
{
  CONTEXTE *p;                    /* pointeur d'une case de _contexte */
  static   uint16_t tache = -1;   /* contient numero dernier cree */


  _lock_();                       /* debut section critique */
  tache++;                        /* numero de tache suivant */

  if (tache >= MAX_TACHES)        /* sortie si depassement */
    noyau_exit();

  p = &_contexte[tache];          /* contexte de la nouvelle tache */

  p->sp_ini = _tos;               /* allocation d'une pile a la tache */
  _tos -= PILE_TACHE + PILE_IRQ;  /* decrementation du pointeur de pile pour*/
                                  /* la prochaine tache. */

  _unlock_();                     /* fin section critique */

  p->tache_adr = adr_tache;       /* memorisation adresse debut de tache */
  p->status = CREE; 		          /* mise a l'etat CREE */
  return(tache);                  /* tache est un uint16_t */
}

/*--------------------------------------------------------------------------*
 *                          --- Elire une tache ---                         *
 * Entree : Numero de la tache                                              *
 * Sortie : Neant                                                           *
 * Descrip: Cette procedure place une tache dans la file d'attente des      *
 *	    taches eligibles.                                                   *
 *	    Les activations ne sont pas memorisee                               *
 * Err. fatale: Tache inconnue	                                            *
 *                                                                          *
 *--------------------------------------------------------------------------*/
void  active( uint16_t tache )
{
  CONTEXTE *p = &_contexte[tache]; /* acces au contexte tache */

  if (p->status == NCREE)
    noyau_exit();                 /* sortie du noyau */

  _lock_();                       /* debut section critique */
  if (p->status == CREE)          /* n'active que si receptif */
  {
    p->status = PRET;             /* changement d'etat, mise a l'etat PRET */
    ajoute(tache);                /* ajouter la tache dans la liste */
    schedule();                /* activation d'une tache prete */
  }
  _unlock_();                     /* fin section critique */
}


/*--------------------------------------------------------------------------*
 *                  ORDONNANCEUR preemptif optimise                         *
 *                                                                          *
 *             !! Cette fonction doit s'ex�cuter en mode IRQ !!             *
 *  !! Pas d'appel direct ! Utiliser schedule pour provoquer une            *
 *  commutation !!                                                          *
 *--------------------------------------------------------------------------*/
void __attribute__((naked)) scheduler( void )
{
  register CONTEXTE *p;
  register unsigned int sp asm("sp");  /* Pointeur de pile */

  /* Sauvegarder le contexte complet sur la pile IRQ */
  __asm__ __volatile__(
      "stmfd  sp, {r0-r14}^\t\n"  /* Sauvegarde registres mode system */
      "nop\t\n"                   /* Attendre un cycle */
      "sub    sp, sp, #60\t\n"    /* Ajustement pointeur de pile */
      "mrs    r0, spsr\t\n"       /* Sauvegarde de spsr_irq */
      "stmfd  sp!, {r0, lr}\t\n");/* et de lr_irq */

  if (_ack_timer)                 /* R�initialiser le timer si n�cessaire */
  {
    register struct imx_timer* tim1 = (struct imx_timer *) TIMER1_BASE;
    tim1->tstat &=~TSTAT_COMP;
  }
  else
  {
    _ack_timer = 1;
  }

  _contexte[_tache_c].sp_irq = sp;/* memoriser le pointeur de pile */
  _tache_c = suivant();           /* recherche du suivant */
  if (_tache_c == F_VIDE)
  {
    printf("Plus rien a ordonnancer.\n");
    noyau_exit();                 /* Sortie du noyau */
  }
  compteurs[_tache_c]++;          /* Incr�menter le compteur d'activations  */
  p = &_contexte[_tache_c];       /* p pointe sur la nouvelle tache courante*/

  if (p->status == PRET)          /* tache prete ? */
  {
    sp = p->sp_ini;               /* Charger sp_irq initial */
    _set_arm_mode_(ARMMODE_SYS);  /* Passer en mode syst�me */
    sp = p->sp_ini - PILE_IRQ;    /* Charger sp_sys initial */
    p->status = EXEC;             /* status tache -> execution */
    _irq_enable_();               /* autoriser les interuptions   */
    (*p->tache_adr)();            /* lancement de la t�che */
  }
  else
  {
    sp = p->sp_irq;               /* tache deja en execution, restaurer sp_irq */
  }

  /* Restaurer le contexte complet depuis la pile IRQ */
  __asm__ __volatile__(
      "ldmfd  sp!, {r0, lr}\t\n"  /* Restaurer lr_irq */
      "msr    spsr, r0\t\n"       /* et spsr_irq */
      "ldmfd  sp, {r0-r14}^\t\n"  /* Restaurer registres mode system */
      "nop\t\n"                   /* Attendre un cycle */
      "add    sp, sp, #60\t\n"    /* Ajuster pointeur de pile irq */
      "subs   pc, lr, #4\t\n");   /* Retour d'exception */
}


/*--------------------------------------------------------------------------*
 *                  --- Provoquer une commutation ---                       *
 *                                                                          *
 *             !! Cette fonction doit s'ex�cuter en mode IRQ !!             *
 *  !! Pas d'appel direct ! Utiliser schedule pour provoquer une            *
 *  commutation !!                                                          *
 *--------------------------------------------------------------------------*/
void  schedule( void )
{
  _lock_();                         /* Debut section critique */

  /* On simule une exception irq pour forcer un appel correct � scheduler().*/
  _ack_timer = 0;
  _set_arm_mode_(ARMMODE_IRQ);      /* Passer en mode IRQ */
  __asm__ __volatile__(
      "mrs  r0, cpsr\t\n"           /* Sauvegarder cpsr dans spsr */
      "msr  spsr, r0\t\n"
      "add  lr, pc, #4\t\n"         /* Sauvegarder pc dans lr et l'ajuster */
      "b    scheduler\t\n"          /* Saut � scheduler */
      );
  _set_arm_mode_(ARMMODE_SYS);      /* Repasser en mode system */

  _unlock_();                       /* Fin section critique */
}

/*--------------------------------------------------------------------------*
 *                        --- Lancer le systeme ---                         *
 * Entree : Adresse de la premiere tache a lancer                           *
 * Sortie : Neant                                                           *
 * Descrip: Active la tache et lance le systeme                             *
 *                                                                          *
 *                                                                          *
 * Err. fatale: Neant                                                       *
 *                                                                          *
 *--------------------------------------------------------------------------*/
void	start( TACHE_ADR adr_tache )
{
  short j;
  register unsigned int sp asm("sp");
  struct imx_timer* tim1 = (struct imx_timer *) TIMER1_BASE;
  struct imx_aitc* aitc = (struct imx_aitc *) AITC_BASE;

  for (j=0; j<MAX_TACHES; j++)
  {
    _contexte[j].status = NCREE;    /* initialisation de l'etat des taches */
  }
  _tache_c = 0;                     /* initialisation de la tache courante */
  file_init();                      /* initialisation de la file           */

  _tos = sp;                        /* Haut de la pile des t�ches */
  _set_arm_mode_(ARMMODE_IRQ);      /* Passer en mode IRQ */
  sp = _tos;                        /* sp_irq initial */
  _set_arm_mode_(ARMMODE_SYS);      /* Repasser en mode SYS */

  _irq_disable_();                  /* on interdit les interruptions */

  /* Initialisation du timer � 100 Hz */
  tim1->tcmp = 10000;
  tim1->tprer = 0;
  tim1->tctl |= TCTL_TEN | TCTL_IRQEN | TCTL_CLKSOURCE_PERCLK16;

  /* Initialisation de l'AITC */
  aitc->intennum = TIMER1_INT;

  active(cree(adr_tache));          /* creation et activation premiere tache */
}


/*-------------------------------------------------------------------------*
 *                    --- Endormir la t�che courante ---                   *
 * Entree : Neant                                                          *
 * Sortie : Neant                                                          *
 * Descrip: Endort la t�che courante et attribue le processeur � la t�che  *
 *          suivante.                                                      *
 *                                                                         *
 * Err. fatale:Neant                                                       *
 *                                                                         *
 *-------------------------------------------------------------------------*/

void  dort(void)
{
	CONTEXTE *p;
	_lock_();
	p = &_contexte[_tache_c];

	if (p->status == NCREE)
	  noyau_exit();

	if (p->status == EXEC) {
		p->status = SUSP;
		retire(_tache_c);
		schedule();
	}
	_unlock_();

}

/*-------------------------------------------------------------------------*
 *                    --- R�veille une t�che ---                           *
 * Entree : num�ro de la t�che � r�veiller                                 *
 * Sortie : Neant                                                          *
 * Descrip: R�veille une t�che. Les signaux de r�veil ne sont pas m�moris�s*
 *                                                                         *
 * Err. fatale:t�che non cr��e                                             *
 *                                                                         *
 *-------------------------------------------------------------------------*/


void reveille(uint16_t t)
{
  CONTEXTE *p;
  _lock_();
  p = &_contexte[t];

  // printf("Statut de %d = %d.\n", t, p->status);
  if (p->status == NCREE)
    noyau_exit();

  if (p->status == SUSP) {
	p->status = EXEC;
	ajoute(t);
	schedule();
  }
  _unlock_();
}
