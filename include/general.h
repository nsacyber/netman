#ifndef GENERAL_H
#define GENERAL_H

#include "minunit.h"
#include "errorcodes.h"
#include <fcntl.h> // open

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h> // close
#include <stdlib.h> // malloc
#include <stdio.h> // printf
#include <string.h> // strncmp

#include <net/bpf.h> // Berkley Packet Filters
#include <pthread.h>

#include <net/ethernet.h> // ether_header etc
#include <getopt.h>
#include <signal.h> // for kill

#include <sys/ioctl.h>
#include <net/if.h> // AF_LINK if_req etc

#include <spawn.h> // posix_spawn
#include <stdbool.h>

extern char **environ;

#include <errno.h>

struct list {
    void *content;
    struct list *next;
};
typedef struct list list;

struct doubly_list {
    struct doubly_list *prev;
    void *content;
    struct doubly_list *next;
};
typedef struct doubly_list doubly_list;

typedef enum COMMAND {
	BYTES,
	UP, 
	DOWN,
	MONITOR
} COMMAND;

#define println(...) { \
    fprintf(stdout, __VA_ARGS__);\
    fprintf(stdout, "\n");\
}

#define printERR(...) {\
	char buff[20];\
	time_t now = time(NULL);\
	strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));\
	fprintf(stderr, "[!] (%s): ", buff);\
	fprintf(stderr, __VA_ARGS__);\
	fprintf(stderr, " %s", strerror(errno));\
	fprintf(stderr, "\n");\
}

#define printVERBOSE(...) {\
	if(verbose_flag) {\
		char buff[20];\
		time_t now = time(NULL);\
		strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));\
		fprintf(stdout, "[-] (%s): ", buff);\
		fprintf(stdout, __VA_ARGS__);\
		fprintf(stdout, "\n");\
	}\
}


#ifdef DEBUG
#define printDEBUG(...) {\
		char buff[20];\
		time_t now = time(NULL);\
		strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));\
		fprintf(stdout, "[D] (%s): ", buff);\
		fprintf(stdout, __VA_ARGS__);\
		fprintf(stdout, "\n");\
}
#else 
#define printDEBUG(...) {}
#endif

u_int64_t bytesRead;// total number of bytes read from the bpf

int verbose_flag; 	// flag set by --verbose, --silent, --quite
int label_flag;   	// flag set by --label

pthread_mutex_t thread_mutex;
pthread_t threads[20];

int threadCount();

void version();
void usage();

pid_t runCmd(char *cmd);

void* monitor(void *ifname);
int open_dev_at(int start);
int open_dev(void);
int check_dlt(int fd, char *iface);
int set_options(int fd, char *iface);
int read_packets(int fd, char *iface);

#endif