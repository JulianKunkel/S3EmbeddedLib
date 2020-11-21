FLAGS=-g3 -O3 -Wall

all: libS3e.so libS3r.so libS3gw

libS3e.so: libS3-embedded.c
	gcc libS3-embedded.c -fPIC $(FLAGS) -shared -o libS3e.so

libS3gw: libS3-gw.c
	gcc libS3-gw.c $(FLAGS) -o libS3gw -lpthread -I.

libS3r.so: libS3-remote.c
	gcc libS3-remote.c $(FLAGS) -fPIC -shared -o libS3r.so -I.

clean:
	rm *.so *.o
