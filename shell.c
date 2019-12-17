/*Tero Nevalainen */
/*Shell*/
/*Lähteet https://brennan.io/2015/01/16/write-a-shell-in-c/
					man-sivut, stackoverflow*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#define MAXIMI 1024
char error_message[30] = "An error has occurred\n";

void loop(char **);
char *luerivi(void);
char **parse(char *);
char *suoritus(char **, char *, char *, char *);
int cd(char **);
char *path(char **, char *);
int suoritus2(char **, char *, char *);
int suoritus3(char **, char *, char *);
char *batch(char *, char **);
char *parsetdsto(char *);

int main(int argc, char **argv) {
  if (argc == 1) { //Interactive mode
    loop(NULL);
  }
  if (argc == 2) { //Batch mode
    loop(argv);
  }
  else {
    printf("Error: Too many arguments.\n"); //Liikaa argumentteja
  }
  return(0);
}

void loop(char **argv) {
  char *rivi;
  char **argit;								
  char *polku = malloc(MAXIMI*sizeof(char));
  char *tdsto = NULL;
  strcpy(polku,"/bin"); 
  polku[strlen(polku)+1] = 0; 
  if (argv != NULL) {
    polku = batch(polku, argv); //Batchi käyntiin, luetaan, parsitaan ja suoritetaan tdsto
  }  
  while (1) {                  //https://brennan.io/2015/01/16/write-a-shell-in-c/ Täältä idea
    printf("wish> ");          
    rivi = luerivi();          //Rivin lukeminen
    tdsto = parsetdsto(rivi);  //Tarkistaa onko syötteessä ">"
    argit = parse(rivi);			 //Argumentit	
    polku = suoritus(argit, polku, rivi, tdsto); //Tarkastamaan syötteitä ja suoritukseen  
  }   
}

char *luerivi(void) {				   //Lukee standard inputista ja palauttaa rivin
  size_t koko = 0;
  char *rivi = NULL;
  if (getline(&rivi, &koko, stdin) == -1) { 
    perror("getline");                     
  }
  return rivi;
}

char **parse(char *rivi) {  
	int i=0;  
	char *x;                                 
  char **argit = malloc(MAXIMI*(sizeof(char)));
  if (argit == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
  }
  x = strtok(rivi, " \n");
  while (x != NULL) {                  //Luodaan lista ja palautetaan se
    argit[i] = x;                      
    i++;
    x = strtok(NULL, " \n");
  }
  argit[i] = NULL;
  return(argit);
}

char *suoritus(char **argit, char *polku, char *rivi, char *tdsto) {
  if (argit[0] == NULL) {
    free(argit);
    free(rivi);
    return(polku);
  }
  char *builtins[] = {"exit", "cd", "path"};	//sisäänrakennetut komennot, tarkastetaan nämä
  if ((strcmp(builtins[0], argit[0])) == 0) {					//exit
    free(argit);		//argumenttien muistinvapautus
    free(polku);		//polun muistinvapautus
  	free(rivi);			//rivin muistinvapautus
    exit(0);  			//Ohjelman lopetus kun käyttäjä syöttää "exit"
  } else if ((strcmp(builtins[1], argit[0])) == 0) {	//cd
    cd(argit);
    free(argit);
    free(rivi);
    return(polku);    
  } else if ((strcmp(builtins[2], argit[0])) == 0) {	//path
    polku = path(argit, polku);
    free(argit);
    free(rivi);
    return(polku);
  } 
  suoritus2(argit, polku, tdsto); //Syötteessä ei builtineja, joten suoritus2
  free(argit);
  free(rivi);
  return(polku);
}

int cd(char **argit) {  	//builtin cd
  if (argit[1] == NULL) {
    fprintf(stderr, "Expecting 2 arguments\n");
    return(1);    
  } else if (argit[2] != NULL) {
    fprintf(stderr, "Too many arguments\n");
    return(1);    
  } else {
    if (chdir(argit[1]) != 0) {	//cd
      printf("error in cd\n");
    }
  }
  return(0);    
}

char *path(char **argit, char *polku) { //Pathin käsittelyä/muutoksia käyttäjän-
  int i;																//syötteiden mukaan
  char x[] = ";";
  strcpy(polku, "");
  if (argit[1] == NULL) {		//Ei argumenttia 
    return(polku);
  }
  for (i=1; argit[i] != NULL; i++) {	//muodostetaan path merkkijono yhdestä tai
    strcat(polku, argit[i]);					//useammasta argumentistä
    strcat(polku, x);
  }
  return(polku);
}

int suoritus2(char **argit, char *polku, char *tdsto) {
  int i = 0;
  int j;
  char vv[] = "/";
  char **lista = malloc(MAXIMI*sizeof(char*));
  if  (lista  == NULL) {
  	write(STDERR_FILENO, error_message, strlen(error_message));
  	perror("malloc");
  }
  char *mjono = malloc(MAXIMI*sizeof(char*));
  if (mjono  == NULL) {
  	write(STDERR_FILENO, error_message, strlen(error_message));
  	perror("malloc");
  }
  char *token;
  char *polku2 = malloc(MAXIMI*sizeof(char*));
  if (polku2  == NULL) {
  	write(STDERR_FILENO, error_message, strlen(error_message));
  	perror("malloc");
  }
  strcpy(polku2, polku);
  token = strtok(polku2, ";");	
  while (token != NULL) {				//Listataan pathit
    lista[i] = token;
    i++;
    token = strtok(NULL, ";");    
  }
  for (j=0; j<i; j++) {					//Käydään lista läpi ja muokataan exevc varten
    strcpy(mjono, lista[j]);
    strcat(mjono, vv);
    strcat(mjono, argit[0]);
    if (access(mjono, X_OK) != -1) { //Jos voidaan suorittaa: siirrytään: suoritus3
      suoritus3(argit, mjono, tdsto);
      free(lista);
      free(mjono);
      free(token);
      free(polku2);
      return(0);
    }
  }
  if (tdsto != NULL) {						//Tiedoston virheen kirjoitus
  	FILE *file;
    if ((file = fopen(tdsto, "w")) == NULL) {
			perror("fopen");
			exit(1);
		} else {
			fprintf(file, "%s", error_message);
			fclose(file);
			free(lista);
			free(mjono);
			free(token);
			free(polku2);
			return(1);
		}
  }
  write(STDERR_FILENO, error_message, strlen(error_message));
  perror("Error in access");		//Syötekomento väärä ja muistinvapautuksia
  free(lista);
  free(mjono);
  free(token);
  free(polku2);
  return(1);
}

int suoritus3(char **argit, char *polku, char *tdsto) {
  int filex;
  FILE *file;
  if (tdsto != NULL) {
    if ((file = fopen(tdsto, "w")) == NULL) {		//
      perror("fopen");
      exit(1);
    }else {
      fclose(file);
    }
  }
  if (tdsto != NULL) {
    filex = open(tdsto, O_WRONLY|O_CREAT);
  }
  int pid;
  pid = fork();								//luennoista
  if (pid == -1) {
    exit(1);
  }
  else if (pid == 0) { 				//Child-prosessissa suoritetaan haluttu komento execv käyttäen
    if (tdsto != NULL) {
      close(1);
      dup(filex);							//kopio tiedostosta
    }													//https://stackoverflow.com/questions/29154056/redirect-stdout-to-a-file
    if (execv(polku, argit) == -1) { //https://stackoverflow.com/questions/28507950/calling-ls-with-execv
      write(STDERR_FILENO, error_message, strlen(error_message));
      perror("execv");
      exit(1);
    }	
  }
  else { 										 //Parent 
    if (wait(&pid) == -1) {
      perror("wait");
      exit(1);
    }
    if (tdsto != NULL) {
      close(filex);
    }
    return(0); 
  }
  return(0);
}

char *batch(char *polku, char **argv) {					
  FILE *file;
  if ((file = fopen(argv[1], "r")) == NULL) {		//Tiedoston avaus
    perror("fopen");
    exit(1);
  }
  char *tied = NULL;
  char **argit = NULL;
  size_t koko = 0;
  char *rivi = NULL;
  while ((getline(&rivi, &koko, file)) != -1) {	//Käydään läpi tiedoston rivit
    tied = parsetdsto(rivi);										//Rivi parsiutumaan 
    argit = parse(rivi);												//Argumentit
    polku = suoritus(argit, polku, rivi, tied);
    rivi = malloc(sizeof(char)*MAXIMI);					//Muistivuodot ja errorit pois tiedostoja-
  }																							//käsitellessä					
  free(rivi);
  fclose(file);
  return(polku);
}

char *parsetdsto(char *rivi) {
  char *tds;
  tds = strtok(rivi, ">");			//Erotellaan merkkijono jos huomataan ">" 
  tds = strtok(NULL, " >\n");		
  return(tds);									//Palautetaan tiedosto
}
    
