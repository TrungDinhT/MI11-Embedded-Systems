# Compte-rendu TP "Réalisation d’un mini noyau temps réel - Partie 3 et 4"

**Mewen Michel et Sander Ricou - MI11 UTC**
### Partie 3 : exclusion mutuelle

Questions:
*On nous demande d’ajouter au mini-noyau temps réel la gestion de la suspension et du réveil d’une tâche. 
On a donc réalisé les deux primitives suivantes :
- `void dort(void)` : endort (suspend) la tâche courante
- `void reveille(register ushort tache)` : réveille la tâche tache. Le signal de réveil n’est pas mémorisé si la tâche n’est pas suspendue. 
Vous modifierez les fichiers NOYAU.C et NOYAU.H afin d’implémenter ces deux primitives.*

Voici le code de ces fonctions :

```c
void reveille(uint16_t t)
{
	CONTEXTE *p;
	//récuparation du contexte général
	p = &_contexte[t];
 
	// Erreur si tâche non créée
	if (p->status == NCREE)
	noyau_exit();
 
	// Protection des accès sur la file
	_lock_();
	if (p->status == SUSP) {
		p->status = EXEC;

		// Ajout de la tâche précédement endormie à la file des tâches à exécuter
		ajoute(t);

		// Appel de la tâche suivante par le scheduler
		schedule();
	}	
	_unlock_();
}
 
void dort(void)
{
	CONTEXTE *p;
	p = &_contexte[_tache_c];
 
	if (p->status == NCREE)
		noyau_exit();
 
	_lock_();
	if (p->status == EXEC) {
		p->status = SUSP;
		retire(_tache_c);
		// Appel de la tâche suivante par le scheduler
		schedule();
	}
	_unlock_();
 
}
```

Quand on suspend une tâche, on la retire donc de la file des tâches à exécuter et on la rajoute au réveil. Il ne faut pas oublier les appelle à la fonction `schedule()` permettant de lancer la tâche suivante dans le scheduler. De plus nous vérifions bien que la tâche est en execution pour l'endormir et inversement suspendu pour la réveiller. Si la tâche n'est pas créée alors on interromp le noyau.
Notons également l'utilisation des primitives `_lock_()` et `_unlock_()` qui évitent les péemptions pendant l'exécution du code entre ces deux fonctions, aucune interruption n'est acceptée. Ceci nous assure la protection de la section critique.


On réalise maintenant un producteur et un consommateur selon les besoins définis dans le sujet :
*Vous écrirez un petit programme de test mettant en œuvre ces deux primitives dans le cas du modèle de communications producteur/consommateur. Le programme devra comporter au moins deux tâches ; la première, le producteur, produira des entiers courts dans une file circulaire, la seconde, le consommateur, retirera ces entiers de la file et les affichera.*

Le producteur va s'endormir si le nombre de places vides est nul et il va réveiller le consommateur s'il y a un seul élément dans la file. Le consommateur va s'endormir si le nombre places vides est égal à la taille de la file et va réveiller le producteur dès qu'il y a une case vide. Il peut donc y avoir les 2 tâches lancées en même temps mais elles ne peuvent pas être éteintes en même temps !

On va donc s'inspirer de notre file du TP précédent pour que les deux tâches se transmettent des entiers. En dehors de cette communication, on va les occuper pour simuler la préparation et le traitement des données.

Nos variables globales:
```c
#define NB_LOOPS	20
#define PROD_wait_OLD	100000
#define CONS_wait_OLD	100000
```

##### Le producteur
Pendant `NB_LOOPS` itérations le producteur va produire un entier `n` dans la file circulaire (`file_circ_push(n)`), il affiche ensuite la file. Puis si celle-ci est pleine, il s'endort et si il remarque que la file à au moins un élément alors il réveille le consommateur avec `reveille(2)` (tâche identifiée par 2, car créée en second). A la fin des itérations le producteur appelle `fin_tache()`, la tâche est alors interrompue.

Voici notre tâche producteur :
```c
TACHE	producteur(void)
{
  puts("------> DEBUT producteur");

  for (int n = 0; n < NB_LOOPS; n++) {
	  wait(PROD_wait);
	  int res = file_circ_push ( n );
	  if (res == -1) {
		  puts("Erreur : la file est pleine !");
	  } else {
		  printf("Producteur a ecrit %d\n", res);
	  }
	  affic_file_circ();
	  
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
}
```

##### Le consommateur
Le consommateur est endormi des sa création, il sera réveiller par le producteur quand celui-ci aura produit un premier élément dans la file circulaire. Des qu'il est réveillé il se met à consommer les entiers de la file.
La file est à nouveau afficher et le consommateur teste si la file est vide, si c'est la cas il s'endort.
Des qu'il s'aperçoit que la file à un premier élément vide il réveille alors le producteur (première tâche créée d'où le numéro 1), `reveille(1)`.

Voici notre tâche consommateur:
```c
TACHE	consommateur(void)
{
  puts("------> DEBUT consommateur");
  puts("Cons s'endort");
  dort();
  while (1) {
	  wait(CONS_wait);
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
}
```



Note sur FIFO circulaire implémentée:
La file ciculaire utilisée dans le producteur et le consommateur est initialisée dans notre main avec la fonction `fifo_circ_init()`.

```c
void fifo_circ_init(FIFO* f)
{
	f->tete = 0;
	f->queue = 0;
	for(int i = 0; i < NB_MAX; i++) {
		f->fifo[i] = FILE_VIDE;
	}
}
```

```c
char fifo_empty(FIFO* f) {
	if (f->fifo[f->queue] == FILE_VIDE) {
		//printf("File vide, f->queue = %d, f[q] = %d, fifo_VIDE = %d.", f->queue, _fifo_mutex[f->queue], fifo_VIDE);
		return 1;
	} else {
		return 0;
	}
}
```

```c
char fifo_full(FIFO* f) {
	if (f->fifo[f->tete] != FILE_VIDE) {
		return 1;
	} else {
		return 0;
	}
}
```

```c
char	fifo_nearly_full(FIFO* f)
{
	return (f->tete == (f->queue - 1))
			|| ((f->tete == (NB_MAX - 1))
					&& (f->queue == 0));
}
```

```c
char	fifo_nearly_empty(FIFO* f)
{
	return (f->tete == (f->queue + 1))
			|| ((f->queue == (NB_MAX - 1))
					&& (f->tete == 0));
}
```

```c
uint16_t fifo_next_idx(FIFO* f, uint16_t idx) {
	if(idx+1 < NB_MAX) {
		return idx+1;
	} else {
		return 0;
	}
}
```

```c
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
```

```c
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
```

```c
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
```

TEST du MUTEX:

Nous avons effectué trois tests différents en faisant varier les temps de traitement du producteur et du consommateur.

Avec le producteur plus rapide,
```
Test noyau MUTEX
------> EXEC tache A
------> DEBUT producteur
------> DEBUT consommateur
Cons s'endort
Producteur a ecrit 0
[0] -> 0
Prod reveille cons
Producteur a ecrit 1
[0] -> 0
[1] -> 1
Producteur a ecrit 2
[0] -> 0
[1] -> 1
[2] -> 2
Producteur a ecrit 3
[0] -> 0
[1] -> 1
[2] -> 2
[3] -> 3
Producteur a ecrit 4
[0] -> 0
[1] -> 1
[2] -> 2
[3] -> 3
[4] -> 4
Producteur a ecrit 5
[0] -> 0
[1] -> 1
[2] -> 2
[3] -> 3
[4] -> 4
[5] -> 5
Producteur a ecrit 6
[0] -> 0
[1] -> 1
[2] -> 2
[3] -> 3
[4] -> 4
[5] -> 5
[6] -> 6
Producteur a ecrit 7
Prod s'endort
Consommateur a lu 0
[1] -> 1
[2] -> 2
[3] -> 3
[4] -> 4
[5] -> 5
[6] -> 6
[7] -> 7
Cons reveille prod
Producteur a ecrit 8
Prod s'endort
Consommateur a lu 1
[2] -> 2
[3] -> 3
[4] -> 4
[5] -> 5
[6] -> 6
[7] -> 7
[0] -> 8
Cons reveille prod
Producteur a ecrit 9
Prod s'endort
Consommateur a lu 2
[3] -> 3
[4] -> 4
[5] -> 5
[6] -> 6
[7] -> 7
[0] -> 8
[1] -> 9
Cons reveille prod
Producteur a ecrit 10
Prod s'endort
Consommateur a lu 3
[4] -> 4
[5] -> 5
[6] -> 6
[7] -> 7
[0] -> 8
[1] -> 9
[2] -> 10
Cons reveille prod
Producteur a ecrit 11
Prod s'endort
Consommateur a lu 4
[5] -> 5
[6] -> 6
[7] -> 7
[0] -> 8
[1] -> 9
[2] -> 10
[3] -> 11
Cons reveille prod
Producteur a ecrit 12
Prod s'endort
Consommateur a lu 5
[6] -> 6
[7] -> 7
[0] -> 8
[1] -> 9
[2] -> 10
[3] -> 11
[4] -> 12
Cons reveille prod
Producteur a ecrit 13
Prod s'endort
Consommateur a lu 6
[7] -> 7
[0] -> 8
[1] -> 9
[2] -> 10
[3] -> 11
[4] -> 12
[5] -> 13
Cons reveille prod
Producteur a ecrit 14
Prod s'endort
Consommateur a lu 7
[0] -> 8
[1] -> 9
[2] -> 10
[3] -> 11
[4] -> 12
[5] -> 13
[6] -> 14
Cons reveille prod
Producteur a ecrit 15
Prod s'endort
Consommateur a lu 8
[1] -> 9
[2] -> 10
[3] -> 11
[4] -> 12
[5] -> 13
[6] -> 14
[7] -> 15
Cons reveille prod
Producteur a ecrit 16
Prod s'endort
Consommateur a lu 9
[2] -> 10
[3] -> 11
[4] -> 12
[5] -> 13
[6] -> 14
[7] -> 15
[0] -> 16
Cons reveille prod
Producteur a ecrit 17
Prod s'endort
Consommateur a lu 10
[3] -> 11
[4] -> 12
[5] -> 13
[6] -> 14
[7] -> 15
[0] -> 16
[1] -> 17
Cons reveille prod
Producteur a ecrit 18
Prod s'endort
Consommateur a lu 11
[4] -> 12
[5] -> 13
[6] -> 14
[7] -> 15
[0] -> 16
[1] -> 17
[2] -> 18
Cons reveille prod
Producteur a ecrit 19
Prod s'endort
Consommateur a lu 12
[5] -> 13
[6] -> 14
[7] -> 15
[0] -> 16
[1] -> 17
[2] -> 18
[3] -> 19
Cons reveille prod
------> FIN producteur
Consommateur a lu 13
[6] -> 14
[7] -> 15
[0] -> 16
[1] -> 17
[2] -> 18
[3] -> 19
Consommateur a lu 14
[7] -> 15
[0] -> 16
[1] -> 17
[2] -> 18
[3] -> 19
Consommateur a lu 15
[0] -> 16
[1] -> 17
[2] -> 18
[3] -> 19
Consommateur a lu 16
[1] -> 17
[2] -> 18
[3] -> 19
Consommateur a lu 17
[2] -> 18
[3] -> 19
Consommateur a lu 18
[3] -> 19
Consommateur a lu 19
Plus rien \0xef\0xbf\0xbd ordonnancer.
Sortie du noyau
 
Activations tache 0 : 3
Activations tache 1 : 99
Activations tache 2 : 896
Activations tache 3 : 0
Activations tache 4 : 0
Activations tache 5 : 0
Activations tache 6 : 0
Activations tache 7 : 0
 
avec
#define NB_LOOPS    20
#define PROD_WAIT    100000
#define CONS_WAIT    1000000
 ```
 
 
 ```
Test noyau MUTEX
------> EXEC tache A
------> DEBUT producteur
------> DEBUT consommateur
Cons s'endort
Producteur a ecrit 0
[0] -> 0
Prod reveille cons
Consommateur a lu 0
Fifo vide
Cons s'endort
Producteur a ecrit 1
[1] -> 1
Prod reveille cons
Consommateur a lu 1
Fifo vide
Cons s'endort
Producteur a ecrit 2
[2] -> 2
Prod reveille cons
Consommateur a lu 2
Fifo vide
Cons s'endort
Producteur a ecrit 3
[3] -> 3
Prod reveille cons
Consommateur a lu 3
Fifo vide
Cons s'endort
Producteur a ecrit 4
[4] -> 4
Prod reveille cons
Consommateur a lu 4
Fifo vide
Cons s'endort
Producteur a ecrit 5
[5] -> 5
Prod reveille cons
Consommateur a lu 5
Fifo vide
Cons s'endort
Producteur a ecrit 6
[6] -> 6
Prod reveille cons
Consommateur a lu 6
Fifo vide
Cons s'endort
Producteur a ecrit 7
[7] -> 7
Prod reveille cons
Consommateur a lu 7
Fifo vide
Cons s'endort
Producteur a ecrit 8
[0] -> 8
Prod reveille cons
Consommateur a lu 8
Fifo vide
Cons s'endort
Producteur a ecrit 9
[1] -> 9
Prod reveille cons
Consommateur a lu 9
Fifo vide
Cons s'endort
Producteur a ecrit 10
[2] -> 10
Prod reveille cons
Consommateur a lu 10
Fifo vide
Cons s'endort
Producteur a ecrit 11
[3] -> 11
Prod reveille cons
Consommateur a lu 11
Fifo vide
Cons s'endort
Producteur a ecrit 12
[4] -> 12
Prod reveille cons
Consommateur a lu 12
Fifo vide
Cons s'endort
Producteur a ecrit 13
[5] -> 13
Prod reveille cons
Consommateur a lu 13
Fifo vide
Cons s'endort
Producteur a ecrit 14
[6] -> 14
Prod reveille cons
Consommateur a lu 14
Fifo vide
Cons s'endort
Producteur a ecrit 15
[7] -> 15
Prod reveille cons
Consommateur a lu 15
Fifo vide
Cons s'endort
Producteur a ecrit 16
[0] -> 16
Prod reveille cons
Consommateur a lu 16
Fifo vide
Cons s'endort
Producteur a ecrit 17
[1] -> 17
Prod reveille cons
Consommateur a lu 17
Fifo vide
Cons s'endort
Producteur a ecrit 18
[2] -> 18
Prod reveille cons
Consommateur a lu 18
Fifo vide
Cons s'endort
Producteur a ecrit 19
[3] -> 19
Prod reveille cons
------> FIN producteur
Consommateur a lu 19
Fifo vide
Cons s'endort
Plus rien \0xef\0xbf\0xbd ordonnancer.
Sortie du noyau
 
Activations tache 0 : 3
Activations tache 1 : 897
Activations tache 2 : 102
Activations tache 3 : 0
Activations tache 4 : 0
Activations tache 5 : 0
Activations tache 6 : 0
Activations tache 7 : 0
 
avec
#define NB_LOOPS    20
#define PROD_WAIT    1000000
#define CONS_WAIT    100000
 ```
 
 
 ```
Test noyau MUTEX
------> EXEC tache A
------> DEBUT producteur
------> DEBUT consommateur
Cons s'endort
Producteur a ecrit 0
[0] -> 0
Prod reveille cons
Consommateur a lu 0
Fifo vide
Cons s'endort
Producteur a ecrit 1
[1] -> 1
Prod reveille cons
Consommateur a lu 1
Fifo vide
Cons s'endort
Producteur a ecrit 2
[2] -> 2
Prod reveille cons
Consommateur a lu 2
Fifo vide
Cons s'endort
Producteur a ecrit 3
[3] -> 3
Prod reveille cons
Consommateur a lu 3
Fifo vide
Cons s'endort
Producteur a ecrit 4
[4] -> 4
Prod reveille cons
Consommateur a lu 4
Fifo vide
Cons s'endort
Producteur a ecrit 5
[5] -> 5
Prod reveille cons
Consommateur a lu 5
Fifo vide
Cons s'endort
Producteur a ecrit 6
[6] -> 6
Prod reveille cons
Consommateur a lu 6
Fifo vide
Cons s'endort
Producteur a ecrit 7
[7] -> 7
Prod reveille cons
Consommateur a lu 7
Fifo vide
Cons s'endort
Producteur a ecrit 8
[0] -> 8
Prod reveille cons
Consommateur a lu 8
Fifo vide
Cons s'endort
Producteur a ecrit 9
[1] -> 9
Prod reveille cons
Consommateur a lu 9
Fifo vide
Cons s'endort
Producteur a ecrit 10
[2] -> 10
Prod reveille cons
Consommateur a lu 10
Fifo vide
Cons s'endort
Producteur a ecrit 11
[3] -> 11
Prod reveille cons
Consommateur a lu 11
Fifo vide
Cons s'endort
Producteur a ecrit 12
[4] -> 12
Prod reveille cons
Consommateur a lu 12
Fifo vide
Cons s'endort
Producteur a ecrit 13
[5] -> 13
Prod reveille cons
Consommateur a lu 13
Fifo vide
Cons s'endort
Producteur a ecrit 14
[6] -> 14
Prod reveille cons
Consommateur a lu 14
Fifo vide
Cons s'endort
Producteur a ecrit 15
[7] -> 15
Prod reveille cons
Consommateur a lu 15
Fifo vide
Cons s'endort
Producteur a ecrit 16
[0] -> 16
Prod reveille cons
Consommateur a lu 16
Fifo vide
Cons s'endort
Producteur a ecrit 17
[1] -> 17
Prod reveille cons
Consommateur a lu 17
Fifo vide
Cons s'endort
Producteur a ecrit 18
[2] -> 18
Prod reveille cons
Consommateur a lu 18
Fifo vide
Cons s'endort
Producteur a ecrit 19
[3] -> 19
Prod reveille cons
------> FIN producteur
Consommateur a lu 19
Fifo vide
Cons s'endort
Plus rien \0xef\0xbf\0xbd ordonnancer.
Sortie du noyau
 
Activations tache 0 : 3
Activations tache 1 : 121
Activations tache 2 : 101
Activations tache 3 : 0
Activations tache 4 : 0
Activations tache 5 : 0
Activations tache 6 : 0
Activations tache 7 : 0
 
avec
#define NB_LOOPS    20
#define PROD_WAIT    100000
#define CONS_WAIT    100000
```

Avec le même temps de traitement pour les deux tâches,
```c

```

### Partie 4 :sémaphores

*Questions
1 - On vous demande d’ajouter au mini-noyau temps réel la gestion de sémaphores à compte. Vous
placerez dans un fichier SEM.C les structures de données permettant la gestion des sémaphores ainsi que
les diverses fonctions d’accès (primitives), et dans SEM.H les déclarations permettant d’en faire usage
dans un programme utilisateur.
Structure de données d’un sémaphore :
```
typedef struct {
    FIFO file ;
    /* File circulaire des tâches en attente */
    short valeur ; /* compteur du sémaphore e(s) */
} SEMAPHORE ;
```
*

*Données globales :
Les sémaphores sont stockés dans un tableau global de taille MAX_SEM, chaque élément contenant un
sémaphore distinct. Un élément est considéré vide si le compteur d’éléments de la file associée au
sémaphore est négatif.

`SEMAPHORE _sem[MAX_SEM] ;`

Primitives d’accès :
    `void s_init( void )` initialise le tableau des sémaphores _sem.
    `ushort s_cree( short v )` crée un sémaphore dans un emplacement libre, retourne le numéro d’index dans _sem du sémaphore créé. La valeur initiale du compteur est v.
    `void s_close( ushort n )` supprime le sémaphore n.
    `void s_wait( ushort n )` Implémente P(s). Tente de prendre le sémaphore n.
    `void s_signal ( ushort n )` Implémente V(s). Libère le semaphore n.
    
2 – En utilisant les primitives que vous venez de définir, reprendre le modèle producteur/consommateur et
résoudre les problèmes d’exclusion mutuelle et de synchronisation entre le producteur et le consommateur
en utilisant exclusivement des sémaphores. Vérifiez le fonctionnement avec plusieurs producteurs et
consommateurs.*
