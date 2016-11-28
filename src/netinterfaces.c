#include "general.h"
#include "netinterfaces.h"

/** 
 * http://stackoverflow.com/questions/3055622/howto-check-a-network-devices-status-in-c
 * - parameter interface: name of the network interface
 * - returns: 0 if up, otherwise 1, negative number on error
 */
int getInterfaceStatus(char *interface) {
	if(!interface) return ERR_NULL;

    int fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(fd < 0) {
		printERR("Unable to create socket.");
		return ERR_SOCKET;
	}

    struct ifreq ethreq;
    memset(&ethreq, 0, sizeof(ethreq));
    strncpy(ethreq.ifr_name, interface, IFNAMSIZ);
    // Get or set the active flag word of the device.
    int res = ioctl(fd, SIOCGIFFLAGS, &ethreq);
    if(res < 0) {
    	printERR("Unable to check %s's status.", interface);
    	return res;
    }

    if (ethreq.ifr_flags & IFF_UP) {
        close(fd);
        return 0;
    }
    close(fd);
    return 1;
}

/**
 * - parameter interface: name of interface
 * - returns: true if interface is up, false otherwise
 */
int isInterfaceUp(char *interface) {
	return getInterfaceStatus(interface) == 0 ? true : false;
}

/**
 * sets a list of network interfaces using `getifaddrs`
 * should call freeInterfaces on the list after calling
 * - parameter interfaces: a linked list of network interfaces of type `struct interface`
 */
void interfaces(list **interfaces) {
    struct ifaddrs *ifap, *itmp;
    getifaddrs(&ifap);
    list *prev = NULL;
    for(itmp = ifap; itmp; itmp = itmp->ifa_next) {
      
      if (itmp->ifa_data != NULL && itmp->ifa_addr->sa_family == AF_LINK) {
      	list *tmp = malloc(sizeof(list));

        tmp->content = calloc(sizeof(struct interface),1);
        struct if_data *data = itmp->ifa_data;

        ((struct interface *)tmp->content)->name = malloc(strnlen(itmp->ifa_name, MAXCOMLEN));
        strncpy(((struct interface *)tmp->content)->name, itmp->ifa_name, MAXCOMLEN);

        ((struct interface *)tmp->content)->if_addr = malloc(sizeof(struct sockaddr));
        memcpy(((struct interface *)tmp->content)->if_addr, itmp->ifa_addr, sizeof(struct sockaddr));

        ((struct interface *)tmp->content)->obytes = data->ifi_obytes;
        ((struct interface *)tmp->content)->ibytes = data->ifi_ibytes;
        
        tmp->next = NULL;
        if(*interfaces == NULL) {
        	#ifdef DEBUG
        		struct if_data *data = itmp->ifa_data;
        		printDEBUG("name %s ibytes: %u obytes: %u %p", itmp->ifa_name, data->ifi_ibytes, data->ifi_obytes, tmp);
        	#endif
            *interfaces = tmp;
            prev = tmp;
        } else {
        	#ifdef DEBUG
        		struct if_data *data = itmp->ifa_data;
        		printDEBUG("name %s ibytes: %u obytes: %u %p %p", itmp->ifa_name, data->ifi_ibytes, data->ifi_obytes, tmp, prev);
        	#endif
            prev->next = tmp;
            prev = tmp;
        }
      }
    }
    freeifaddrs(ifap);
}

/** 
 * free a struct interface
 * - parameter i: newtork interface to free
 */
void freeInterface(struct interface *i) {
	if(i->name) {
		free(i->name);
	}
	if(i->if_addr) {
		free(i->if_addr);
	}
}

/**
 * free a list of interfaces
 * - paramter interfaces: list of interfaces to free
 */
void freeInterfaces(list **interfaces) {
	list *root = *interfaces;
	list *tmp = NULL;
	while(root != NULL) {
		tmp = root->next;
		freeInterface((struct interface *) (root->content));
		free(root);
		root = tmp;
	}
}

/**
 * a helper method to run a function on each item in an interface list
 * - parameter interfaces: list of interfaces
 * - paramter f: function to run on each interface
 * - returns: zero if success, otherwise error
 */
int loopInterfaces(list *interfaces, int (*f)(struct interface *)) {
	list *root = interfaces;
	int ans = 0;
	while(root != NULL) {
		ans |= f((struct interface *)(root->content));
		root = root->next;
	}
	return ans;
}

/**
 * prints the interface in a nice format
 * - paramter i: interface to print
 * - returns: zero for success
 */
int print(struct interface *i) {
	printf("interface <%p>: {", i);
	printf(" %s, ", i->name);
	printf(" ibytes: %lu,", i->ibytes);
	printf(" obytes: %lu", i->obytes);
	printf(" }\n");
	return 0;
}

/**
 * set a flag for a specific interface using ioctls
 * http://stackoverflow.com/questions/5858655/linux-programmatically-up-down-an-interface-kernel
 * Setting the active flag word is a privileged operation, but
 * any process may read it. user ID of 0 or the CAP_NET_ADMIN capability
 * - parameter ifname: name of the interface
 * - paramter flag: flag
 * - returns: 0 on success, otherwise -1
 */
int set_if_flags(char *ifname, short flags) {
	if(!ifname) return ERR_NULL;
	struct ifreq ifr;
	int res = 0;
	ifr.ifr_flags = flags;       
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	int skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd < 0) {
		printERR("Unable to create socket.");
		return -1;
	}
	// Get or set the active flag word of the device.
	res = ioctl(skfd, SIOCSIFFLAGS, &ifr);
	
	if (res < 0) {
		printERR("Interface '%s' SIOCSIFFLAGS failed.",ifname);
	} else {
		printVERBOSE("Interface '%s': flags set to %04X.\n", ifname, flags);
	}

	 close(skfd);
	 return res;
 }

int set_if_up(char *ifname, short flags)
{
    return set_if_flags(ifname, flags | IFF_UP);
}
int set_up(struct interface *i) {
	return set_if_up(i->name, 0);
}

int set_if_down(char *ifname, short flags)
{
    return set_if_flags(ifname, flags & ~IFF_UP);
}
int set_down(struct interface *i) {
	return set_if_down(i->name, 0);
}
