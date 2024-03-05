seek: seek.c
	gcc -ggdb -pthread -o seek seek.c

base: seeker-base.c
	gcc -ggdb -o base seeker-base.c



