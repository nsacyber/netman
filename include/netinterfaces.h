#ifndef NETINTERFACES_H
#define NETINTERFACES_H

#include <netinet/in.h> // IPPROTO_TCP
#include <ifaddrs.h>

struct interface {
	char *name;
	struct sockaddr *if_addr;
	u_long obytes;
	u_long ibytes;
};
typedef struct interface interface;

void interfaces(list **interfaces);
void freeInterfaces(list **interfaces);

int turnOffInterfaces(list *interfaces);

int set_if_up(char *ifname, short flags);
int set_up(struct interface *i);
int set_if_down(char *ifname, short flags);
int set_down(struct interface *i);

int getInterfaceStatus(char *interface);
int isInterfaceUp(char *interface);

int loopInterfaces(list *interfaces, int (*f)(struct interface *));
int print(struct interface *i);

#endif
