CC = gcc
CFLAGS = -Wall -g -Iinclude `pkg-config --cflags --libs glib-2.0`
LDFLAGS = `pkg-config --libs glib-2.0`

all: folders dserver dclient

folders:
	@mkdir -p src include obj bin tmp obj/extras

leitura_database: obj/extras/leitura_database.o obj/database.o
	$(CC) $^ $(LDFLAGS) -o scripts/$@

dserver: obj/dserver.o obj/database.o obj/cache.o obj/client_queue.o obj/task_queue.o obj/respostas.o
	$(CC) $^ $(LDFLAGS) -o $@

dclient: obj/dclient.o obj/cliparser.o obj/task_queue.o obj/respostas.o
	$(CC) $^ $(LDFLAGS) -o $@

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f obj/*.o obj/*/*.o tmp/* bin/* pipes/*.pipe dserver dclient