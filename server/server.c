/*
 * server.c
 * 
 * Copyright 2019 Michele De Quattro <mikde4@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <util.h>
#include <threadS.h>

/**
 * \file server.c
 * \brief codice sorgente per il server che rappresenta l'object store
 * \author Michele De Quattro
*/

/**
 * \def MAXBACKLOG
 * \brief il numero massimo di connessioni in coda nella lista
 */
#define MAXBACKLOG 32

volatile int interrupt_end = 0; /**< variabile per indicare che è arrivata una signal di interruzione */
connection_stat **tcv = NULL; /**< vettore contenente statistiche di ogni connessione effettuata */

/**
 * \fn cleanup
 * \brief funzione per eliminare la socket una volta terminato il server
 */
void cleanup() {
	unlink(SOCKNAME);
}

/**
 * \fn printstat (connection_stat **tcv, int n_thread)
 * \brief funzione per stampare a schermo le statistiche
 * \param tcv il vettore di connessioni, contenente le statistiche di ogni connessione
 * \param n_thread la lunghezza di tcv
 */
void printstat (connection_stat **tcv, int n_thread) {
	DIR *dir;
	struct dirent* file;
	int n_activeconnections = 0;
	int n_files = 0;
	int total_size = 0;
	int len = N;
	char *buf = malloc(sizeof(char)*len);
	struct stat info;
	int notused;
	
	fprintf(stdout, "ci sono state attualmente: %i connessioni diverse\n", n_thread);
	for (int i = 0; i < n_thread; i++) {
		pthread_mutex_lock(&registration_mut);
		int socket_id = tcv[i]->socket_id;
		char* name = tcv[i]->name;
		time_t start = tcv[i]->start;
		time_t end = tcv[i]->end;
		pthread_mutex_unlock(&registration_mut);
		if (socket_id > 0)
			n_activeconnections++;
		if (name != NULL) {
			if (end == 0)
				fprintf(stdout, "Connessione su %s iniziata a %li e non ancora terminata\n", name, start);
			else 
				fprintf(stdout, "Connessione su %s iniziata a %li e finita a %li\n", name, start, end);
		}
	}
	fprintf(stdout, "Attualmente sono attive %i connessioni\n", n_activeconnections);
	
	//CHECKNULL (notnullused, getcwd(buf, N), "getcwd");
	while (getcwd(buf, N) == NULL) {
		if (errno == ERANGE)
			buf = realloc(buf, sizeof(char)*(len=(len*2)));
		else {
			perror("getcwd interna");
			exit(errno);
		}
	}
	SYSCALL(notused, chdir(DATAPATH), "chdir");
	CHECKNULL (dir, opendir("."), "opendir");
	while ((errno = 0, file = readdir(dir)) != NULL) {
		if (strcmp (file->d_name, ".") && strcmp(file->d_name, "..")) {
			DIR *dir_int;
			struct dirent* file_int;
			int len_int = N;
			char *buf_int = malloc (sizeof(char) *len_int);
			
			SYSCALL (notused, stat(file->d_name, &info), "stat");
			//fprintf (stdout, "%s size:%li\n", file->d_name, info.st_size); //forse (?)
			//CHECKNULL (notnullused, getcwd(buf_int, N), "getcwd");
			while (getcwd(buf_int, N) == NULL) {
				if (errno == ERANGE)
					buf_int = realloc(buf_int, sizeof(char)*(len_int=(len_int*2)));
				else {
					perror("getcwd interna");
					free(buf_int);
					exit(errno);
				}
			}
			SYSCALL(notused, chdir(file->d_name), "chdir interno");
			CHECKNULL (dir_int, opendir("."), "opendir interno");
			while ((errno = 0, file_int = readdir(dir_int)) != NULL) {
				if (strcmp (file_int->d_name, ".") && strcmp(file_int->d_name, "..")) {
					SYSCALL (notused, stat(file_int->d_name, &info), "stat");
					//fprintf (stdout, "\t%s size:%li\n", file_int->d_name, info.st_size); //forse (?)
					n_files++;
					total_size += info.st_size;
				}
			}
			if (errno != 0)
				perror ("readdir interna"); //non ha senso interrompere il server per questa
			SYSCALL (notused, closedir(dir_int), "closedir");
			SYSCALL(notused, chdir(buf_int), "chdir");
			free(buf_int);
		}
	}
	if (errno != 0)
		perror ("readdir"); //non ha senso interrompere il server per questa
	SYSCALL (notused, closedir(dir), "closedir");
	SYSCALL(notused, chdir(buf), "chdir");
	free(buf);
	fprintf(stdout, "ci sono un totale di %i file, per un totale di %i byte salvati\n", n_files, total_size);
}

/**
 * \fn sigHandler
 * \brief funzione di gestione eccezioni
 * \param arg l'argomento della funzione, viene subito castato a sigHandler_t
 */
static void *sigHandler(void *arg) {
	sigset_t *set = ((sigHandler_t*)arg)->set;
	int pipe = ((sigHandler_t*)arg)->pipe;
	int notused;
	while (1) {
		int sig;
		int r = sigwait(set, &sig); //dato che è sigwait non ha le eccezioni mascherate come tutti gli altri thread iirc
		if (r != 0) { //c'è n errore
			errno = r;
			perror("FATAL ERROR in sigwait:");
			return NULL;
		} else {
			switch (sig) {
			case SIGUSR1:
				SYSCALL(notused, write(pipe, "USR1", 5), "Write in sigHandler");;
			break;
			case SIGINT:
			case SIGQUIT:
			case SIGTERM:
				close (pipe); //there is really nothing to say to the matter
				return NULL;
			break;
			default:	;
			}
		}
	}
	return NULL;
}

/**
 * \fn main(int argc, char **argv)
 * \brief Il main del server
 * \param argc non usato
 * \param argv non usato
 * \return EXIT_SUCCESS se il server viene terminato correttamente
 * \return EXIT_FAILURE se PTHREADCALL fallisce
 * \return errno se SYSCALL o CHECKNULL falliscono
 */
int main(int argc, char **argv)
{
	//pulizia socket
	cleanup();    
	atexit(cleanup); 
	
	//dichiarazione variabili
	//variabili per gestione eccezioni
	int signal_pipe[2];
	sigset_t mask, oldmask;
	sigHandler_t h_args;
	pthread_t sigHandler_thread;
	//variabili per gestione server
	struct sockaddr_un socket_address;
	int n_thread = 0;
	int len_tcv = 10;
	int socket_id;
	int client_id;//al posto di questo crearsi una struttura contenente socket e nome dell'utente
	pthread_t t;
	//variabili per la gestione della select
	fd_set set, tmpset;
	int fdmax;
	//variabili generali
	int notused; //variabile in cui vanno tutti i valori di ritorno che non ci interessano
	
	//allocazione iniziale tcv
	CHECKNULL (tcv, malloc(sizeof(connection_stat*)*(len_tcv)), "malloc");
	
	//creazione maschera di filtraggio signal
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGTERM);
	
	//inserimento maschera nella signal mask per i thread
	PTHREADCALL(notused, pthread_sigmask(SIG_BLOCK, &mask, &oldmask), "FATAL ERROR: pthread_sigmask");
	
	//creazione pipe per il thread che gestisce i segnali
	SYSCALL(notused, pipe(signal_pipe), "signal pipe");
	
	//creazione thread di gestione eccezioni
	h_args.set = &mask;
	h_args.pipe = signal_pipe[1];
	PTHREADCALL(notused, pthread_create(&sigHandler_thread, NULL, sigHandler, &h_args), "FATAL ERROR: pthread_create thread eccezioni");
	
	//inizio variabili server
	strncpy (socket_address.sun_path, SOCKNAME, sizeof(SOCKNAME)+1);
	socket_address.sun_family = AF_UNIX;
	
	//inizio server
	SYSCALL(socket_id, socket(AF_UNIX, SOCK_STREAM, 0), "Fallimento socket");
	SYSCALL(notused, bind(socket_id, (struct sockaddr *) &socket_address, sizeof(socket_address)), "Fallimento bind");
	SYSCALL(notused, listen(socket_id, MAXBACKLOG), "Fallimento listen");
	
	//inizio mutex
	PTHREADCALL (notused, pthread_mutex_init(&registration_mut, NULL), "init mutex");
	
	//inizio gestione select
	FD_ZERO(&set);
	FD_ZERO(&tmpset);
	
	FD_SET(socket_id, &set);
	FD_SET(signal_pipe[0], &set);
	
	fdmax = (socket_id > signal_pipe[0]) ? socket_id : signal_pipe[0];
	
	//server in accettazione usando la select
	while (!interrupt_end) { 
		
		tmpset = set;
		SYSCALL(notused, select (fdmax+1, &tmpset, NULL, NULL, NULL), "select");
		
		for (int i = 0; i < fdmax+1; i++) {
			//fprintf(stderr, "not so much Bruh\n");
			if (FD_ISSET(i, &tmpset)) {
				if (i == socket_id) { //richiesta di connessione
					SYSCALL(client_id, accept(socket_id, (struct sockaddr*)NULL, NULL), "accept");
					
					//da implementare, per ora mettiamoci il detatch TODO
					if (client_id > 0) {
						/*connection_stat *tcs = malloc(sizeof(connection_stat));
						tcs->socket_id = client_id;*/
						pthread_mutex_lock(&registration_mut);
						if (n_thread == len_tcv)
							CHECKNULL (tcv, realloc(tcv, sizeof(connection_stat*)*(len_tcv*=2)), "realloc");
						tcv[n_thread] = malloc(sizeof(connection_stat));
						tcv[n_thread]->socket_id = client_id;
						tcv[n_thread]->start = 0;
						tcv[n_thread]->end = 0;
						pthread_mutex_unlock(&registration_mut);
						
						pthread_create(&t, NULL, client_thread, tcv[n_thread++]);//magari usare il thread apposta per detatchare
						pthread_detach(t);
					}
					
				} else if (i == signal_pipe[0]) {
					char buf[N];
					if (read(signal_pipe[0], buf, N) == 0) { //la pipe è chiusa. Significa che è arrivata la richiesta di terminazione
						interrupt_end = 1;
						break;
					} else {
						if (!strncmp("USR1", buf, 5)) { //la pipe è aperta, è arrivato un messaggio che indica che è stata ricevuta SIGUSR1
							printstat(tcv, n_thread);
						}
					}
				}
			}
		}
	}
	//operazione da fare prima di chiudere il server per liberare la memoria
	PTHREADCALL (notused, pthread_mutex_destroy(&registration_mut), "destroy mutex");
	PTHREADCALL (notused, pthread_join(sigHandler_thread, NULL), "join sigHadler");
	for (int i = 0; i < n_thread; i++) {
		free(tcv[i]->name);
		free(tcv[i]);
	}
	free(tcv);
	close (socket_id);
	exit (EXIT_SUCCESS);
	return 0;
}

