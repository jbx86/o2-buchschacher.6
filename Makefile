all: oss user

oss: oss.c proj5.h
	gcc -o oss oss.c

user: user.c proj5.h
	gcc -o user user.c
