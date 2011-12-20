object_files = obj/loadtest.o obj/utils.o obj/tcp.o obj/test.o

loadtest: $(object_files)
	gcc -lpthread -o loadtest $(object_files)

obj/loadtest.o: src/loadtest.c obj/utils.o obj/tcp.o
	gcc -c src/loadtest.c -o obj/loadtest.o

obj/utils.o: src/utils.c src/utils.h
	gcc -c src/utils.c -o obj/utils.o

obj/tcp.o: src/tcp.c src/tcp.h
	gcc -c src/tcp.c -o obj/tcp.o

obj/test.o: obj/tcp.o obj/utils.o src/test.c src/test.h
	gcc -c src/test.c -o obj/test.o

clean:
	rm loadtest $(object_files)
