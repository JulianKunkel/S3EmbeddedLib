all: libS3.so

libS3.so: libS3-embedded.c
	gcc libS3-embedded.c -g3 -O3 -fPIC -Wall -shared -o libS3.so

clean:
	rm *.so *.o
