/*
 * util.h
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

#ifndef _UTIL_H
#define _UTIL_H
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>

/**
 * \file util.h
 * \brief file contenente define, strutture, macro e funzioni utili per tutto il codice
 * \author Michele De Quattro
 */

/**
 * \def SOCKNAME
 * \brief il nome della socket che viene usata per la connessione client/server
 */
#define SOCKNAME "./objstore.sock"

//define per le varie operazioni
/**
 * \def RTV
 * \brief definisce l'operazione RETRIEVE
 */
#define RTV	"RETRIEVE"
/**
 * \def RGS
 * \brief definisce l'operazione REGISTER
 */
#define RGS	"REGISTER"
/**
 * \def STR
 * \brief definisce l'operazione STORE
 */
#define STR	"STORE"
/**
 * \def DLT
 * \brief definisce l'operazione DELETE
 */
#define DLT	"DELETE"
/**
 * \def LVE
 * \brief definisce l'operazione LEAVE
 */
#define LVE	"LEAVE"
/**
 * \def OK
 * \brief definisce il messaggio di ritorno OK
 */
#define OK	"OK"
/**
 * \def KO
 * \brief definisce il messaggio di ritorno KO
 */
#define KO	"KO"

/**
 * \def N
 * \brief definisce il valore standard di lettura/scrittua dove non si sà la lunghezza precisa
 */
#define N	256 //variabile per lo scambio di dati con read e write, da tenere ad un minimo di 10, che è la lunghezza minima del messaggio di richiesta più lungo (RETRIEVE n\n)

/**
 * \def SYSCALL
 * \brief macro che controlla il valore di ritorno di \a c, ed in caso di errore esce con errno
 */
#define SYSCALL(r,c,e) \
	if((r=c)==-1) { perror(e);exit(errno); }
/**
 * \def CHECKNULL
 * \brief macro che controlla il valore di ritorno di \a c, ed in caso di errore esce con errno
 */
#define CHECKNULL(r,c,e) \
	if ((r=c)==NULL) { perror(e);exit(errno); }
/**
 * \def PTHREADCALL
 * \brief macro che controlla il valore di ritorno delle chiamate dei thread, ed in caso di errore esce
 */
#define PTHREADCALL(r, c, e) \
	if ((r=c)!=0) {fprintf(stderr, e);exit(EXIT_FAILURE); }

/**
 * \typedef sigHandler_t
 * \brief struttura contenente una maschera di signal ed una pipe. Serve per il thread sigHandler
 */
typedef struct {
	sigset_t *set;	///<la maschera contenente le signal che devono essere gestite dal thread
	int pipe;		///<la pipe in comunicazione con il server per comunicare quale signal è arrivata
} sigHandler_t;

/**
 * \fn readn
 * \brief funzione per sostituire la read e far in modo che continui la lettura nonostante un interruzione da signal
 * \param fd il file descriptor da dove si deve leggere
 * \param buf il buffer dove vengono scritti i valori letti
 * \param size la lunghezza di lettura massima
 * \return len la lunghezza del file letto
 * \return 0 se la connessione viene interrotta
 * \return -1 in caso di errore
 */
static inline int readn (long fd, void *buf, size_t size) {
	size_t left = size;
	int r;
	char *bufptr = (char*) buf;
	int end = 0;
	while (left > 0 && !end) {
		if ((r = read((int)fd, bufptr, left))==-1) { //da finire
			if (errno == EINTR) continue;
			else return -1;
		} else if (r == 0) return 0; //chiusura socket
		bufptr += r;
		left -= r;
		if (*(bufptr-1) == '\n')
			end = 1;
	}
	return size;
}

/**
 * \fn writen
 * \brief funzione per sostituire la read in modo che continui la scrittura nonostante venga interrotta da una signal
 * \param fd il file descriptor da dove si deve leggere
 * \param buf il buffer dove vengono presi i dati da scrivere sul fd
 * \param size la lunghezza del buffer, o lunghezza massima di scrittura
 * \return 1 in caso di successo
 * \return 0 se la connessione viene interrotta
 * \return -1 in caso di errore
 */
static inline int writen (long fd, void *buf, size_t size) {
	size_t left = size;
	int r;
	char *bufptr = (char*) buf;
	while (left > 0) {
		if ((r = write((int)fd, bufptr, left))==-1) { //da finire
			if (errno == EINTR) continue;
			else return -1;
		} else if (r == 0) return 0; //chiusura socket
		bufptr += r;
		left -= r;
	}
	return 1;
}

#endif
