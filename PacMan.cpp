#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "GrilleSDL.h"
#include "Ressources.h"

// Dimensions de la grille de jeu
#define NB_LIGNE 21
#define NB_COLONNE 17

// Macros utilisees dans le tableau tab
#define VIDE         0
#define MUR          1
#define PACMAN       -2
#define PACGOM       -3
#define SUPERPACGOM  -4
#define BONUS        -5

// Macro tid Threads
#define NBRTHREADS 	7

#define THEVENT 	0
#define THPACMAN 	1
#define THPACGOM 	2
#define THSCORE		3
#define THBONUS		4
#define THCPTFANTOME	5
#define THVIES		6


// Autres macros
#define LENTREE 15
#define CENTREE 8
#define FANTOMEMAX 8

struct fantome
{
int L ;
int C ;
int couleur ;
int cache ;
};
typedef struct fantome S_FANTOME ;


int tab[NB_LIGNE][NB_COLONNE]
={ {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
   {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1},
   {1,0,1,1,0,1,1,0,1,0,1,1,0,1,1,0,1},
   {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
   {1,0,1,1,0,1,0,1,1,1,0,1,0,1,1,0,1},
   {1,0,0,0,0,1,0,0,1,0,0,1,0,0,0,0,1},
   {1,1,1,1,0,1,1,0,1,0,1,1,0,1,1,1,1},
   {1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,1,1},
   {1,1,1,1,0,1,0,1,0,1,0,1,0,1,1,1,1},
   {0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0},
   {1,1,1,1,0,1,0,1,1,1,0,1,0,1,1,1,1},
   {1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,1,1},
   {1,1,1,1,0,1,0,1,1,1,0,1,0,1,1,1,1},
   {1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1},
   {1,0,1,1,0,1,1,0,1,0,1,1,0,1,1,0,1},
   {1,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,1},
   {1,1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,1},
   {1,0,0,0,0,1,0,0,1,0,0,1,0,0,0,0,1},
   {1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,0,1},
   {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
   {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}};

pthread_mutex_t accesTab, accesGraph,mutexNbPacGom, mutexScore,mutexNbFantomes;
pthread_cond_t condNbPacGom, condScore, condNbFantomes;

pthread_t ThreadTab[NBRTHREADS];
int cpt = 0;//nbrThreads actuel

pthread_t ThreadFantome[FANTOMEMAX];
int nbrFantome = 0;
int mode = 1;

void afficheNbPacGom();
void afficheScore();
void Arret(int);
void Attente(int milli);
void calculNewCase(int*,int*,int);
void cleanUp();
void creerFantome(int);
void DessineGrilleBase();
int fantomeIA(int, int);
void mask();
int modifFantome(int, int, int, int, int, int);
void modifPacMan(int, int, int);
void remplirCasePacGom(int,int,int);
void remplirPacGom();
void routineEvent();
void setUp();
void situationCritique(char);
void spawnFantome(int,int);
void spawnPacMan();
void threadBonus();
void threadCompteurFantomes();
void threadFantome(S_FANTOME*);
void threadPacMan();
void threadPacGom();
void threadScore();
void threadVies();

int dir = GAUCHE;
int nbrPacGom = 0;
int delay = 300;

int score=0;
bool MAJscore=0;

int nbRouge=0, nbVert=0, nbMauve=0, nbOrange=0;

int nbrVies = 3;

///////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char* argv[])
{
	void *val;

	//crée la fenêtre, la remplie de sprite et lance les threads(pacman)
 	setUp();

	//on attends la fin de event
	if(pthread_join(ThreadTab[THEVENT], &val) == 0)
		printf("main Thread : j'ai recu la fin de Event\n");
  	
	//ferme la fenêtre graphique
  	cleanUp();

  	exit(0);
}

//*********************************************************************************************
void Attente(int milli)
{
  	struct timespec del;
  	del.tv_sec = milli/1000;
  	del.tv_nsec = (milli%1000)*1000000;
  	nanosleep(&del,NULL);
}

//*********************************************************************************************
void DessineGrilleBase()
{
	for (int l=0 ; l<NB_LIGNE ; l++)
		for (int c=0 ; c<NB_COLONNE ; c++)
	    	{
	      		if (tab[l][c] == VIDE) 
				EffaceCarre(l,c);
			if (tab[l][c] == MUR) 
				DessineMur(l,c);
		}
}

void threadPacMan()
{
	int col = 8,ligne = 15;
	int newCol = 8, newLigne = 15;


	spawnPacMan();

	Attente(300);

	printf("pacman entre dans mask\n");
	mask();
	
	while(1)
	{
		calculNewCase(&newLigne, &newCol,dir);		
		if(newCol >= 0 && newLigne >=0 && newCol <=17 && newLigne <= 21)//dans les limites
		{		
			if(tab[newLigne][newCol] != MUR)//pas un mur
			{
				ligne = newLigne; col = newCol;//on applique la modif
				modifPacMan(ligne, col, dir);
			}
			else
			{
				newCol = col;
				newLigne = ligne;
			}
		}
		else
		{
			newCol = col;
			newLigne = ligne;
		}
	situationCritique('c');
	Attente(delay);
	situationCritique('k');
	}

	pthread_exit(NULL);
}

void modifPacMan(int l, int c, int dir)
{
	int valCase;

	situationCritique('c');
	
	//effacer ancien pacman
	switch(dir)
	{
		case GAUCHE : 
			{
				pthread_mutex_lock(&accesGraph);
				EffaceCarre(l,c+1);
				pthread_mutex_unlock(&accesGraph);

				pthread_mutex_lock(&accesTab);
				tab[l][c+1] = VIDE; 
				pthread_mutex_unlock(&accesTab);
				break;
			}
		case DROITE : 
			{
				pthread_mutex_lock(&accesGraph);
				EffaceCarre(l,c-1);
				pthread_mutex_unlock(&accesGraph); 

				pthread_mutex_lock(&accesTab);
				tab[l][c-1] = VIDE; 
				pthread_mutex_unlock(&accesTab);
				break;
			}
		
		case HAUT :
			{
				pthread_mutex_lock(&accesGraph);
				EffaceCarre(l+1,c);
				pthread_mutex_unlock(&accesGraph);

				pthread_mutex_lock(&accesTab);
				tab[l+1][c] = VIDE; 
				pthread_mutex_unlock(&accesTab);				
				break;
			}

		case BAS :
			{
				pthread_mutex_lock(&accesGraph);
				EffaceCarre(l-1,c); 
				pthread_mutex_unlock(&accesGraph);

				pthread_mutex_lock(&accesTab);
				tab[l-1][c] = VIDE;
				pthread_mutex_unlock(&accesTab);
				break;
			}
	}

	valCase = tab[l][c];

	if(valCase < 1)//pas fantome
	{
		pthread_mutex_lock(&accesTab);
		tab[l][c] = PACMAN;
		pthread_mutex_unlock(&accesTab);	

		pthread_mutex_lock(&accesGraph);
		DessinePacMan(l,c,dir);
		pthread_mutex_unlock(&accesGraph);

		//calcul score
		if(valCase == PACGOM || valCase == SUPERPACGOM)
		{
			pthread_mutex_lock(&mutexNbPacGom);
			nbrPacGom--;
			pthread_mutex_unlock(&mutexNbPacGom);
			pthread_cond_signal(&condNbPacGom);

			pthread_mutex_lock(&mutexScore);
		
			if(valCase == PACGOM)
				score+=1;
			else
				score+=5;
			MAJscore=1;

			pthread_mutex_unlock(&mutexScore);
			pthread_cond_signal(&condScore);
		}
		else
			if(valCase == BONUS)
			{
				pthread_mutex_lock(&mutexScore);
				score+=30;
				MAJscore=1;
				pthread_mutex_unlock(&mutexScore);
				pthread_cond_signal(&condScore);
			}
	}
	else
	{
		pthread_exit(NULL);
	}
	situationCritique('k');
}

void spawnPacMan()
{
	pthread_mutex_lock(&accesGraph);
	DessinePacMan(15,8,GAUCHE);
	pthread_mutex_unlock(&accesGraph);
	
	pthread_mutex_lock(&accesTab);
	tab[15][8] = PACMAN;
	pthread_mutex_unlock(&accesTab);
}

void calculNewCase(int* ligne, int* col, int dir)
{
	switch(dir)
	{
		case GAUCHE : (*col)--; break;
		case DROITE : (*col)++; break;
		case HAUT : (*ligne)--; break;
		case BAS : (*ligne)++; break;
	}
}


void PM_Haut(int sig)
{
	dir = HAUT;
}

void PM_Bas(int sig)
{
	dir = BAS;
}

void PM_Gauche(int sig)
{
	dir = GAUCHE;
}

void PM_Droite(int sig)
{
	dir = DROITE;
}


void routineEvent()
{
  	EVENT_GRILLE_SDL event;
	mask();

	char ok = 0;
  	while(!ok)
  	{
    		event = ReadEvent();
    		if (event.type == CROIX) ok = 1;
    		if (event.type == CLAVIER)
    		{
      			switch(event.touche)
      			{
        			case 'q' : ok = 1; break;
        			case KEY_RIGHT : pthread_kill(ThreadTab[THPACMAN], SIGHUP); break;
        			case KEY_LEFT : pthread_kill(ThreadTab[THPACMAN], SIGINT); break;
				case KEY_UP : pthread_kill(ThreadTab[THPACMAN], SIGUSR1); break;
				case KEY_DOWN : pthread_kill(ThreadTab[THPACMAN], SIGUSR2); break;
      			}
    		}
  	}

	//on ferme les threads(on passe 0 car 0 = threadEvent)
	for(int i=1;i<NBRTHREADS;i++)
	{
		printf("event : kill %d/%d %d\n", i, NBRTHREADS-1, ThreadTab[i]);
		pthread_cancel(ThreadTab[i]);
		cpt--;
	}

	cpt--;
	pthread_exit(NULL);
}

void mask()
{
	sigset_t masque;

	sigemptyset(&masque);
	sigaddset(&masque, SIGINT);
	sigaddset(&masque, SIGHUP);
	sigaddset(&masque, SIGUSR1);
	sigaddset(&masque, SIGUSR2);
	
	sigprocmask(SIG_BLOCK, &masque, NULL);//on bloque les signaux du joueur
}

void setUp()
{
	//armement des signaux
	struct sigaction Act;
	Act.sa_flags = 0;
	sigemptyset(&Act.sa_mask);

	Act.sa_handler = PM_Haut;
	sigaction(SIGUSR1, &Act, NULL);

	Act.sa_handler = PM_Bas;
	sigaction(SIGUSR2, &Act, NULL);

	Act.sa_handler = PM_Gauche;
	sigaction(SIGINT, &Act, NULL);

	Act.sa_handler = PM_Droite;
	sigaction(SIGHUP, &Act, NULL);


	mask();


  	// Ouverture de la fenetre graphique
  	printf("(MAIN %d) Ouverture de la fenetre graphique\n",pthread_self()); fflush(stdout);
  	if (OuvertureFenetreGraphique() < 0)
  	{
    		printf("Erreur de OuvrirGrilleSDL\n");
    		fflush(stdout);
    		exit(1);
  	}

  	DessineGrilleBase();



	//init mutex
	pthread_mutex_init(&accesTab,NULL);
	pthread_mutex_init(&accesGraph,NULL);
	pthread_mutex_init(&mutexNbPacGom,NULL);
	pthread_mutex_init(&mutexScore,NULL);
	
	pthread_cond_init(&condNbPacGom,NULL);
	pthread_cond_init(&condScore,NULL);




	//lancement des threads
	//Event
	if(pthread_create(&ThreadTab[THEVENT], NULL, (void*(*)(void*))routineEvent, NULL) == -1)
		printf("erreur de creation Event\n");
	else
	{
		printf("thread Event cree\n");
		cpt++;
	}	

	//PacGom
	if(pthread_create(&ThreadTab[THPACGOM], NULL, (void*(*)(void*))threadPacGom, NULL) == -1)
		printf("erreur de creation Event\n");
	else
	{
		printf("thread PacGom cree\n");
		cpt++;
	}

	//Score
	if(pthread_create(&ThreadTab[THSCORE], NULL, (void*(*)(void*))threadScore, NULL) == -1)
		printf("erreur de creation Event\n");
	else
	{
		printf("thread Score cree\n");
		cpt++;
	}

	//bonus
	if(pthread_create(&ThreadTab[THBONUS], NULL, (void*(*)(void*))threadBonus, NULL) == -1)
		printf("erreur de creation Event\n");
	else
	{
		printf("thread Bonus cree\n");
		cpt++;
	}

	//compteur fantome
	if(pthread_create(&ThreadTab[THCPTFANTOME], NULL, (void*(*)(void*))threadCompteurFantomes, NULL) == -1)
		printf("erreur de creation Event\n");
	else
	{
		printf("thread Score cree\n");
		cpt++;
	}

	//Vies
	if(pthread_create(&ThreadTab[THVIES], NULL, (void*(*)(void*))threadVies, NULL) == -1)
		printf("erreur de creation Event\n");
	else
	{
		printf("thread Vies cree\n");
		cpt++;
	}
}

void cleanUp()
{
	// Fermeture de la fenetre
  	printf("(MAIN %d) Fermeture de la fenetre graphique...",pthread_self()); fflush(stdout);
  	FermetureFenetreGraphique();
  	printf("OK\n"); fflush(stdout);
}

void situationCritique(char onOff)
{
	sigset_t masque;
	sigemptyset(&masque);
	sigaddset(&masque, SIGINT);
	sigaddset(&masque, SIGHUP);
	sigaddset(&masque, SIGUSR1);
	sigaddset(&masque, SIGUSR2);
	
	if(onOff == 'c')//situation critique
		sigprocmask(SIG_BLOCK, &masque, NULL);//on bloque les signaux du joueur
	else
		sigprocmask(SIG_UNBLOCK, &masque, NULL);//on les débloque
}

void threadPacGom()
{
	int niveau = 0;
	mask();

	while(1)
	{
	niveau++;
	DessineChiffre(14,22,niveau);

	remplirPacGom();
	afficheNbPacGom();
	
	pthread_mutex_lock(&mutexNbPacGom);
	while(nbrPacGom>0)
	{
		pthread_cond_wait(&condNbPacGom, &mutexNbPacGom);
		afficheNbPacGom();
	}
	pthread_mutex_unlock(&mutexNbPacGom);

	delay /= 2;
	}
}

void afficheNbPacGom()
{
	pthread_mutex_lock(&accesGraph);
	DessineChiffre(12,24,(nbrPacGom % 10));//unité
	DessineChiffre(12,23,((nbrPacGom / 10) % 10));//dizaine
	DessineChiffre(12,22,(nbrPacGom / 100));//centaine
	pthread_mutex_unlock(&accesGraph);
}

void remplirPacGom()
{
	for(int x=0; x<NB_LIGNE;x++)
	{
		for(int y=0; y<NB_COLONNE;y++)
		{
			if( (x==15 && y==8) || (y==8 && (x==8 || x==9)) );
			else
				if( (x==2 && (y==1 || y==15)) || (x==15 && (y==1 || y==15)) )
					remplirCasePacGom(x,y,SUPERPACGOM);
				else
					remplirCasePacGom(x,y,PACGOM);
		}
	}
}

void remplirCasePacGom(int l, int c, int val)
{
	pthread_mutex_lock(&accesTab);
	if(tab[l][c] == VIDE)
	{
		if(val == PACGOM)
		{
			tab[l][c] = PACGOM;
			pthread_mutex_unlock(&accesTab);
		
			pthread_mutex_lock(&accesGraph);
			DessinePacGom(l,c);
			pthread_mutex_unlock(&accesGraph);
			pthread_mutex_lock(&mutexNbPacGom);
			nbrPacGom++;
			pthread_mutex_unlock(&mutexNbPacGom);
		}
		else
		{
			tab[l][c] = SUPERPACGOM;
			pthread_mutex_unlock(&accesTab);
	
			pthread_mutex_lock(&accesGraph);
			DessineSuperPacGom(l,c);
			pthread_mutex_unlock(&accesGraph);

			pthread_mutex_lock(&mutexNbPacGom);
			nbrPacGom++;
			pthread_mutex_unlock(&mutexNbPacGom);
		}
	}
	else
		pthread_mutex_unlock(&accesTab);
}

void threadScore()
{
	pthread_mutex_lock(&mutexScore);
	mask();
	while(1)
	{
		afficheScore();
		pthread_cond_wait(&condScore, &mutexScore);
		MAJscore=0;
	}
}

void afficheScore()
{
	DessineChiffre(16,22,(score/1000)%10);//mille
	DessineChiffre(16,23,(score/100)%10);//cent
	DessineChiffre(16,24,(score/10)%10);//dix
	DessineChiffre(16,25,score%10);//unité
}

void Arret(int sig)
{
	pthread_exit(NULL);
}

void threadBonus()
{
	int timer;
	int x,y,trouve;

	mask();

	srand(time(NULL));
	while(1)
	{
		x=0;
		trouve=0;
	
		//attente random 10->20sec
		timer = (rand() % 11) + 10;
		Attente(timer * 1000);

		//recherche case libre
		while(x<NB_LIGNE && trouve == 0)
		{
		y=0;
			while(y<NB_COLONNE && trouve == 0)
			{
				if(tab[x][y] == VIDE)
					trouve = 1;
				else
					y++;
			}
		x++;
		}

		//ajout bonus dans le tab et le graph
		if(trouve == 1)
		{
			x--;
			
			pthread_mutex_lock(&accesTab);
			tab[x][y] = BONUS;
			pthread_mutex_unlock(&accesTab);

			pthread_mutex_lock(&accesGraph);
			DessineBonus(x,y);
			pthread_mutex_unlock(&accesGraph);
		}

		//attente 10 sec
		Attente(10000);

		//test si bonus existe encore
		if(tab[x][y] == BONUS)
		{
			pthread_mutex_lock(&accesTab);
			tab[x][y] = VIDE;
			pthread_mutex_unlock(&accesTab);

			pthread_mutex_lock(&accesGraph);
			EffaceCarre(x,y);
			pthread_mutex_unlock(&accesGraph);
			
		}
	}
}

void threadCompteurFantomes()
{
	mask();
	while(1)
	{
		while(nbRouge==2 && nbMauve==2 && nbOrange==2 && nbVert==2)
		{
			Attente(1000);
		};
		if(nbRouge!=2)
			creerFantome(ROUGE);
		else
			if(nbMauve!=2)
				creerFantome(MAUVE);
		else
			if(nbOrange!=2)
				creerFantome(ORANGE);
		else
			creerFantome(VERT);

		Attente(1000);
	};
}

void creerFantome(int couleur)
{
	S_FANTOME *fantome;

	fantome = (S_FANTOME*) malloc(sizeof(S_FANTOME));

	fantome->L = 9;
	fantome->C = 8;
	fantome->cache = 0;
	fantome->couleur = couleur;

	if(pthread_create(&ThreadFantome[nbrFantome], NULL, (void*(*)(void*))threadFantome, fantome) == -1)
		printf("erreur de creation fantome\n");
	else
		printf("creation d'un fantome %d\n",couleur);

	nbrFantome++;

	switch(couleur)
	{
		case ROUGE : nbRouge++; break;
		case ORANGE : nbOrange++; break;
		case VERT : nbVert++; break;
		default : nbMauve++;
	}
}

void threadFantome(S_FANTOME* ptrFantome)
{
	int tid = pthread_self();
	int direction = HAUT;
	int newCol=8, newLigne=9;
	int valCase = VIDE;
	pthread_key_t cle;
	S_FANTOME fantome = *ptrFantome;
	free(ptrFantome);	

	pthread_key_create(&cle, NULL);
	pthread_setspecific(cle, (void*) &fantome);

	spawnFantome(fantome.couleur, direction);

	mask();

	while(1)
	{
		calculNewCase(&newLigne, &newCol, direction);
		if(newCol >= 0 && newLigne >=0 && newCol <=17 && newLigne <= 21)//dans les limites
		{		
			if(tab[newLigne][newCol] < 1)//pas d'obstacle (mur=1/autre fantome=entier positif)
			{
				fantome.L = newLigne; fantome.C = newCol;//on applique la modif
				valCase = modifFantome(fantome.L, fantome.C, direction, tid, fantome.couleur, valCase);
			}
			else//obstacle rencontre
			{
				newCol = fantome.C;
				newLigne = fantome.L;
				direction = fantomeIA(fantome.L, fantome.C);
			}
		}
		else
		{
			newCol = fantome.C;
			newLigne = fantome.L;
			direction = fantomeIA(fantome.L, fantome.C);
		}
	Attente((delay*5)/3);
	}
}

void spawnFantome(int couleur, int dir)
{
	while(tab[9][8]!=VIDE)
		Attente(500);

	pthread_mutex_lock(&accesGraph);
	DessineFantome(9,8,couleur,dir);
	pthread_mutex_unlock(&accesGraph);
	
	pthread_mutex_lock(&accesTab);
	tab[9][8] = pthread_self();
	pthread_mutex_unlock(&accesTab);
}

int modifFantome(int l, int c, int dir, int tid, int couleur, int remplacer)
{
	int valCase;

	pthread_mutex_lock(&accesTab);
	valCase = tab[l][c];
	tab[l][c] = tid;
	pthread_mutex_unlock(&accesTab);

	pthread_mutex_lock(&accesGraph);
	DessineFantome(l,c,couleur,dir);
	pthread_mutex_unlock(&accesGraph);

	if(valCase == PACMAN)
		pthread_cancel(ThreadTab[THPACMAN]);

	switch(dir)
	{
		case GAUCHE : 
			{
				pthread_mutex_lock(&accesGraph);
				switch(remplacer)
				{
					case PACGOM : DessinePacGom(l, c+1); break;
					case SUPERPACGOM : DessineSuperPacGom(l, c+1); break;
					case BONUS : DessineBonus(l, c+1); break;
					default : EffaceCarre(l, c+1);
				}
				pthread_mutex_unlock(&accesGraph);

				pthread_mutex_lock(&accesTab);
				if(remplacer == PACMAN)
					tab[l][c+1] = VIDE;
				else
					tab[l][c+1] = remplacer; 
				pthread_mutex_unlock(&accesTab);
				break;
			}
		case DROITE : 
			{
				pthread_mutex_lock(&accesGraph);
				switch(remplacer)
				{
					case PACGOM : DessinePacGom(l, c-1); break;
					case SUPERPACGOM : DessineSuperPacGom(l, c-1); break;
					case BONUS : DessineBonus(l, c-1); break;
					default : EffaceCarre(l, c-1);
				}
				pthread_mutex_unlock(&accesGraph); 

				pthread_mutex_lock(&accesTab);
				if(remplacer == PACMAN)
					tab[l][c-1] = VIDE;
				else
					tab[l][c-1] = remplacer; 
				pthread_mutex_unlock(&accesTab);
				break;
			}
		
		case HAUT :
			{
				pthread_mutex_lock(&accesGraph);
				switch(remplacer)
				{
					case PACGOM : DessinePacGom(l+1, c); break;
					case SUPERPACGOM : DessineSuperPacGom(l+1, c); break;
					case BONUS : DessineBonus(l+1, c); break;
					default : EffaceCarre(l+1, c); 
				}
				pthread_mutex_unlock(&accesGraph);

				pthread_mutex_lock(&accesTab);
				if(remplacer == PACMAN)
					tab[l+1][c] = VIDE;
				else
					tab[l+1][c] = remplacer; 
				pthread_mutex_unlock(&accesTab);				
				break;
			}

		case BAS :
			{
				pthread_mutex_lock(&accesGraph);
				switch(remplacer)
				{
					case PACGOM : DessinePacGom(l-1, c); break;
					case SUPERPACGOM : DessineSuperPacGom(l-1, c); break;
					case BONUS : DessineBonus(l-1, c); break;
					default : EffaceCarre(l-1, c);
				}
				pthread_mutex_unlock(&accesGraph);

				pthread_mutex_lock(&accesTab);
				if(remplacer == PACMAN)
					tab[l-1][c] = VIDE;
				else
					tab[l-1][c] = remplacer; 
				pthread_mutex_unlock(&accesTab);
				break;
			}
	}

	return valCase;
}

int fantomeIA(int l, int c)
{
	int Haut=-1, Bas=-1, Gauche=-1, Droite=-1;
	int choix;

	

	srand(time(NULL));

	do
	{
		//pas d'obstacle
		if(tab[l-1][c] < 1)
			Haut = 1;
		if(tab[l+1][c] < 1)
			Bas = 1;
		if(tab[l][c+1] < 1)
			Droite = 1;
		if(tab[l][c-1] < 1)
			Gauche = 1;

		choix = rand()%4;//0->3
		switch(choix)
		{
			case 0 : 
				{
					if(Haut!=-1) 
						return HAUT;
					break;
				}
			case 1 :
				{
					if(Bas!=-1) 
						return BAS;
					break;
				}
			case 2 : 
				{
					if(Gauche!=-1) 
						return GAUCHE;
					break;
				}

			case 3 :
				{
					if(Droite!=-1) 
						return DROITE;
					break;
				}

		}
	}while(1);
	
}

void threadVies()
{
	mask();
	while(nbrVies > 0)
	{
		//création du thread PacMan
		if(pthread_create(&ThreadTab[THPACMAN], NULL, (void*(*)(void*))threadPacMan, NULL) == -1)
			printf("erreur creation pacMan\n");
		else
		{
			printf("thread pacman cree (reste %d vies)\n", nbrVies);
			cpt++;
		}

		//affichage nbr de vies
		DessineChiffre(18,22,nbrVies);

		//attente respawn
		pthread_join(ThreadTab[THPACMAN], NULL);
		nbrVies--;
		Attente(1000);
	}
	printf("game over\n");
	
	pthread_mutex_lock(&accesGraph);
	DessineGameOver(9,4);
}
