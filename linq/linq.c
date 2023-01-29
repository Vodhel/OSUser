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


pthread_t thread_serveur_tcp_id;
char gbuffer[256];
char gServerIpAddress[256];
int gServerPort;
char gClientIpAddress[256];
int gClientPort;
char gName[256];
char gNames[5][256];
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

SDL_Surface *connectbutton;
SDL_Texture *texture_connectbutton;
TTF_Font* Sans; 

volatile int synchro;

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
                //printf("%s",gbuffer);

                synchro=1;

                while (synchro); // Permet d'empêcher qu'on écrase le buffer alors qu'il n'a pas encore été traité.

     }
}

void sendMessageToServer(char *ipAddress, int portno, char *mess)
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char sendbuffer[256];

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
			case 3:  
			{
				int car=event.key.keysym.sym;
    			printf("%d\n",event.key.keysym.sym);
    			if (car==8)
    			{
     				strcpy(word,"");
     				cptWord=0;
    			}

				else if (car == 10)   //si touche entrée
				{
					//envoyerMotAuServeur(message);
					//screenNumber = ecran suivant , s'il ne faut faire ça ailleurs
					
				}

    			else if ((car>=97) && (car<=122)) 
    			{
    				word[cptWord++]=car;
    				word[cptWord]='\0';
    			}
			}
			break;

			default:
			break;
		}
	}
	break;

  	case  SDL_MOUSEBUTTONDOWN:
   	switch(screenNumber)
   	{
    case 0:
     	SDL_GetMouseState( &mx, &my );
     	if ((mx<200) && (my<50) && (connectEnabled==1))
     	{
      		sprintf(sendBuffer,"C %s %d %s", gClientIpAddress,gClientPort,gName);
      		sendMessageToServer(gServerIpAddress, gServerPort,sendBuffer);
      		connectEnabled=0;
     	}
    default:
    	break;
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
                //int temp_gRole[5] = {-1, -1, -1, -1, -1};
                //printf("Le return du scan f : %d\n",sscanf(gbuffer,"R %d %d %d %d %d",&temp_gRole[0], &temp_gRole[1], &temp_gRole[2], &temp_gRole[3], &temp_gRole[4]));
                //gRole = temp_gRole[gId];
                //printf("Les temp roles : %d %d %d %d %d", temp_gRole[0], temp_gRole[1], temp_gRole[2], temp_gRole[3], temp_gRole[4]);
                printf("ID : %d\nRole : %d\n",gId, gRole);
                if (gRole == 0) screenNumber=2; // On change d'écrans ssi : non-espion | (espion & on a le mot)
        }
        break;

      // Message 'W' : Le joueur est un espion et reçoit alors le mot secret
      case 'W' :
        sscanf(gbuffer+2, "%s", &secretWord);
        screenNumber=2; // On change d'écrans ssi : non-espion | (espion & on a le mot)
        break;
    }
   break;

   case 2:

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

//On pourrait remplacer les ecrans a deux version par un if(espion) stratigiquement placé mais pour l'instant je vais laisser comme ça

  case 2: //affiché lors des tours des autres joueurs 
  {
        SDL_SetRenderDrawColor(renderer, 255, 230, 230, 230);
        SDL_Rect rect = {0, 0, 1024, 768};
        SDL_RenderFillRect(renderer, &rect);

        myRenderText(gNames[0],400,400);
        myRenderText(gNames[1],250,200);
        myRenderText(gNames[2],500,80);
        myRenderText(gNames[3],750,200);
        myRenderText(gNames[4],700,400);
        if(gRole == 1) //faut ajouter l'affichage du mot secret ici
        {
                myRenderText("Role: espion", 0, 0);
        }
        if(gRole == 0)
        {
                myRenderText("Role: contre-espion", 0, 0);
        }

  } 
  break;

  case 3: //affiché lors du tour du joueur
  {

        SDL_SetRenderDrawColor(renderer, 255, 230, 230, 230);
        SDL_Rect rect = {0, 0, 1024, 768};
        SDL_RenderFillRect(renderer, &rect);

        myRenderText(gNames[0],400,400);
        myRenderText(gNames[1],250,200);
        myRenderText(gNames[2],500,80);
        myRenderText(gNames[3],750,200);
        myRenderText(gNames[4],700,400);
        
        if(gRole == 1) //faut ajouter l'affichage du mot secret ici
        {
                myRenderText("Role: espion", 0, 0);
        }
        if(gRole == 0)
        {
                myRenderText("Role: contre-espion", 0, 0);
        }

        SDL_Rect textBoxBG = {450, 560, 330, 60};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0,0);
        SDL_RenderDrawRect(renderer, &textBoxBG);
        myRenderText("Mot: ", 320, 545);
        if (cptWord>0)
        {
                myRenderText(word, 450, 550);
        }
        
  }
  
  case 4: //écran ou on tente de deviner les espions
  {


  } 
  break;

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
