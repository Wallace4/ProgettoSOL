/*
 * threadS.c
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


#include <threadS.h>

/**
 * \file threadS.c
 * \brief codice sorgente per il thread che gestisce la connessione con il client lato server
 * \author Michele De Quattro
*/

//variabili esterne del server
extern volatile int interrupt_end;
extern connection_stat **tcv;
int notused; ///<variabile usata per salvare i valori di ritorno di read e/o write

/**
 * \fn register_user (int client_socket, char *tmpstr)
 * \brief funzione che registra l'utente che ha richiesto la connessione nel server
 * \param client_socket il socket che descrive la connessione con il server
 * \param tmpstr la stringa usata dalla __strtok_r dove è salvato il resto del messaggio. ci serve per continuare le __strtok_r
 * \return path il percorso del file dove verranno salvati tutti i dati dell'utente
 * \return NULL in caso di errore
 */
char *register_user (int client_socket, char *tmpstr) {
	struct stat st = {0};
	char *token = __strtok_r(NULL, " \n", &tmpstr);
	char *path = malloc (sizeof(char) * (strlen(token)+strlen(DATAPATH)+2));
	sprintf(path, "%s%s/", DATAPATH, token);
	
	if (stat(path, &st) == -1) { //la cartella non esiste
		if (mkdir(path, 0700) == -1) { 
			if (errno == EINTR) { 
				if (interrupt_end) { //se ho un interruzione allora per lasciare in uno stato consistente il server cancello quel poco che ero riuscito a creare della cartella
					if (unlink(path) == -1) {
						perror ("Unlink RGS");
					} 
					free(path);
					free(token);
					return NULL;
				}
			}
			perror("mkdir");
			RWCHECK(notused, writen(client_socket, KOGEN, sizeof(KOGEN)+1), "writen RGS", {return NULL;}); 
			return NULL;
		}
	} else { //la cartella esiste, dobbiamo vedere se c'è una connessione attiva con la stessa registrazione.
		for (int i = 0; i < 0; i++) {
			pthread_mutex_lock(&registration_mut);
			if (tcv[i]->socket_id > 0 && tcv[i]->name != NULL && !strcmp(path, tcv[i]->name)) {
				pthread_cond_wait(&reg_cond, &registration_mut);
				i--;
			}
			pthread_mutex_unlock(&registration_mut);
		}
	}
	writen(client_socket, "OK\n", 4);
	return path;
}

/**
 * \fn store_data (int client_socket, char* tmpstr, char *path)
 * \brief la funzione che salva il file mandato dal client nel server
 * \param client_socket il socket che descrive la connessione con il server
 * \param tmpstr la stringa usata dalla __strtok_r dove è salvato il resto del messaggio. ci serve per continuare le __strtok_r
 * \param path il percorso dedicato all'utente dove è possibile salvare il file
 * \return 0 in caso di successo
 * \return -1 in caso di errore
 */
int store_data (int client_socket, char* tmpstr, char *path) {
	char *filename;
	char *bin_data;
	char *token = __strtok_r(NULL, " \n", &tmpstr);
	if (path != NULL) { //allora la registrazione è avvenuta
		filename = malloc (sizeof(char)*(strlen(token)+strlen(path)+5));
		int f;
		sprintf(filename, "%s%s.bin", path, token);
		token = __strtok_r(NULL, " \n", &tmpstr);
		int len = atoi (token);
		
		bin_data = malloc (len*sizeof(char));
		memset(bin_data, 0, len);
		token = __strtok_r(NULL, "", &tmpstr);
		if (token != NULL) {
			int l = strlen(token);
			memcpy(bin_data, token, l);
			if ((len-l) > 0) {
				RWCHECK(notused, readn(client_socket, bin_data+l, len-l), "readn STR", {return -1;});
			}
		} else {
			RWCHECK(notused, readn(client_socket, bin_data, len), "readn STR", {return -1;});
		}
		if ((f = open(filename, O_CREAT | O_WRONLY, 0660)) == -1) { //Non uso la macro SYSCALL perché lei fa exit e non return -1
			perror("open file STR");
			return -1;
		}
		int n;
		char *buf_ptr = bin_data;
		int len2 = len;
		while (len2 > 0) {
			if ((n = write(f, buf_ptr, len2)) == -1) { 
				close (f);
				if (unlink(filename) == -1) 
					perror("unlink STR");
				if (errno == EINTR) {
					if (interrupt_end) {
						RWCHECK(notused, writen(client_socket, KOINT, strlen(KOINT)+1), "writen STR KOINT", {return-1;});
					} 
				} else {
					perror("write su file");
					RWCHECK(notused, writen(client_socket, KOGEN, strlen(KOGEN)+1), "writen STR KOGEN", {return-1;});
				}
				free(filename);
				free(bin_data);
				return -1;
			}
			buf_ptr += n; //se c'è un interruzione che non fa chiudere il server allora dobbiamo continuare a scrivere su file
			len2 -= n;
		}
		RWCHECK(notused, writen(client_socket, "OK\n", 4), "writen STR", {return -1;});
		close (f);
		free(filename);
		free(bin_data);
	} else {
		RWCHECK(notused, writen(client_socket, KOINE, strlen(KOINE)+1),"writen STR bad ending", {return -1;});
	}
	return 0;
}

/**
 * \fn retrieve_data (int client_socket, char *tmpstr, char *path)
 * \brief la funzione che manda al client il file che ha richiesto
 * \param client_socket il socket che descrive la connessione con il server
 * \param tmpstr la stringa usata dalla __strtok_r dove è salvato il resto del messaggio. ci serve per continuare le __strtok_r
 * \param path il percorso dedicato all'utente dove è possibile salvare il file
 * \return 0 in caso di successo
 * \return -1 in caso di errore
 */
int retrieve_data (int client_socket, char *tmpstr, char *path) {
	char *filename;
	char *bin_data;
	char *exit_msg;
	struct stat st = {0};
	char *token = __strtok_r(NULL, " \n", &tmpstr);
	
	if (path != NULL) { //la registrazione è avvenuta
		filename = malloc(sizeof(char)*(strlen(token)+strlen(path)+5));
		int f;
		//FILE *f;
		sprintf(filename, "%s%s.bin", path, token);
		stat(filename, &st);
		int len = st.st_size;
		exit_msg = malloc(sizeof(char)*100); //non va bene 10, uff. TODO
		sprintf(exit_msg, "DATA %i\n", len);
		bin_data = malloc (len*sizeof(char));//non dovrebbe esserci bisogno di memset, perché va sovrascritta subito -CIT
		
		if ((f = open(filename, O_RDONLY)) == -1) {
			if (errno == ENOENT) {
				RWCHECK(notused, writen(client_socket, KOEAD, strlen(KOEAD)+1), "writen RTV", {return -1;});
				return 0;
			} else {
				perror("apertura file RTV");
				RWCHECK(notused, writen(client_socket, KOGEN, strlen(KOGEN)+1), "writen RTV", {return -1;});
				fprintf (stderr, "umhhhhhh");
				return -1;
			}
		}
		RWCHECK(notused, readn(f, bin_data, len), "readn RTV da file", {return-1;});
		
		RWCHECK(notused, writen(client_socket, exit_msg, strlen(exit_msg)), "write RTV exitmsg", {return -1;});
		//RWCHECK(notused, readn(client_socket, exit_msg, 4), "readn RTV ping pong", {return -1;});
		RWCHECK(notused, writen(client_socket, bin_data, len), "write RTV", {return -1;});
		
		close (f);
		free(filename);
		free(exit_msg);
		free(bin_data);
	} else {
		RWCHECK(notused, writen(client_socket, KOINE, strlen(KOINE)+1), "writen RTV bad ending", {return -1;});
	}
	return 0;
}

/**
 * \fn delete_data (int client_socket, char *tmpstr, char *path)
 * \brief la funzione che cancella dal server il file richeisto dal client
 * \param client_socket il socket che descrive la connessione con il server
 * \param tmpstr la stringa usata dalla __strtok_r dove è salvato il resto del messaggio. ci serve per continuare le __strtok_r
 * \param path il percorso dedicato all'utente dove è possibile salvare il file
 * \return 0 in caso di successo
 * \return -1 in caso di errore
 */
int delete_data (int client_socket, char *tmpstr, char *path) {
	char *filename;
	char *token = __strtok_r (NULL, " \n", &tmpstr);
	if (path != NULL) {
		filename = malloc(sizeof(char)*(strlen(token)+strlen(path)+5));
		sprintf(filename, "%s%s.bin", path, token);
		//sostitire system con un execl del comando
		if (unlink(filename) == -1) {
			if (errno == ENOENT) {
				RWCHECK(notused, writen(client_socket, KOEAD, strlen(KOEAD)+1), "writen DLT", {return -1;});
				return 0;
			} else {
				perror ("unlink delete");
				RWCHECK(notused, writen(client_socket, KOGEN, strlen(KOGEN)+1), "writen DLT worst ending", {return -1;});
				return -1;
			}
		}
		RWCHECK(notused, writen(client_socket, "OK\n", 4), "writen DLT", {return -1;});
		free(filename);
	} else {
		RWCHECK(notused, writen(client_socket, KOINE, strlen(KOINE)+1), "writen DLT bad ending", {return -1;});
	}
	return 0;
}

void *client_thread (void *arg) { 
	//dichiarazione variabili;
	connection_stat *tcs = (connection_stat *)arg;
	int socket_id = tcs->socket_id;
	char *path = NULL;
	char *msg = malloc(sizeof(char)*N+1);
	msg[N] = 0;
	char *tmpstr;
	char *token;
	int end = 0;
	
	//ciclo di gestione richieste
	while (!end) {
		//lettura richiesta da parte 
		memset(msg, 0, N);
		RWCHECK(notused, read(socket_id, msg, N), "readn client_thread", {close(socket_id); pthread_exit(NULL);});
		token = __strtok_r(msg, " \n", &tmpstr);
		
		//valutazione 
		if (!strncmp(RGS, token, strlen(RGS))) { //Richiesta di registrazione da parte del client
			if ((path = register_user(socket_id, tmpstr)) == NULL) {
				end = 1;
			}
			pthread_mutex_lock(&registration_mut);
			//memcpy(tcs->name, path, strlen(path));
			tcs->name = path;
			tcs->start = time(NULL);
			pthread_mutex_unlock(&registration_mut);
		} else if (!strncmp(LVE, token, strlen(LVE))) { //richiesta di chiusura connessione da parte del client
			//fprintf(stderr, "Inizio disconnessione\n");
			end = 1;
			RWCHECK(notused, writen(socket_id, "OK\n", 4), "writen LVE", {close(socket_id); pthread_exit(NULL);});
			//fprintf(stderr, "fine disconnessione\n");
		} else if (!strncmp(STR, token, strlen(STR))) { //richiesta di deposito dati da parte del client
			//fprintf(stderr, "Inizio STR\n");
			if (store_data(socket_id, tmpstr, path) == -1) {
				end = 1;
			}
		} else if (!strncmp(RTV, token, strlen(RTV))) { //richiesta di recupero dati da parte del client
			//fprintf(stderr, "Inizio RTV\n");
			if (retrieve_data(socket_id, tmpstr, path) == -1) {
				end = 1;
			}
		} else if (!strncmp(DLT, token, strlen(DLT))) { //richiesta di cancellazione da parte del client
			if (delete_data(socket_id, tmpstr, path) == -1) {
				end = 1;
			}
		} else {
			writen(socket_id, KODEF, strlen(KODEF)+1);
		}
		memset(token, 0, strlen(token));
	}
	free(msg);
	close (socket_id);
	pthread_mutex_lock(&registration_mut);
	tcs->socket_id = -1;
	tcs->end = time(NULL);
	pthread_mutex_unlock(&registration_mut);
	pthread_cond_signal(&reg_cond);
	pthread_exit(NULL);
}

