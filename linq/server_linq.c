#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include <netdb.h>
#include <arpa/inet.h>

struct _client
{
        char ipAddress[40];
        int port;
        char name[40];
	char words[2][40];
	int choix[2];
	int score;
	int role;
        int cible; // Compte le nombre de personne qui suspectent ce joueur d'être un espion.
} tcpClients[5];

int nbClients;
int fsmServer;
int deck[5]={0,0,0,1,1};
char *mpts[]={"bas","muse","boulet","spectre","patron","moulin","piano","oseille","billet","Rome"};
int joueurCourant;
int nbReponses;
int idEspion[2];
int quit;
char *secretWord;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void melangerDeck()
{
        int i;
        int index1,index2,tmp;

        for (i=0;i<1000;i++)
        {
                index1=rand()%5;
                index2=rand()%5;

                tmp=deck[index1];
                deck[index1]=deck[index2];
                deck[index2]=tmp;
        }
}

void printDeck()
{
        int i,j; // j ?

        for (i=0;i<5;i++)
                printf("%d:%d\n",i,deck[i]);
}

void printClients()
{
        int i;

        for (i=0;i<nbClients;i++)
                printf("%d: %s %5.5d %s %s %s %d %d\n",i,tcpClients[i].ipAddress, tcpClients[i].port, tcpClients[i].name, tcpClients[i].words[0], tcpClients[i].words[1],tcpClients[i].score, tcpClients[i].role);
}

int findClientByName(char *name)
{
        int i;

        for (i=0;i<nbClients;i++)
                if (strcmp(tcpClients[i].name,name)==0)
                        return i;
        return -1;
}

void sendMessageToClient(char *clientip,int clientport,char *mess)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    server = gethostbyname(clientip);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(clientport);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        {
                printf("ERROR connecting\n");
                exit(1);
        }

        sprintf(buffer,"%s\n",mess);
        n = write(sockfd,buffer,strlen(buffer));

    close(sockfd);
}

void broadcastMessage(char *mess)
{
        int i;
        printf("%s\n", mess); // TEST
        for (i=0;i<nbClients;i++)
                sendMessageToClient(tcpClients[i].ipAddress,
                        tcpClients[i].port,
                        mess);
}

/// Fonctions réalisées par les étudiants pour réaliser le projet.

//On affecte simplement les rôles.
void affecterRoles()
{
        int k=0;
        for(int i = 0; i<5; ++i)
        {
                tcpClients[i].role = deck[i];
                if(deck[i] == 1)
                {
                        idEspion[k] = i;
                        k++; // Y'a moyen de fusionner ces deux lignes mais c'est pas lisible et en plus je sais plus si c'est k++ ou ++k
                }
        }
}

void broadcastRoles()
{       
        char buffer[256]; // R pour rôle !
        
        sprintf(buffer, "R %d %d %d %d %d", tcpClients[0].role, tcpClients[1].role, tcpClients[2].role, tcpClients[3].role, tcpClients[4].role);

        broadcastMessage(buffer);
}


int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
	int i;

        char com;
        char clientIpAddress[256], clientName[256];
        int clientPort;
        int id;
        char reply[256];


     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);

	printDeck();
	melangerDeck();
	printDeck();
	joueurCourant=0;

	for (i=0;i<5;i++)
	{
        	strcpy(tcpClients[i].ipAddress,"localhost");
        	tcpClients[i].port=-1;
        	strcpy(tcpClients[i].name,"-");
        	strcpy(tcpClients[i].words[0],"-");
        	strcpy(tcpClients[i].words[1],"-");
	}
     quit = 0;
     while (quit == 0)
     {    
     	newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     	if (newsockfd < 0) 
          	error("ERROR on accept");

     	bzero(buffer,256);
     	n = read(newsockfd,buffer,255);
     	if (n < 0) 
		error("ERROR reading from socket");

        printf("Received packet from %s:%d\nData: [%s]\n\n",
                inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), buffer);

        if (fsmServer==0)
        {
        	switch (buffer[0])
        	{
                	case 'C':
                        	sscanf(buffer,"%c %s %d %s", &com, clientIpAddress, &clientPort, clientName);
                        	printf("COM=%c ipAddress=%s port=%d name=%s\n",com, clientIpAddress, clientPort, clientName);

                        	// fsmServer==0 alors j'attends les connexions de tous les joueurs
                                strcpy(tcpClients[nbClients].ipAddress,clientIpAddress);
                                tcpClients[nbClients].port=clientPort;
                                strcpy(tcpClients[nbClients].name,clientName);
                                nbClients++;

                                printClients();

				// rechercher l'id du joueur qui vient de se connecter

                                id=findClientByName(clientName);
                                printf("id=%d\n",id);

				// lui envoyer un message personnel pour lui communiquer son id

                                sprintf(reply,"I %d",id);
                                sendMessageToClient(tcpClients[id].ipAddress,
                                       tcpClients[id].port,
                                       reply);

				// Envoyer un message broadcast pour communiquer a tout le monde la liste des joueurs actuellement
				// connectes

                                sprintf(reply,"L %s %s %s %s %s", tcpClients[0].name, tcpClients[1].name, tcpClients[2].name, tcpClients[3].name,tcpClients[4].name);
                                broadcastMessage(reply);

				// Si le nombre de joueurs atteint 5, alors on peut lancer le jeu

                                if (nbClients==5)
				{
                                        fsmServer=1;
					
					melangerDeck();
                                        for (int i = 0; i<5; ++i) 
                                        {
                                                tcpClients[i].score = 0;
                                                tcpClients[i].cible = 0;
                                        }
					affecterRoles();
					broadcastRoles();

					secretWord = mpts[rand()%10];           //tirer un mot au hasard, penser à changer le 10 si on ajoute/enlève des mots
                                        sprintf(reply, "W %s", secretWord);
                                        for(int i=0; i<5; ++i)                  //pour les espions, envoyer le mot
                                        {
                                                if (tcpClients[i].role == 1)    // if(tcpClients[i].role) suffirait mais c'est plus lisible ainsi
                                                {
                                                        sendMessageToClient(tcpClients[i].ipAddress,
                                                        tcpClients[i].port,
                                                        reply);
                                                }
                                        }
					joueurCourant=0;
					nbReponses=0;
                                        sprintf(reply, "T");
                                        sendMessageToClient(tcpClients[joueurCourant].ipAddress,
                                                        tcpClients[joueurCourant].port,
                                                        reply);
					
				}
				break;

                        default:
                                break;
                }
	}
	else if (fsmServer==1)
	{
		switch (buffer[0])
		{
                	case 'P':
				{
					if (strcmp(tcpClients[joueurCourant].words[0], "-") == 0)
					   sscanf(buffer+2, "%s", tcpClients[joueurCourant].words[0]);
                                        else
                                           sscanf(buffer+2, "%s", tcpClients[joueurCourant].words[1]);

                                        sprintf(reply,"M %s %s %s %s %s %s %s %s %s %s",
                                                 tcpClients[0].words[0], tcpClients[0].words[1], 
                                                 tcpClients[1].words[0], tcpClients[1].words[1], 
                                                 tcpClients[2].words[0], tcpClients[2].words[1], 
                                                 tcpClients[3].words[0], tcpClients[3].words[1], 
                                                 tcpClients[4].words[0], tcpClients[4].words[1]);
                                        broadcastMessage(reply);

					nbReponses++;
					if(nbReponses==10)
                                                {
                                                        fsmServer = 2;
                                                        nbReponses = 0;
                                                }
                                        else
                                        {


                                                joueurCourant = (joueurCourant+1)%5;
                                                sprintf(reply, "T");
                                                sendMessageToClient(tcpClients[joueurCourant].ipAddress,
                                                        tcpClients[joueurCourant].port,
                                                        reply);
                                        }
                                break;
				}

                	default:
                        	break;
		}
        }
        else if (fsmServer ==2)
        {
                int joueurId;
                char motJoueur[256];
                int cibleJoueur[2];

                switch(buffer[0])
                {
                        // Message 'A' : On reçoit une réponse d'un joueur
                        case 'A':
                                {
                                        sscanf(buffer+2, "%d %d %d %s", &joueurId, &cibleJoueur[0], &cibleJoueur[1], motJoueur);

                                        if (tcpClients[joueurId].role == 1) //Dans le cas d'une réponse d'un espion :
                                        {
                                                if(tcpClients[cibleJoueur[0]].role == 1) tcpClients[joueurId].score++;
                                        }
                                        else
                                        {
                                                for(int i=0; i<2; ++i)
                                                {
                                                        if(tcpClients[cibleJoueur[i]].role == 1)
                                                        {
                                                                tcpClients[joueurId].score++;
                                                                tcpClients[cibleJoueur[i]].cible++;
                                                        }
                                                }
                                                if(strcmp(motJoueur, secretWord) == 0) tcpClients[joueurId].score+=2;
                                        }

                                        nbReponses++;
                                        if(nbReponses == 5)
                                        {
                                                for(int i=0; i<5;++i)
                                                {
                                                        if((tcpClients[i].role == 1) && (tcpClients[i].cible == 0)) tcpClients[i].score += 3;

                                                        sprintf(reply, "S %d %d %s %d",idEspion[0], idEspion[1], secretWord, tcpClients[i].score);
                                                        sendMessageToClient(tcpClients[i].ipAddress,
                                                        tcpClients[i].port,
                                                        reply);
                                                }
                                                quit = 1; // Fermeture paisible du serveur
                                        }
                                break;
                                }
                }
        }

     	close(newsockfd);
     }
     close(sockfd);
     return EXIT_SUCCESS; 
}
