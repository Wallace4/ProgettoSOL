/*
 * client.h
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


#ifndef _CLIENT_H
#define _CLIENT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <util.h>

/**
 * \file client.h
 * \brief file contenente le firme delle funzioni pubbliche di client.c
 * \author Michele De Quattro
*/

/**
 * \def RWCHECK(r, s, e)
 * \brief macro che controlla se la funzione \a s (una read o una write) ha ritornato un errore, e a seconda del valore di ritorno printa in modo diverso \a e
 */
#define RWCHECK(r, s, e) \
	if ((r = s) == 0) {fprintf(stderr, "Connessione socket interrotta: %s\n", e); exit(EXIT_FAILURE);} \
	else if (r == -1) {perror(e); exit (EXIT_FAILURE);}

/**
 * \fn os_connect (char *name)
 * \brief funzione che connette il client al server
 * \param name il nome dell'utente che si vuole connettere
 * \return socket_id l'id del socket che rappresenta la connessione (forse no)
 * \return 0 se la connessione è stata stabilita con successo
 * \return -1 se la connessione fallisce
 */
int os_connect (char *name);

/**
 * \fn os_store (char *name, void *block, size_t len)
 * \brief funzione che manda un file al server
 * \param name il nome del file che si sta mandando
 * \param block puntatore al blocco di dati che si vuole mandare
 * \param len la lunghezza, in byte, del blocco
 * \return 0 se l'operazione è andata a buon fine
 * \return -1 altrimenti
 */
int os_store (char *name, void *block, size_t len);

/**
 * \fn *os_retrieve (char *name)
 * \brief funzione che richiede un file al server
 * \param name il nome dell'oggetto che si richiede
 * \return block il blocco di dati richiesto
 * \return NULL se il blocco non è presente nel server
 */
void *os_retrieve (char *name);

/**
 * \fn os_delete (char *name)
 * \brief funzione che cancella un dato dal server
 * \param name il nome dell'oggetto che si desidera cancellare
 * \return 0 se l'oggetto viene cancellato con successo
 * \return -1 se c'è stato un errore
 */
int os_delete (char *name);

/**
 * \fn os_disconnect
 * \brief funzione che disconnette dal server, e termina la connessione
 * \return 0 se la connessione è stata interrotta con successo
 * \return -1 se ci sono stati degli errori
 */
int os_disconnect();

#endif
