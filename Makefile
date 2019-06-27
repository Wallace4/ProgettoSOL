#comandi di compilazione
CC		=gcc -std=c11
#flag di compilazione
CFLAGS	=-g -Wall
#flag di thread
LPFLAGS	=-lpthread
#flag di ottimizzazione
OPTFLAGS=-O3
#dove si trovano gli include (non di sistema)
INCLUDES=-I ./includes/
#dove si trovano le librerie
LDFLAGS	=-L ./bin/
#comandi per la compilazione statica
ARFLAGS	=ar rvs
#directory dei file
LIBDIR	=lib/
SRVDIR	=server/
CLTDIR	=client/
BINDIR	=bin/
INCDIR	=includes/
#file da linkare
LIBFILE	=client.c threadS.c
LIBS	=$(addprefix $(LIBDIR),$(LIBFILE))
STATLIBS=$(BINDIR)libclient.a
OBJS	=$(addprefix $(BINDIR),$(LIBFILE:.c=.o))
#file di codice sorgente
SOURCES	=server.c
BINS	=$(addprefix $(BINDIR),$(SOURCES:.c=))
#file di test (diventeranno test1, test2 e test3
TEST	=test_client.c

.PHONY: all clean test

#comandi compilazione
$(BINDIR)server: $(SRVDIR)server.c $(BINDIR)threadS.o
	$(CC) $(CFLAGS) $(LPFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $^ $(LDFLAGS)

$(BINDIR)%.o: $(LIBDIR)%.c $(INCDIR)%.h
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -c -o $@ $<

$(BINDIR)lib%.a: $(BINDIR)%.o
	$(ARFLAGS) $@ $^

%: %.c $(BINDIR)libclient.a
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS) -lclient

all: $(BINS) $(OBJS) $(TEST:.c=) $(STATLIBS)

clean: 
	\rm -f *~ $(BINS) $(OBJS) $(TEST:.c=) $(STATLIBS)

test:
	echo "Starting Tests" 1>testout.log
	num=1 ; while [[ $$num -le 50 ]] ; do \
		printf -v j "%03d" $$num ; \
		( ./test_client user$$j 1 1>>testout.log &); \
		((num = num + 1)) ; \
	done
	wait
	num=1 ; while [[ $$num -le 30 ]] ; do \
		printf -v j "%03d" $$num ; \
		( ./test_client user$$j 2 1>>testout.log &); \
		((num = num + 1)) ; \
	done
	num=31 ; while [[ $$num -le 50 ]] ; do \
		printf -v j "%03d" $$num ; \
		( ./test_client user$$j 3 1>>testout.log &); \
		((num = num + 1)) ; \
	done
