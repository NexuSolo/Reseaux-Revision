#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

int server_1p();

int server_2p();

int serveur_1p_fork();

int serveur_1p_thread();

void game_2p();

void game_1p(int sock);

int serveur_1p_thread();

int serveur_2p_thread();

void *game_1p_point(void *arg) {
    int *joueurs = (int *)arg;
    game_1p(*joueurs);
    return NULL;
}
typedef struct {
    int *sock1;
    int *sock2;
} thread_args;

int server_np(int n);

void *game_2p_point(void *arg) {
    thread_args *args = (thread_args *)arg;
    int *joueurs1 = args->sock1;
    int *joueurs2 = args->sock2;
    game_2p(*joueurs1, *joueurs2);
    return NULL;
}

int main(int argn,char** args) {
  if(argn<=1|| strcmp(args[1],"-1p")==0){
    //server_1p();
    //serveur_1p_fork();
    //server_2p();
    //serveur_1p_thread();
    //serveur_2p_thread();
    server_np(3);
  }
}

/***************LES FONCTIONS DE JEU ******************/

/**
 * fait tourner une partie pour un joueur dont le socket est passé en argument
 */
void game_1p(int sock) {
  srand(time(NULL) + sock);
  
  // une valeur aléatoire mystere entre 0 et 65536
  unsigned short int n = rand() % (1 << 16); 
  printf("nb mystere pour partie socket %d = %d\n", sock, n);

  unsigned short int guess;  // le nb proposé par le joueur, sur 2 octets
  int taille = 0;
  int tentatives = 20;
  int gagne = 0;
  char buff_in[100];
  while ((taille = recv(sock, buff_in, 100, 0)) > 0) {
    sscanf(buff_in, "%hu", &guess);
    printf("Joueur socket %d a envoyé : %d\n", sock, guess);
    char reponse[20];
    if (n < guess || n > guess) {
      tentatives--;
    }
    if (tentatives == 0)
      sprintf(reponse, "PERDU\n");
    else if (n < guess)
      sprintf(reponse, "MOINS %d\n", tentatives);
    else if (n > guess)
      sprintf(reponse, "PLUS %d\n", tentatives);
    else {
      sprintf(reponse, "GAGNE\n");
      gagne = 1;
    }
    send(sock, reponse, strlen(reponse), 0);
    if (gagne || !tentatives) break;
  }
  printf("Fin de partie\n");

  close(sock);
}

/**
 * serveur pour jeu 1 player avec 1 connexion à la fois
 */
int server_1p() {
    int serv_sock = socket(PF_INET6, SOCK_STREAM, 0);

    struct sockaddr_in6 serv_addr;
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_port = htons(9873);
    serv_addr.sin6_addr = in6addr_any;

    int r = bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (r != 0) {
        fprintf(stderr, "Échec de bind");
        exit(-1);
    }

    r = listen(serv_sock, 0);
    if (r != 0) {
        fprintf(stderr, "Échec de listen");
        exit(-1);
    }

    int client_sock = 0;
    while (1) {
        client_sock = accept(serv_sock, NULL, NULL);
        if (client_sock < 0) {
            fprintf(stderr, "Échec de accept");
            exit(-1);
        } 
        else {
            printf("Connexion acceptee, nouvelle partie lancée.\n");
            game_1p(client_sock);
            close(client_sock);      
        }
    }
    close(serv_sock);
    return 0;
}

int serveur_1p_fork(){
    int serv_sock = socket(PF_INET6, SOCK_STREAM, 0);
    
    struct sockaddr_in6 serv_addr;
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_port = htons(9876);
    serv_addr.sin6_addr = in6addr_any;

    
    int r = bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (r != 0) {
        fprintf(stderr, "Échec de bind");
        exit(-1);
    }
    
    r = listen(serv_sock, 0);
    if (r != 0) {
        fprintf(stderr, "Échec de listen");
        exit(-1);
    }
    
    int client_sock = 0;
    while (1) {
        client_sock = accept(serv_sock, NULL, NULL);
        if (client_sock < 0) {
        fprintf(stderr, "Échec de accept");
        exit(-1);
        } 
        else {
        printf("Connexion acceptee, nouvelle partie lancée.\n");
        int pid = fork();
        if (pid == 0) {
            game_1p(client_sock);
            close(client_sock);
            exit(0);
        }
        }
    }
    close(serv_sock);
    return 0;
}

int serveur_1p_thread(){
    int serv_sock = socket(PF_INET6, SOCK_STREAM, 0);
    
    struct sockaddr_in6 serv_addr;
    serv_addr.sin6_family = AF_INET;
    serv_addr.sin6_port = htons(9876);
    serv_addr.sin6_addr  = in6addr_any;
    
    int r = bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (r != 0) {
        fprintf(stderr, "Échec de bind");
        exit(-1);
    }
    
    r = listen(serv_sock, 0);
    if (r != 0) {
        fprintf(stderr, "Échec de listen");
        exit(-1);
    }
    
    int client_sock = 0;
    while (1) {
        client_sock = accept(serv_sock, NULL, NULL);
        if (client_sock < 0) {
        fprintf(stderr, "Échec de accept");
        exit(-1);
        } 
        else {
        printf("Connexion acceptee, nouvelle partie lancée.\n");
        pthread_t thread;
        pthread_create(&thread, NULL, game_1p_point, &client_sock);
        }
    }
    close(serv_sock);
    return 0;
}

int server_2p() {
  int serv_sock = socket(PF_INET, SOCK_STREAM, 0);

  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(9875);
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  int r = bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (r != 0) {
    fprintf(stderr, "Échec de bind");
    exit(-1);
  }

  r = listen(serv_sock, 0);
  if (r != 0) {
    fprintf(stderr, "Échec de listen");
    exit(-1);
  }

  int client_sock1 = 0;
  int client_sock2 = 0;
  while (1) {
    client_sock1 = accept(serv_sock, NULL, NULL);
    client_sock2 = accept(serv_sock, NULL, NULL);
    if (client_sock1 < 0) {
      fprintf(stderr, "Échec de accept");
      exit(-1);
    } else if (client_sock2 < 0) {
      fprintf(stderr, "Échec de accept");
      exit(-1);
    } 
    else {
      printf("Connexion acceptee, nouvelle partie lancée.\n");
      game_2p(client_sock1, client_sock2);
      close(client_sock1);
      close(client_sock2);      
    }
  }
  close(serv_sock);
  return 0;
}

int serveur_2p_thread(){
    int serv_sock = socket(PF_INET6, SOCK_STREAM, 0);
    
    struct sockaddr_in6 serv_addr;
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_port = htons(9876);
    serv_addr.sin6_addr  = in6addr_any;
    
    int r = bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (r != 0) {
        fprintf(stderr, "Échec de bind");
        exit(-1);
    }
    
    r = listen(serv_sock, 0);
    if (r != 0) {
        fprintf(stderr, "Échec de listen");
        exit(-1);
    }
    
    int client_sock1 = 0;
    int client_sock2 = 0;
    while (1) {
        client_sock1 = accept(serv_sock, NULL, NULL);
        client_sock2 = accept(serv_sock, NULL, NULL);
        if (client_sock1 < 0) {
            fprintf(stderr, "Échec de accept");
            exit(-1);
        } else if (client_sock2 < 0) {
            fprintf(stderr, "Échec de accept");
            exit(-1);
        } 
        
        else {
            thread_args *args = (thread_args *) malloc(sizeof(thread_args));
            args->sock1 = &client_sock1;
            args->sock2 = &client_sock2;
            printf("Connexion acceptee, nouvelle partie lancée.\n");
            pthread_t thread;
            pthread_create(&thread, NULL, game_2p_point, args);
        }
    }
    close(serv_sock);
    return 0;
}


/************************* PARTIE 2 JOUEURS */
/**
 * fait tourner une partie pour 2 joueur dont le socket est passé en argument
 */
void game_2p(int sock1, int sock2) {
  srand(time(NULL) + sock1);
  
  // une valeur aléatoire mystere entre 0 et 65536
  unsigned short int n = rand() % (1 << 16); 
  printf("nb mystere pour partie socket1 %d, socket2 %d = %d\n", sock1, sock2, n);

  unsigned short int guess1;  // le nb proposé par le joueur, sur 2 octets
  unsigned short int guess2;  // le nb proposé par le joueur, sur 2 octets
  int taille_sock1 = 0;
  int taille_sock2 = 0;
  int tours = 10;
  int gagne = 0;
  char buff_in1[100];
  char buff_in2[100];
  while (!gagne) {
    taille_sock1 = recv(sock1, buff_in1, 100, 0);
    taille_sock2 = recv(sock2, buff_in2, 100, 0);
    sscanf(buff_in1, "%hu", &guess1);
    sscanf(buff_in2, "%hu", &guess2);
    printf("Joueur 1 a envoyé : %d\n", guess1);
    printf("Joueur 2 a envoyé : %d\n", guess2);
    char reponse1[20];
    char reponse2[20];
    if (n < guess1 || n > guess1 && (n < guess2 || n > guess2)) {
      tours--;
    }
    if (n < guess1)
      sprintf(reponse1, "MOINS %d\n", tours);
    else if (n > guess1)
      sprintf(reponse1, "PLUS %d\n", tours);

    if (n < guess2)
      sprintf(reponse2, "MOINS %d\n", tours);
    else if (n > guess2)
      sprintf(reponse2, "PLUS %d\n", tours);
    else if (n == guess1){
      sprintf(reponse1, "GAGNE\n");
      sprintf(reponse2, "PERDU\n");
      gagne = 1;
    } 
    if (n == guess2){
      sprintf(reponse2, "GAGNE\n");
      sprintf(reponse1, "PERDU\n");
      gagne = 1;
    }
    if (tours == 0) {
      sprintf(reponse1, "PERDU\n");
      sprintf(reponse2, "PERDU\n");
    }
    send(sock1, reponse1, strlen(reponse1), 0);
    send(sock2, reponse2, strlen(reponse2), 0);
    if (gagne || !tours) break;
  }
  printf("Fin de partie\n");

  close(sock1);
  close(sock2);
}

/************************* PARTIE n JOUEURS */
/**
 * fait tourner une partie pour n joueur dont le socket est passé en argument
 */
void game_np(int nb_socks, int *socks) {
  srand(time(NULL) + socks[0]);
  
  // une valeur aléatoire mystere entre 0 et 65536
  unsigned short int n = rand() % (1 << 16); 
  printf("nb mystere pour partie  = %d\n", n);

  unsigned short int guess1;  // le nb proposé par le joueur, sur 2 octets
  int taille_sock1 = 0;
  int tours = 10;
  int gagne = 0;
  char buff_in1[100];
  
  while (!gagne) {
    for(int i = 0; i < nb_socks; i++){
        taille_sock1 = recv(socks[i], buff_in1, 100, 0);
        sscanf(buff_in1, "%hu", &guess1);
        printf("Joueur %d a envoyé : %d\n", i, guess1);
        char reponse1[20];
        if (n < guess1)
            sprintf(reponse1, "MOINS %d\n", tours);
        else if (n > guess1)
            sprintf(reponse1, "PLUS %d\n", tours);
        else if (n == guess1){
            sprintf(reponse1, "GAGNE\n");
            gagne = 1;
        }
        send(socks[i], reponse1, strlen(reponse1), 0);
    }
    if(!gagne){
        tours--;
    }

    if(tours == 0){
        for(int i = 0; i < nb_socks; i++){
            char reponse1[20];
            sprintf(reponse1, "PERDU\n");
            send(socks[i], reponse1, strlen(reponse1), 0);
            break;
        }
    }

    if(gagne){
        for(int i = 0; i < nb_socks; i++){
            char reponse1[20];
            sprintf(reponse1, "FIN PARTIE, NOUS AVONS UN GAGNANT\n");
            send(socks[i], reponse1, strlen(reponse1), 0);
        }
        break;
    }
  }
    printf("Fin de partie\n");
    for(int i = 0; i < nb_socks; i++){
        close(socks[i]);
    }
  
}

int server_np(int nb_socks) {
  int serv_sock = socket(PF_INET, SOCK_STREAM, 0);

  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(9875);
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  int r = bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (r != 0) {
    fprintf(stderr, "Échec de bind");
    exit(-1);
  }

  r = listen(serv_sock, 0);
  if (r != 0) {
    fprintf(stderr, "Échec de listen");
    exit(-1);
  }

  int *socks = malloc(nb_socks * sizeof(int));
  while (1) {
    for(int i = 0; i < nb_socks; i++){
        socks[i] = accept(serv_sock, NULL, NULL);
        if (socks[i] < 0) {
            fprintf(stderr, "Échec de accept");
            for (int j = 0; j < i; j++) {
                close(socks[j]);
            }
            exit(-1);
        } 
    }
    printf("Connexion acceptee, nouvelle partie lancée.\n");
    game_np(nb_socks, socks);
    
  }
  for (int i = 0; i < nb_socks; i++) {
    close(socks[i]);
  }
  return 0;
}

