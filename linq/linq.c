#include <SDL.h>        
#include <SDL_image.h>        
#include <SDL_ttf.h>        
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include<unistd.h> //pour la fonction sleep()

#define ECRAN_3_CONTRE_ESPION 3
#define ECRAN_3_ESPION 4

pthread_t thread_serveur_tcp_id;
char gbuffer[256];
char gServerIpAddress[256];
int gServerPort;
char gClientIpAddress[256];
int gClientPort;
char gName[256];
char gNames[5][256];
char gWords[5][2][256] = {{"-","-"},{"-","-"},{"-","-"},{"-","-"},{"-","-"}};
int gId;
int gRole = -1;
int goEnabled;
int connectEnabled;
int screenNumber;
char word[40];
char secretWord[40];
int cptWord;
int quit = 0;
SDL_Event event;
int mx,my;
char sendBuffer[256];
SDL_Window * window;
SDL_Renderer *renderer;

int idEspion[2];
int score;

SDL_Surface *connectbutton;
SDL_Texture *texture_connectbutton;
TTF_Font* Sans; 

volatile int synchro;

int numeroDuChoix[4] = {-1,-1,-1,-1}; //pour garder le numero du choix qu'on vient de faire dans choix[2] (premier ou second), utile pour dé-clicker un bouton
int flagBoutonAppuye[4] = {0, 0, 0, 0}; //indique l'état des quatre boutons de "vote": appuyé ou pas 
int choix[2] = {-1, -1};  //choix actuels du joueur avant validation et envoi
int flagChoixFait = 0;  //on envoit la reponse du joueur au serveur quand ce flag est levé 


void *fn_serveur_tcp(void *arg)
{
        int sockfd, newsockfd, portno; 
        socklen_t clilen;
        struct sockaddr_in serv_addr, cli_addr;
        int n;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd<0)
        {
                printf("sockfd error\n");
                exit(1);
        }

        bzero((char *) &serv_addr, sizeof(serv_addr));
        portno = gClientPort;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portno);
       if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        {
                printf("bind error\n");
                exit(1);
        }

        listen(sockfd,5);
        clilen = sizeof(cli_addr);
        while (1)
        {
                newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
                if (newsockfd < 0)
                {
                        printf("accept error\n");
                        exit(1);
                }

                bzero(gbuffer,256);
                n = read(newsockfd,gbuffer,255);
                if (n < 0)
                {
                        printf("read error\n");
                        exit(1);
                }

                synchro=1;

                while (synchro); // Permet d'empêcher qu'on écrase le buffer alors qu'il n'a pas encore été traité.

     }
}

void sendMessageToServer(char *ipAddress, int portno, char *mess)
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char sendbuffer[256]; // Pourquoi Monsieur ?! Y'a un sendBuffer[256] global et à cause de ce deuxième LOCAL mon éditeur me fait faire une erreur à chaque fois que je veux utiliser le buffer global !

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    server = gethostbyname(ipAddress);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        {
                printf("ERROR connecting\n");
                exit(1);
        }

        sprintf(sendbuffer,"%s\n",mess);
        n = write(sockfd,sendbuffer,strlen(sendbuffer));

    close(sockfd);
}

void manageEvent(SDL_Event event)
{
    switch (event.type)
    {
    case SDL_QUIT:
        quit = 1;
    break;

	case SDL_KEYDOWN:
	{
		switch(screenNumber)
		{
			case 2:  //ecran avec textbox
			{

				if(goEnabled ==1)
				{
					int car=event.key.keysym.sym;
    				printf("%d\n",event.key.keysym.sym);
    				if (car==8)
    				{
     					strcpy(word,"");
     					cptWord=0;
    				}

					else if (car == 13)   //si touche entrée
					{
                                                goEnabled =0;
                                                sprintf(sendBuffer, "P %s", word);
                                                sendMessageToServer(gServerIpAddress, gServerPort,sendBuffer);
                                                strcpy(word,""); 
                                                cptWord = 0;			
					}

    				else if ((car>=97) && (car<=122)) 
    				{
    					word[cptWord++]=car;
    					word[cptWord]='\0';
    				}
				}
			}
			break;

             case 3:  //ecran saisi du mot secret pour les contre-espions
            {
                if(gRole == 0) //si contre espion
                {
                 	int car=event.key.keysym.sym;
    				printf("%d\n",event.key.keysym.sym);
    				if (car==8)
    				{
     					strcpy(word,"");
     					cptWord=0;
    				}
				else if (((car == 13) && (flagChoixFait == 0)) && ((choix[0] != -1) && (choix[1] != -1)))  //si touche entrée
				{
                                        flagChoixFait = 1;
                                        if (cptWord == 0) strcpy(word, "-");
                                        sprintf(sendBuffer, "A %d %d %d %s", gId, choix[0], choix[1], word);
                                        sendMessageToServer(gServerIpAddress, gServerPort,sendBuffer);		
				}
    				else if ((car>=97) && (car<=122)) 
    				{
                                        word[cptWord++]=car;
                                        word[cptWord]='\0';
    				}   
                }
            }
            break;
		}
	}
	break;

  	case  SDL_MOUSEBUTTONDOWN:
   	switch(screenNumber)
   	{
    case 0:
	{
     	SDL_GetMouseState( &mx, &my );
     	if ((mx<200) && (my<50) && (connectEnabled==1))
     	{
      		sprintf(sendBuffer,"C %s %d %s", gClientIpAddress,gClientPort,gName);
      		sendMessageToServer(gServerIpAddress, gServerPort,sendBuffer);
      		connectEnabled=0;
     	} 
	}
	break;

    case 3: //phase de "vote"
	{
		//petit tableau temp pour stocker les noms des autres joueurs sans le notre, probablement à sortir de cette fonc
		char idAutresJoueurs[4]; // Je crois qu'il sert à rien ce truc mdr
		int j = 0;
		for (int i = 0; i < 5; i++)
		{
			if(i!=gId)
			{
				idAutresJoueurs[j] = i;
	    		j++; 
	    	}
	    }

	    SDL_GetMouseState( &mx, &my );
        if ((15<mx)&&(mx<(15+240)) && (300<my)&&(my<(300+80))) // rectangle de 240x80 situé à (15, 300) {15, 300, 240, 80}
        {
            if((flagBoutonAppuye[0] == 0) && ((choix[0] == -1) || (choix[1] == -1)))
            {
                flagBoutonAppuye[0] = 1; //on leve le flag du premier bouton
                if (choix[0] == -1) { choix[0] = idAutresJoueurs[0]; numeroDuChoix[0] = 0;}
                else if (choix[1] == -1) { choix[1] =  idAutresJoueurs[0]; numeroDuChoix[0] = 1;}
            }
            else if (flagBoutonAppuye[0] == 1)
            {
                choix[numeroDuChoix[0]] = -1;
                flagBoutonAppuye[0] = 0;
            }
        }

        if ((265<mx)&&(mx<(265+240)) && (300<my)&&(my<(300+80))) // rectangle de 240x80 situé à (265, 300) {265, 300, 240, 80};
        {
            if((flagBoutonAppuye[1] == 0) && ((choix[0] == -1) || (choix[1] == -1)))
            {
                flagBoutonAppuye[1] = 1;

                if (choix[0] == -1) { choix[0] =  idAutresJoueurs[1]; numeroDuChoix[1] = 0; }
                else if (choix[1] == -1) { choix[1] = idAutresJoueurs[1]; numeroDuChoix[1] = 1; }
            }
            else if (flagBoutonAppuye[1] == 1)
            {
                choix[numeroDuChoix[1]] = -1;
                flagBoutonAppuye[1] = 0;
            }
        }

        if ((515<mx)&&(mx<(515+240)) && (300<my)&&(my<(300+80))) //{515, 300, 240, 80}
        {
                printf("Clic bouton 2\n"); //TEST
            if((flagBoutonAppuye[2] == 0) && ((choix[0] == -1) || (choix[1] == -1)))
            {
                flagBoutonAppuye[2] = 1; 

                if (choix[0] == -1) { choix[0] = idAutresJoueurs[2]; numeroDuChoix[2] = 0;}
                else if (choix[1] == -1) { choix[1] = idAutresJoueurs[2]; numeroDuChoix[2] = 1;}
            }
            else if (flagBoutonAppuye[2] == 1)
            {
                choix[numeroDuChoix[2]] = -1;
                flagBoutonAppuye[2] = 0;
            }
        }

        if ((765<mx)&&(mx<(765+240)) && (300<my)&&(my<(300+80)))
        {
            if((flagBoutonAppuye[3] == 0) && ((choix[0] == -1) || (choix[1] == -1)))
            {
                flagBoutonAppuye[3] = 1; 

                if (choix[0] == -1) { choix[0] = idAutresJoueurs[3]; numeroDuChoix[3] = 0;}
                else if (choix[1] == -1) { choix[1] = idAutresJoueurs[3]; numeroDuChoix[3] = 1;}
            }
            else if (flagBoutonAppuye[3] == 1)
            {
                choix[numeroDuChoix[3]] = -1;
                flagBoutonAppuye[3] = 0;
            }
        }

        if((765<mx)&&(mx<(765+240)) && (420<my)&&(my<(420+80)))// {765, 420, 240, 80}
        {
            if((choix[0] != -1 && choix[1] != -1) && (flagChoixFait == 0)) // En fait ce flag est vital, sinon le client peut potentiellemnt spam le serveur et ça, c'est mal !
            {
                flagChoixFait = 1; // Je pense c'est plutôt un passage à l'écran final qu'il faudrait faire ici, ou alors on fait ça lors de la récepion de la réponse du serveur
                if (cptWord == 0) strcpy(word, "-");
                sprintf(sendBuffer, "A %d %d %d %s", gId, choix[0], choix[1], word);
                sendMessageToServer(gServerIpAddress, gServerPort,sendBuffer);
            }
        }
	}
	break;

    default: break;
	}
	}
}

void manageNetwork()
{
 if (synchro==1)
 {
  printf("consomme |%s|\n",gbuffer);
  switch (screenNumber)
  {
   case 0:
    switch (gbuffer[0])
    {
     // Message 'I' : le joueur recoit son Id
     case 'I':
      sscanf(gbuffer+2,"%d",&gId);
      screenNumber=1;
      break;
    }
    break;
   case 1:
    switch (gbuffer[0])
    {
     // Message 'L' : le joueur recoit les noms des joueurs connectés 
     case 'L':
        sscanf(gbuffer+2,"%s %s %s %s %s",
        gNames[0],
        gNames[1],
        gNames[2],
        gNames[3],
        gNames[4]);
        break;

      // Message 'R' : le joueur reçoit les rôles. En théorie tous les joueurs sont connectés, mais on laisse un petit if au cas où.
      case 'R' :
        if (strcmp(gNames[4],"-")!=0)
        {
                sscanf(gbuffer+(2*(gId+1)),"%d", &gRole);
                printf("ID : %d\nRole : %d\n",gId, gRole);
                if (gRole == 0) screenNumber=2; // On change d'écrans ssi : non-espion | (espion & on a le mot)
        }
        break;

      // Message 'W' : Le joueur est un espion et reçoit alors le mot secret
      case 'W' :
        sscanf(gbuffer+2, "%s", &secretWord);
        choix[1] = -2; // On bloque ainsi la 2nd case du choix pour l'écran final, s'assurant qu'il ne puisse voter que pour une personne car il est le 2nd espion
        screenNumber=2; // On change d'écrans ssi : non-espion | (espion & on a le mot)
        break;
    }
   break;

   case 2:
    int tours;
    switch(gbuffer[0])
    {
        // Message 'T' : Le joueur reçoit un message pour signaler son tours de jeu
        case 'T':
         goEnabled = 1;
         
        break;

        // Message 'M' : Le joueur reçoit la liste des mots de chaque joueurs.
        case 'M' :
                sscanf(gbuffer+2,"%s %s %s %s %s %s %s %s %s %s",
                         gWords[0][0], gWords[0][1], 
                         gWords[1][0], gWords[1][1], 
                         gWords[2][0], gWords[2][1], 
                         gWords[3][0], gWords[3][1], 
                         gWords[4][0], gWords[4][1]);
                if (strcmp(gWords[4][1], "-") != 0) {screenNumber = 3;}
        break;
    }
    break;

    case 3:
        switch(gbuffer[0])
        {
                // Message 'S' : Réception du score et des bonnes réponses.
                case 'S' : 
                        sscanf(gbuffer+2, "%d %d %s %d", &idEspion[0], &idEspion[1], &secretWord, &score);
                        screenNumber = 4;
        }
    break;

  }
  synchro=0;
 }
}

void myRenderText(char *m,int x,int y)
{
     SDL_Color col1 = {0, 0, 0};
     SDL_Surface* surfaceMessage = TTF_RenderText_Solid(Sans, m, col1);
     SDL_Texture* Message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

     SDL_Rect Message_rect;
     Message_rect.x = x;
     Message_rect.y = y;
     Message_rect.w = surfaceMessage->w;
     Message_rect.h = surfaceMessage->h;

     SDL_RenderCopy(renderer, Message, NULL, &Message_rect);
     SDL_DestroyTexture(Message);
     SDL_FreeSurface(surfaceMessage);
}



void manageRedraw()
{
 switch (screenNumber)
 {
  case 0: //connect
   {
    // On efface l'écran
    SDL_SetRenderDrawColor(renderer, 255, 230, 230, 230);
    SDL_Rect rect = {0, 0, 1024, 768};
    SDL_RenderFillRect(renderer, &rect);

    // si connectEnabled, alors afficher connect
    if (connectEnabled==1)
    {
     SDL_Rect dstrect = { 0, 0, 200, 50 };
     SDL_RenderCopy(renderer, texture_connectbutton, NULL, &dstrect);
    }

    if (cptWord>0)
	myRenderText(word,105,350);
   }
   break;
  case 1:  //"salle d'attente"
   {
        SDL_SetRenderDrawColor(renderer, 255, 230, 230, 230);
        SDL_Rect rect = {0, 0, 1024, 768};
        SDL_RenderFillRect(renderer, &rect);

        myRenderText(gNames[0],105,50);
        myRenderText(gNames[1],105,110);
        myRenderText(gNames[2],105,170);
        myRenderText(gNames[3],105,230);
        myRenderText(gNames[4],105,300);
   }
   break;

  case 2:  
  {
        SDL_SetRenderDrawColor(renderer, 255, 230, 230, 230);
        SDL_Rect rect = {0, 0, 1024, 768};
        SDL_RenderFillRect(renderer, &rect);

		//j'aurais pu faire ça avec un for mais ct trop tard quand j'ai realisé hihi
        myRenderText(gNames[0],50,200);
        myRenderText(gNames[1],250,200);
        myRenderText(gNames[2],450,200);
        myRenderText(gNames[3],650,200);
        myRenderText(gNames[4],850,200);

		for(int i = 0; i < 5; i++)
		{
			for(int j = 0; j < 2; j++)
			{
				myRenderText(gWords[i][j], 50 + (200*i), 325 + (75*j) );
			}
		}


        if(gRole == 1) //faut ajouter l'affichage du mot secret ici
        {
            myRenderText("Role: espion", 0, 0);
			myRenderText("Mot secret: ", 400, 0);
			myRenderText(secretWord, 720, 0);
        }
        if(gRole == 0)
        {
                myRenderText("Role: contre-espion", 0, 0);
        }

		if(goEnabled == 1)
		{
        	SDL_Rect textBoxBG = {450, 560, 330, 60};
        	SDL_SetRenderDrawColor(renderer, 0, 0, 0,0);
        	SDL_RenderDrawRect(renderer, &textBoxBG);
        	myRenderText("Mot: ", 320, 545);
        	if (cptWord>0)
        	{
        	    myRenderText(word, 450, 550);
        	}
		}

  } 
  break;
  
  case 3: //écran où on tente de deviner les espions
  {

	//on efface l'écran:
 	SDL_SetRenderDrawColor(renderer, 255, 230, 230, 230);
    SDL_Rect rect = {0, 0, 1024, 768};
    SDL_RenderFillRect(renderer, &rect);


	//partie commune aux deux roles:(

    SDL_Rect ButtonBG0 = {15, 300, 240, 80};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0,0);
    SDL_RenderDrawRect(renderer, &ButtonBG0);
    if(flagBoutonAppuye[0] == 1)  //si le joueur a appuyé sur ce bouton, on le remplit avec du vert
    {
        SDL_SetRenderDrawColor(renderer, 1, 50, 32, 100);
        SDL_RenderFillRect(renderer, &ButtonBG0);
    }

    SDL_Rect ButtonBG1 = {265, 300, 240, 80};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0,0);
    SDL_RenderDrawRect(renderer, &ButtonBG1);
    if(flagBoutonAppuye[1] == 1)
    {
        SDL_SetRenderDrawColor(renderer, 1, 50, 32, 100);
        SDL_RenderFillRect(renderer, &ButtonBG1);
    }

    SDL_Rect ButtonBG2 = {515, 300, 240, 80};
	SDL_SetRenderDrawColor(renderer, 0, 0, 0,0);
    SDL_RenderDrawRect(renderer, &ButtonBG2);
    if(flagBoutonAppuye[2] == 1)
    {
        SDL_SetRenderDrawColor(renderer, 1, 50, 32, 100);
        SDL_RenderFillRect(renderer, &ButtonBG2);
    }

    SDL_Rect ButtonBG3 = {765, 300, 240, 80};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0,0);
    SDL_RenderDrawRect(renderer, &ButtonBG3);
    if(flagBoutonAppuye[3] == 1)
    {
        SDL_SetRenderDrawColor(renderer, 1, 50, 32, 100);
        SDL_RenderFillRect(renderer, &ButtonBG3);
    }

    int j = 0;
    for(int i = 0; i < 5; i++)
    {
        if (i != gId)
        {
            myRenderText(gNames[i],(250*(j)+15), 300);
            j++;
        }
    }

    SDL_Rect boutonValider = {765, 420, 240, 80};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0,0);
    SDL_RenderDrawRect(renderer, &boutonValider);
	myRenderText("Valider", 770, 420);

	//partie en fonction du role:
    if(gRole == 1)
    {
		myRenderText("Qui est l'autre espion?",0,0);
		char temp_str[500];
		strcpy(temp_str,"Rappel: le mot secret est:");
		strcat(temp_str, secretWord);
		myRenderText(temp_str, 70, 500);

    }
    if(gRole == 0)
    {
		myRenderText("Qui sont les deux espions?",0,0);

        //zone de texte pour saisir le mot secret:
        SDL_Rect textBoxBGBis = {450, 560, 330, 60};
    	SDL_SetRenderDrawColor(renderer, 0, 0, 0,0);
    	SDL_RenderDrawRect(renderer, &textBoxBGBis);
    	myRenderText("Mot: ", 320, 545);
    	if (cptWord>0)
    	{
    	    myRenderText(word, 450, 550);
    	}
    }

  } 
  break;

  case 4: //ecran de fin
  {
    //on efface l'écran:
    SDL_SetRenderDrawColor(renderer, 255, 230, 230, 230);
    SDL_Rect rect = {0, 0, 1024, 768};
    SDL_RenderFillRect(renderer, &rect);

    char temp_buff[256];
    sprintf(temp_buff, "Fin de la partie. Votre score est %d", score);
    myRenderText(temp_buff, 10, 250);
    sprintf(temp_buff, "Les espions etaient %s et %s", gNames[idEspion[0]], gNames[idEspion[1]]);
    myRenderText(temp_buff, 10, 300);
    sprintf(temp_buff, "et le mot secret etait %s", secretWord);
    myRenderText(temp_buff, 10, 350);

    SDL_RenderPresent(renderer);
    sleep(10); //pour laisser le temps de lire 
    exit(1);
  }
  break;

  default:
   break;
 }
 SDL_RenderPresent(renderer);
}


int main(int argc, char ** argv)
{

 int ret;

 if (argc<6)
 {
  printf("<app> <Main server ip address> <Main server port> <Client ip address> <Client port> <player name>\n");
  exit(1);
 }

 strcpy(gServerIpAddress,argv[1]);
 gServerPort=atoi(argv[2]);
 strcpy(gClientIpAddress,argv[3]);
 gClientPort=atoi(argv[4]);
 strcpy(gName,argv[5]);

 SDL_Init(SDL_INIT_VIDEO);
 TTF_Init();
 
 window = SDL_CreateWindow("SDL2 LINQ", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, 0);
 
renderer = SDL_CreateRenderer(window, -1, 0);

connectbutton = IMG_Load("connectbutton.png");
texture_connectbutton = SDL_CreateTextureFromSurface(renderer, connectbutton);

// Initialisation de variables

strcpy(gNames[0],"-");
strcpy(gNames[1],"-");
strcpy(gNames[2],"-");
strcpy(gNames[3],"-");
strcpy(gNames[4],"-");

goEnabled=0;
connectEnabled=1;
screenNumber=0;

strcpy(word,"");
cptWord=0;

Sans = TTF_OpenFont("sans.ttf", 60); 

/* Creation du thread serveur tcp. */
printf ("Creation du thread serveur tcp !\n");
synchro=0;
ret = pthread_create ( & thread_serveur_tcp_id, NULL, fn_serveur_tcp, NULL);

while (!quit)
{
 if (SDL_PollEvent(&event))
  manageEvent(event);
 manageNetwork();
 manageRedraw();
}
 
 SDL_DestroyRenderer(renderer);
 SDL_DestroyWindow(window);
 
 SDL_Quit();
 
 return 0;
}
