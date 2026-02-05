#ifndef SHIM_H
#define SHIM_H
#include <process.h> 
#include <direct.h> 

/* Translate Linux 'mkdir' (2 args) to Windows '_mkdir' (1 arg) */
#define mkdir(path, mode) _mkdir(path)

/* Windows doesn't have 'fork' (daemon mode). Disable it by returning 0. */
#define fork() 0


/* Mock other missing Linux concepts */
#define getuid() 0
#define flock(fd, op) 0
#define LOCK_EX 1
#define LOCK_NB 2
#endif
