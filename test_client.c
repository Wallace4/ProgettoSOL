/*
 * test_insert.c
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


#include <stdio.h>
#include <client.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <util.h>

#define SUC "Successo!\n"
#define FAL "Fallimento!\n"
#define DNC	"dati non corrispondenti\n"

//funzione che genera n byte da inserire 
char *generateb4f (int n) {
	char *data = malloc (sizeof(char) * n);
	for (int i = 0; i < n; i++) {
		data[i] = '4';
		//data[i] = rand() % 255 +1;
	}
	return data;
}

int checkdata (char *data, int n) {
	for (int i = 0; i < n; i++) {
		if (data[i] != '4')
			return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "Uso: %s n_file output_file\n", argv[0]);
		return EXIT_FAILURE;
	}
	int request = strtol(argv[2], NULL, 10);
	int n_file = 20;
	int op_count = 0;
	int op_succ = 0;
	int op_fail = 0;
	int rtv_fail = 0;
	char *name = malloc(sizeof(char) *(strlen(argv[1])+1));
	memset(name, 0, strlen(argv[1])+1);
	memcpy(name, argv[1], strlen(argv[1]));
	//char *strforfile = malloc (sizeof(char)*N);
	
	//mi connetto al server
	fprintf(stdout, "%s:os_connect:", name);
	if (os_connect(name) == -1) {
		fprintf(stdout, "%s", FAL);
		return -1;
	} else
		fprintf(stdout, "%s", SUC);
	
	switch (request) {
		//mando n_file file al server
		case 1:
		for (int i = 0; i < n_file; i++) {
			char *file = malloc (sizeof(char)*100);
			sprintf(file, "file%.3d", i);
			int n = (i+1)*5000;
			char *data = generateb4f(n);
			op_count++;
			if (os_store(file, (void *)data, n) == -1) {
				op_fail++;
			} else {
				op_succ++;
			}
			free(file);
			free(data);
		}
		fprintf(stdout, "%s:%i os_store:%i riuscite:%i fallite\n", name, op_count, op_succ, op_fail);
		break;
		//recupero i dati per controllare che siano quelli che ho inviato
		case 2:
		for (int i = 0; i < n_file; i++) {
			char *file = malloc (sizeof(char)*100);
			sprintf(file, "file%.3d", i);
			int n = (i+1)*5000;
			char *data;
			op_count++;
			if ((data = (void *)os_retrieve(file)) == NULL) {
				op_fail++;
			} else {
				op_succ++;
				if (checkdata (data, n)) {
					rtv_fail++;
				}
				free(data);
			}
			free(file);
		}
		fprintf(stdout, "%s:%i os_retrieve:%i riuscite:%i inaspettate:%i fallite\n", name, op_count, op_succ, rtv_fail, op_fail);
		break;
		//elimino i file dispari 
		case 3:
		for (int i = 1; i < n_file; i+=2) {
			char *file = malloc (sizeof(char)*100);
			sprintf(file, "file%.3d", i);
			op_count++;
			if (os_delete(file) == -1) {
				op_fail++;
			} else {
				op_succ++;
			}
			free(file);
		}
		fprintf(stdout, "%s:%i os_delete:%i riuscite:%i fallite\n", name, op_count, op_succ, op_fail);
		break;
	}
	//mi disconnetto dal server
	fprintf(stdout, "%s:os_disconnect:", name);
	free(name);
	if (os_disconnect() == -1) {
		fprintf(stdout, "%s", FAL);
		return -1;
	} else {
		fprintf(stdout, "%s", SUC);
		return 0;
	}
}

