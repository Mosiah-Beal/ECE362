seek: seek.c
	gcc -std=c99 -ggdb -pthread -o seek seek.c

base: seeker-base.c
	gcc -std=c99 -ggdb -o base seeker-base.c

clean:
	rm -f seek base

