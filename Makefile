FLAGS=-g3 -O3 -Wall $(CFLAGS)

all: libS3e.so libS3r.so libS3gw

libS3e.so: libS3-embedded.c libS3-gw.h
	gcc libS3-embedded.c -fPIC $(FLAGS) -shared -o libS3e.so -I.

libS3gw: libS3-gw.c libS3e-util.c libS3-gw.h
	gcc libS3-gw.c libS3e-util.c $(FLAGS) -o libS3gw -lpthread -I.

libS3r.so: libS3-remote.c libS3e-util.c libS3-gw.h
	gcc libS3e-util.c libS3-remote.c $(FLAGS) -fPIC -shared -o libS3r.so -I.

clean:
	rm *.so *.o libS3gw
