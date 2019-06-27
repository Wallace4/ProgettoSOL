/*
 * threadS.h
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


#ifndef _THREADS_H
#define _THREADS_H
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <util.h>

/**
 * \file threadS.h
 * \brief file contenente le firme delle funzioni pubbliche di threadS.c, i messaggi di errore predefiniti, e la definizione di connection_stat
 * \author Michele De Quattro
 */

/**
 * \def KODEF
 * \brief messaggio di errore per quando il server riceve una richiesta che non rispetta il protocollo
 */
#define KODEF "KO Messaggio_illeggibile\n"
/**
 * \def KOINT
 * \brief messaggio di errore per quando il server riceve un interruzione
 */
#define KOINT "KO interruzione_server\n"
/**
 * \def KOCAE
 * \brief messaggio di errore per quando il server riceve una richiesta di registrazione da parte di un utente già connesso
 */
#define KOCAE "KO another_connected_on_this_username\n"
/**
 * \def KOEAD
 * \brief messaggio di errore per quando il server non trova il file per cancellarlo o mandarlo al client
 */
#define KOEAD "KO file_not_found\n"
/**
 * \def KOGEN
 * \brief messaggio di errore per quando il server ha un errore generale che fa fallire l'operazione
 */
#define KOGEN "KO operazione_fallita\n"
/**
 * \def KOINE
 * \brief messaggio di errore per quando il server riceve una qualsiasi richiesta prima di una REGISTER
 */
#define KOINE "KO iscrizione_inesistente\n"

/**
 * \def DATAPATH
 * \brief il percorso dove verranno salvati i dati degli utenti
 * 
 * \warning questo percorso dipende da dove viene eseguito l'eseguibile. è questo perché ho sempre richiamato ./bin/server nella cartella iniziale del progetto. Se si vuole chiamare server stando dentro la cartella bin allora questa define deve cambiare in ../data/
 */
#define DATAPATH "./data/" //dipende da dove viene eseguito l'eseguibile

/**
 * \typedef connection_stat
 * \brief struttura che rappresenta vari valori importanti di una connessione
 */
typedef struct {
	int socket_id;	///<id della connessione
	char *name;		///<nome della registrazione
	time_t start;	///<quando è iniziata la connessione
	time_t end;		///<quando è terminata la connessione
} connection_stat;

/**
 * \var registration_mut
 * \brief mutex per l'accesso in mutua esclusione a \a tcv
 */
pthread_mutex_t registration_mut;
/**
 * \var reg_cond
 * \brief variabile di condizione per addormentarsi nel caso ci sia già qualcuno connesso con il tuo stesso utente
 */
pthread_cond_t reg_cond;

/**
 * \def RWCHECK(r, s, e, c)
 * \brief controlla il valore di ritorno di \a s, e in caso di errore printa il messaggio \a e ed esegue il comando \a c
 */
#define RWCHECK(r, s, e, c) \
	if ((r = s) == 0) {fprintf(stderr, "Connessione socket interrotta: %s\n", e); c;} \
	else if (r == -1) {perror(e); c;}

/**
 * \fn client_thread (void *arg)
 * \brief la funzione che rappresenta il thread che gestisce il client connesso
 * \param arg l'argomento del thread, castato subito a connection stat *. Contiene quindi le varie statistiche del thread, compreso il socket id
 * \return NULL
 */
void *client_thread (void *arg);

#endif
