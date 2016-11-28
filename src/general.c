#include "general.h"

static char *VERSION = "1.0";

/**
 * prints the version number
 */
void version() {
    println("netman %s", VERSION);
}

/**
 * prints the help page
 */
void usage() {
    println("usage: netman [interface] [options...] [command]");

    println("\nCommands:");
    println("  bytes                 Print the byte information for the specified interface(s)\n"
            "                        and exit. (default)");
    println("  up                    Turn the selected interface(s) up and exit. (privileged)");
    println("  down                  Turn the selected interface(s) down and exit. (privileged)");
    println("  monitor               Monitor the selected interface(s). (privileged)");

    println("\nOptions:");
    println("  -v, --version         Print the version number of netman and exit.");
    println("  --quite, --silent     Don't echo commands.");
    println("  --verbose             Echo additional messages.");
    println("  --help                Print this message and exit.");
    println("  --label               Print byte labels.")
    println("  -H                    Use (decimal) megabytes intead of bytes.");

    println("\nbyte Options:");
    println("  -t, --totalbytes      Print the (RX + TX) bytes. (default)");
    println("  -i, --ibytes          Print the RX bytes.");
    println("  -o, --obytes          Print the TX bytes.");

    println("\nmonitor Options:");
    println("  -l, --limit           The byte limit. In MB if -H is set, otherwise B.");
    println("  -c, --command         Command to run.");
    println("  --run                 Run the specified command until completion then print")
    println("                        the total RX + TX bytes. This ignores any limit set.")
}

/** 
 * removes the thread from the bpf thread list
 */
void removeThread() {
    pthread_t thread = pthread_self();

    pthread_mutex_lock(&thread_mutex);

    for(u_int32_t i = 0; i < sizeof(threads); i++) {
        if(threads[i] == thread) {
            threads[i] = 0;
            break;
        }
    }

    pthread_mutex_unlock(&thread_mutex);
}

/**
 * - returns: the number of bpf threads 
 */
int threadCount() {
    int count = 0;
    pthread_mutex_lock(&thread_mutex);
    for(int i = 0; i < 20; i++) {
        if(threads[i] != NULL) {
            count++;
        }
    }
    pthread_mutex_unlock(&thread_mutex);
    return count;
}

/**
 *  Runs a specified command in sh in another process
 *  If the user is root, bumps the user down to their original uid
 *  - returns: pid of the fork or a negative number if an error has occured, 0 if command is null
 */
pid_t runCmd(char *cmd) {
    if(cmd == NULL) return 0;

    pid_t pid = fork();
    if(pid == 0) {
        if(geteuid() == 0) {
            char *env = getenv("SUDO_UID");
            if(env == NULL){ 
                printERR("Failed to get SUDO_UID environmental variable.");
                kill(getpid(), SIGKILL);
                return ERR_SUDO; 
            }

            int uid = strtol(env, NULL, 10);
            if(uid <= 0) { 
                printERR("Failed to convert SUDO_UID environmental variable to a number.");
                kill(getpid(), SIGKILL);
                return ERR_SUDO; 
            }

            if(setreuid(uid, uid) != 0) {
                printERR("Failed to set uid.");
                kill(getpid(), SIGKILL);
                return ERR_UID;
            }

        }
        printVERBOSE("Starting command '%s'", cmd);
        printVERBOSE("uid %d; euid %d; cmd '%s'", getuid(), geteuid(), cmd);
        char *const parmList[] = {"sh", "-c", cmd, NULL};
        execv("/bin/sh", parmList);
    } else if(pid < 0) {
        printERR("Failed to fork.");
        return ERR_FORK;
    }
    return pid;
}

/**
 * Monitor an interface name by opening a bpf device and read the 
 * incoming packets
 * - parameter ifname: interface name to monitor
 * - returns: void pointer to an integer if error
 */
void* monitor(void *ifname) {
    if(ifname == NULL) {
        return (void *) ERR_NULL;
    }
    char *name = (char*)ifname;

    printVERBOSE("[%s] Going to open device.", name);
    int fd = 0;
    fd = open_dev();
    if (fd < 0) {
        printVERBOSE("unable to open dev for %s: %s", name, strerror(errno));
        removeThread();
        return (void *) ERR_OPEN; 
    }

    printVERBOSE("[%s] Going to set options for device.", name);
    if (set_options(fd, name) < 0) {
        printVERBOSE("unable to do set options for %s: %s\n", name, strerror(errno));
        removeThread();
        return ( void *) ERR_OPTIONS;
    }

    printVERBOSE("[%s] Checking dlt.", name);
    if (check_dlt(fd, name) < 0) {
        removeThread();
        return (void *) ERR_DLT;
    }

    printVERBOSE("[%s] Reading packets start.", name);
    read_packets(fd, name);

    printVERBOSE("done reading packets\n");
    removeThread();
    return (void *) ERR_READ;
}

/**
 * Finds the first O_RDWR file bpf device from `start`
 * - parameter start: the index to start iterating the bpf devices
 * - returns: file descriptor if success, otherwise -1
 */
int open_dev_at(int start) {
    int fd = -1;
    char dev[32];
    int i = 0;

    // find the first avaiable device
    for (i = start; i < 255; i++) {
        snprintf(dev, sizeof(dev), "/dev/bpf%u", i);

        fd = open(dev, O_RDWR);
        if (fd > -1)
            return fd;

        switch (errno) {
            case EBUSY:
                break;
            default:
                return -1;
        }
    }

    errno = ENOENT;
    return -1;
}

/**
 * Helper functions for `open_dev_at` starting at device zero
 * - returns: file descriptor if success, otherwise -1
 */
int open_dev(void) {
  return open_dev_at(0);
}

/**
 * Checks the datalink type for a given descriptor
 * Note: Datalink types are defined in `<net/bpf.h>`
 * - parameter fd: file descriptor for the bpf
 * - parameter name: network interface name
 * - returns: zero if datalink type is "Ethernet (10Mb)", ERR_DLT otherwise
 */
int check_dlt(int fd, char *name) {
    u_int32_t dlt = 0;

    /*
     * Returns the type of the data link layer underlying the attached interface.
     * EINVAL is returned if no interface has been specified.  The device types, prefixed with
     * ``DLT_'', are defined in <net/bpf.h>.
     */
    if(ioctl(fd, BIOCGDLT, &dlt) < 0)
        return ERR_DLT;

    switch (dlt) {
        case DLT_EN10MB: /* Ethernet (10Mb) */
            return 0;
        case DLT_NULL: /* no link-layer encapsulation */
            printVERBOSE("Unsupported NULL datalink type for %s.", name);
            break;
        case DLT_EN3MB: /* Experimental Ethernet (3Mb) */
            printVERBOSE("Unsupported EN3MB datalink type for %s.\n", name);
            break;
        case DLT_AX25: /* Amateur Radio AX.25 */
            printVERBOSE("Unsupported AX25 datalink type for %s.\n", name);
            break;
        case DLT_PRONET: /* Proteon ProNET Token Ring */
            printVERBOSE("Unsupported PRONET datalink type for %s.\n", name);
            break;
        case DLT_CHAOS: /* Chaos */
            printVERBOSE("Unsupported CHAOS datalink type for %s.\n", name);
            break;
        case DLT_IEEE802: /* IEEE 802 Networks */
            printVERBOSE("Unsupported IEEE802 datalink type for %s.\n", name);
            break;
        case DLT_ARCNET: /* ARCNET */
            printVERBOSE("Unsupported ARCNET datalink type for %s.\n", name);
            break;
        case DLT_SLIP: /* Serial Line IP */
            printVERBOSE("Unsupported SLIP datalink type for %s.\n", name);
            break;
        case DLT_PPP: /* Point-to-point Protocol */
            printVERBOSE("Unsupported PPP datalink type for %s.\n", name);
            break;
        case DLT_FDDI:
            printVERBOSE("Unsupported FDDI datalink type for %s.\n", name);
            break;
        case DLT_ATM_RFC1483: /* LLC/SNAP encapsulated atm */
            printVERBOSE("Unsupported ATM_RFC1483 datalink type for %s.\n", name);
            break;
        case DLT_RAW: /* Raw IP */
            printVERBOSE("Unsupported RAW datalink type for %s.\n", name);
            break;
        default:
            printVERBOSE("Unsupported and unknown datalink type for %s!\n", name);
            break;
    }
    return ERR_DLT;
}

/**
 * set the interface name for the bpf
 * - parameter fd: file descriptor for the bpf
 * - parameter iface: network interface name 
 * - returns: 0 if success, othewise error
 */
int set_options(int fd, char *iface) {
    if(!iface) return ERR_NULL;

    struct ifreq ifr;
    u_int32_t enable = 1;

    /* 
     * Sets the hardware interface associated with the file.  This command must
     * be performed before any packets can be read.  The device is indicated by name using the
     * ifr_name field of the ifreq structure.  Additionally, performs the actions of BIOCFLUSH.
     */
    strlcpy(ifr.ifr_name, iface, sizeof(ifr.ifr_name)-1);
    if(ioctl(fd, BIOCSETIF, &ifr) < 0)
        return ERR_SETIF;

    /* 
     * Sets or gets the status of the ``header complete'' flag.  Set to zero if the
     * link level source address should be filled in automatically by the interface output rou-
     * tine.  Set to one if the link level source address will be written, as provided, to the
     * wire.  This flag is initialized to zero by default.
    */
    if(ioctl(fd, BIOCSHDRCMPLT, &enable) < 0)
        return ERR_HDRCMPLT;

    /*
     * Sets or gets the flag determining whether locally generated packets on the
     * interface should be returned by BPF.  Set to zero to see only incoming packets on the
     * interface.  Set to one to see packets originating locally and remotely on the interface.
     * This flag is initialized to one by default.
     */
    if(ioctl(fd, BIOCSSEESENT, &enable) < 0)
        return ERR_BIDIRECTION;

    /*
     * Enables or disables ``immediate mode'', based on the truth value of the argu-
     * ment.  When immediate mode is enabled, reads return immediately upon packet reception.
     * Otherwise, a read will block until either the kernel buffer becomes full or a timeout
     * occurs.  This is useful for programs like rarpd(8) which must respond to messages in
     * real time.  The default for a new file is off.
     */
    if(ioctl(fd, BIOCIMMEDIATE, &enable) < 0)
        return ERR_IMMEDIATE;

    return 0;
}

/**
 * reads the bpf device for the specified interface 
 * - parameter fd: file descriptor for the bpf
 * - parameter iface: network interface name
 */
int read_packets(int fd, char *iface) {
    if(!iface) return ERR_NULL;
    char *buf = NULL;
    char *p = NULL;
    size_t blen = 0;
    ssize_t n = 0;
    struct bpf_hdr *bh = NULL;
    struct ether_header *eh = NULL;

    // Returns the required buffer length for reads on bpf files.
    if(ioctl(fd, BIOCGBLEN, &blen) < 0)
        return ERR_BLEN;

    buf = malloc(blen);
    if(!buf) return ERR_ALLOC;

    printVERBOSE("Reading packets for \'%s\'...", iface);

    while(true) {
        memset(buf, '\0', blen);

        n = read(fd, buf, blen);

        if (n <= 0)
            return ERR_READ;

        p = buf;
        while (p < buf + n) {
            bh = (struct bpf_hdr *)p;
            bytesRead += bh->bh_caplen;

            eh = (struct ether_header *)(p + bh->bh_hdrlen);

            printVERBOSE("%s: %02x:%02x:%02x:%02x:%02x:%02x -> "
                    "%02x:%02x:%02x:%02x:%02x:%02x "
                    "[type=%u] [len=%u/%u (%llu)]", 
                    iface,
                    eh->ether_shost[0], eh->ether_shost[1], eh->ether_shost[2],
                    eh->ether_shost[3], eh->ether_shost[4], eh->ether_shost[5],

                    eh->ether_dhost[0], eh->ether_dhost[1], eh->ether_dhost[2],
                    eh->ether_dhost[3], eh->ether_dhost[4], eh->ether_dhost[5],

                    eh->ether_type, bh->bh_datalen, bh->bh_caplen, bytesRead);

            p += BPF_WORDALIGN(bh->bh_hdrlen + bh->bh_caplen);
        }
    }
}