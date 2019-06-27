/*
 * client.c
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


#include <util.h>
#include <client.h>

/**
 * \file client.c
 * \brief codice sorgente per le funzioni del client che accedono all'object-store
 * \author Michele De Quattro
*/

/**
 * \var socket_id
 * \brief il fd del socket di connessione al server
 * 
 * \warning non è thread safe
 */
int socket_id = -1;
/**
 * \var notused
 * \brief variabile usata per salvare i valori di ritorno di read e/o write
 */
int notused;

/**
 * \fn responce_handling (char *responce_msg)
 * \brief funzione che dato il messaggio di risposta del server, lo controlla e reagisce di conseguenza
 * \param responce_msg la stringa arrivata dal server contenente il messaggio di risposta
 * \return 0 se OK
 * \return -1 se KO o messaggio illeggibile
 */
int responce_handling (char *responce_msg) {
	char *tmpstr;
	char *token = __strtok_r(responce_msg, " \n", &tmpstr);
	if (!strncmp(OK, token, strlen(OK))) {
		free(responce_msg);
		return 0; 
	} else if (!strncmp(KO, token, strlen(KO))) {
		token = __strtok_r(NULL, "\n", &tmpstr);
		fprintf(stderr, "%s\n", token);
		free(responce_msg);
		return -1;
	} else {
		fprintf(stderr, "Errore non standardizzato\n");
		free(responce_msg);
		return -1;
	} 
}

int os_connect (char *name) {
	if (socket_id != -1) {
		fprintf(stderr, "Connessione già stabilita\n"); //magari settiamo errno se ci riusciamo TODO
		return -1;
	} else {
		struct sockaddr_un server_address;
		//memset (&server_address, '0', sizeof(server_address)); //??????
		
		strncpy (server_address.sun_path, SOCKNAME, sizeof(SOCKNAME)+1);
		server_address.sun_family = AF_UNIX;
		
		SYSCALL (socket_id, socket(AF_UNIX, SOCK_STREAM, 0), "Socket client");
		
		while (connect(socket_id, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
			if (errno == ENOENT) //se non trova la socket, allora aspetta che venga creata. In realtà preferirei ci fosse un altro modo, e forse lo implementerò TODO
				sleep(1); 
			else
				return EXIT_FAILURE;
		}
		
		//Mando il messaggio di Registrazione
		char *rgs_msg = malloc (sizeof(char)*(strlen(RGS)+strlen(name)+3));
		char *responce_msg = malloc (sizeof(char)*N);
		memset (responce_msg, 0, N);
		sprintf(rgs_msg, "%s %s\n", RGS, name);
		
		RWCHECK(notused, writen(socket_id, rgs_msg, strlen(rgs_msg)), "writen os_connect");
		//attendo che mi risponda con una read.
		//readn(socket_id, responce_msg, N);
		RWCHECK(notused, read(socket_id, responce_msg, N), "readn os_connect");
		
		free(rgs_msg);
		if (responce_handling(responce_msg) == -1) {
			close (socket_id);
			socket_id = -1;
			return -1;
		} else 
			return 0;
	}
}

int os_store (char *name, void *block, size_t len) {
	if (socket_id == -1) {
		fprintf(stderr, "Connessione ancora da stabilire\n");
		return -1;
	} else {
		char *str_msg = malloc (sizeof(char) *N);
		char *responce_msg = malloc (sizeof(char) *N);
		memset(responce_msg, 0, N);
		sprintf(str_msg, "%s %s %li\n", STR, name, len);
		
		RWCHECK(notused, writen(socket_id, str_msg, strlen(str_msg)), "writen os_store");
		//RWCHECK(notused, read(socket_id, responce_msg, N), "readn os_store ping pong");
		RWCHECK(notused, writen(socket_id, (char*)block, len), "writen os_store data");
		
		//readn(socket_id, responce_msg, N);
		RWCHECK(notused, read(socket_id, responce_msg, N), "readn os_store");
		free(str_msg);
		return responce_handling(responce_msg);
	}
}

void *os_retrieve (char *name) {
	if (socket_id == -1) {
		fprintf(stderr, "Connessione ancora da stabilire\n");
		return NULL;
	} else {
		char *rtv_msg = malloc (sizeof(char) *N);
		char *responce_msg = malloc (sizeof(char) *N+1);
		memset(responce_msg, 0, N+1);
		char *data;
		sprintf(rtv_msg, "%s %s\n", RTV, name);
		RWCHECK(notused, writen(socket_id, rtv_msg, strlen(rtv_msg)), "writen os_retrieve");
		
		//readn(socket_id, responce_msg, N);
		memset(responce_msg, 0, N);
		RWCHECK(notused, read(socket_id, responce_msg, N), "readn os_retrieve");
		free(rtv_msg);
		
		char *tmpstr;
		char *token = __strtok_r(responce_msg, " \n", &tmpstr);
		if (!strncmp("DATA", token, 4)) {
			token = __strtok_r(NULL, "\n", &tmpstr);
			int len = strtol(token, NULL, 10);
			data = malloc (sizeof(char) *len+1);
			memset(data, 0, len);
			token = __strtok_r(NULL, "", &tmpstr); //valgrind dice che c'è un problema qui ma io boh
			if (token != NULL) {
				int n = strlen(token);
				memcpy(data, token, n); 
				if ((len-n) > 0) {
					RWCHECK(notused, readn(socket_id, data+n, len-n), "readn os_retrieve data");
				}
			} else {
				RWCHECK(notused, readn(socket_id, data, len), "readn os_retrieve data");
			}
			free(responce_msg);
			return (void *)data;
		} else if (!strncmp(KO, token, strlen(KO))) {
			token = __strtok_r(NULL, "\n", &tmpstr);
			fprintf(stderr, "%s\n", token);
			free(responce_msg);
			return NULL;
		} else {
			fprintf(stderr, "Errore non standardizzato\n");
			free(responce_msg);
			return NULL;
		} 
	}
}

int os_delete (char *name) {
	if (socket_id == -1) {
		fprintf(stderr, "Connessione ancora da stabilire\n");
		return -1;
	} else {
		char *dlt_msg = malloc (sizeof(char) *N);
		char *responce_msg = malloc (sizeof(char) *N);
		memset(responce_msg, 0, N);
		sprintf(dlt_msg, "%s %s\n", DLT, name);
		RWCHECK(notused, writen(socket_id, dlt_msg, strlen(dlt_msg)), "writen os_delete");
		
		//readn(socket_id, responce_msg, N);
		RWCHECK(notused, read(socket_id, responce_msg, N), "readn os_delete");
		free(dlt_msg);
		return responce_handling(responce_msg);
	}
}

int os_disconnect() {
	if (socket_id == -1) { //nessuna connessione esistente.
		fprintf(stderr, "Nessuna connessione esistente da chiudere\n");
		return 0;
	} else {
		char *lve_msg = malloc (sizeof(char)*(strlen(LVE)+2));
		char *responce_msg = malloc (sizeof(char) *N);
		memset(responce_msg, 0, N);
		sprintf(lve_msg, "%s\n", LVE);
		RWCHECK(notused, writen(socket_id, lve_msg, strlen(lve_msg)), "writen os_disconnect");
		
		free(lve_msg);
		//readn(socket_id, responce_msg, N);
		RWCHECK(notused, read(socket_id, responce_msg, N), "readn os_disconnect");
		socket_id = -1;
		return responce_handling(responce_msg);
	}
}
