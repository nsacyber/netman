#include "general.h"
#include "netinterfaces.h"

/**
 * - parameter argc: the number of arguments
 * - parameter argv: the argument array
 * - returns: 0 on success
 */
int main(int argc, char *argv[]) {
    // set the command line options
    static struct option long_options[] = {
      {"verbose",   no_argument, &verbose_flag, 1},
      {"quite",     no_argument, &verbose_flag, 0},
      {"silent",    no_argument, &verbose_flag, 0},
      {"label",     no_argument, &label_flag, 1},
      {"help",      no_argument, NULL, 'h'},
      {"version",   no_argument, NULL, 'v'},
      {"totalbytes",no_argument, NULL, 't'},
      {"ibytes",    no_argument, NULL, 'i'},
      {"obytes",    no_argument, NULL, 'o'},
      {"interface", required_argument, NULL, 'I'},
      {"command",   required_argument, NULL, 'c'},
      {"limit",     required_argument, NULL, 'l'},
      {"all",       no_argument, NULL, 'a'},
      {"run",       no_argument, NULL, 'r'},
      {"human",     no_argument, NULL, 'H'},
      {NULL, 0, NULL, 0}
    };

    char *interface_to_use = NULL;  // string to hold the interface name if specified
    char *command = NULL;           // string to hold the command if specified
    int ch = -1;                    // character represented as an integer for the options
    int totalFlag = 0;              // flag to be set if the user wants --totalbytes
    int inFlag = 0;                 // flag to be set if the user wants --ibytes
    int outFlag = 0;                // flag to be set if the user wants --obytes
    int limit=0;                    // flag to be set if the user wants --limit
    int humanFlag = 0;              // flag to be set if the user wants -H
    int option_index = 0;           // an index for options
    int num_options = 0;            // stores the number of correct options used
    int runtilComplete = 0;
    COMMAND cmd = BYTES;            // enum for the command to use (deafult BYTES)
    bytesRead = 0;

    while ((ch = getopt_long(argc, argv, "irHol:vthi:c:", long_options, &option_index)) != -1) {
        num_options++;
        switch (ch) {
            case 'I':
                interface_to_use = optarg;
                break;
            case 'r' :
                runtilComplete = 1;
                break;
            case 'v':
                version();
                return 0;
                break;
            case 'c':
                command = optarg;
                break;
            case 't':
                totalFlag = 1;
                break;
            case 'i':
                inFlag = 1;
                break;
            case 'H':
                humanFlag = 1;
                break;
            case 'l':
                limit = atoi(optarg);
            case 'o':
                outFlag = 1;
                break;
            case 0:
                if (long_options[option_index].flag != NULL)
                break;
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;
            case 'a':
                break;
            default:
                usage();
                return 0;
        }
    }

    #ifdef TEST
        printDEBUG("Going to run tests");
        return run_tests();
    #endif

    // if human flag set then convert bytes to MB
    if(humanFlag == 1) {
        limit = limit * 1000000;
    }

    // figure out what command to use and if the user wants to use a single interface
    if (num_options < argc)
    {
      printDEBUG("non-option ARGV-elements: %d %d", num_options, argc);
      int count = 1;
      while (count < argc) {
        printDEBUG ("%d %s", count, argv[count]);
        if(strncmp(argv[count], "up", 2) == 0) cmd = UP;
        else if(strncmp(argv[count], "down", 4) == 0) cmd = DOWN;
        else if(strncmp(argv[count], "bytes", 5) == 0) cmd = BYTES;
        else if(strncmp(argv[count], "monitor", 7) == 0) cmd = MONITOR;
        else if((char) *(argv[count]) != '-' && !interface_to_use) {
            if (strncmp(argv[count-1], "-l", 2) != 0 &&  strncmp(argv[count-1], "--limit", 5) != 0 &&
                strncmp(argv[count-1], "-c", 2) != 0 &&  strncmp(argv[count-1], "--command", 5) != 0) {

                interface_to_use = argv[count];
            }
        } else if((char) *(argv[count]) != '-') {
            if (strncmp(argv[count-1], "-l", 2) != 0 &&  strncmp(argv[count-1], "--limit", 5) != 0 &&
                strncmp(argv[count-1], "-c", 2) != 0 &&  strncmp(argv[count-1], "--command", 5) != 0) {

                printERR("Unknown command \'%s\'.", argv[count])
                usage();
                return 0;
            }
        }
        count++;
      }
    }

    list *interfaceList = NULL;

    printVERBOSE("argc %d num_options %d\n", argc, num_options);

    // Create the interface list
    // put the one interface in a list if specified, otherwise get them all
    if(interface_to_use) {
        printVERBOSE("using selected interface %s", interface_to_use);
        interfaceList = calloc(sizeof(struct list), 1);
        interfaceList->content = calloc(sizeof(struct interface), 1);
        ((struct interface *)interfaceList->content)->name = malloc(strnlen(interface_to_use, MAXCOMLEN));
        strncpy(((struct interface *)interfaceList->content)->name, interface_to_use, MAXCOMLEN);

        // get if_addr and byte info
        list *tmpInterfaceList = NULL;
        interfaces(&tmpInterfaceList);
        list *root = tmpInterfaceList;
        while(root != NULL) {
            if(strncmp(((struct interface *)root->content)->name, interface_to_use, MAXCOMLEN) == 0) {
                ((struct interface *)interfaceList->content)->if_addr = malloc(sizeof(struct sockaddr));
                memcpy(((struct interface *)interfaceList->content)->if_addr, ((struct interface *)root->content)->if_addr, sizeof(struct sockaddr)); 
                ((struct interface *)interfaceList->content)->obytes = ((struct interface *)root->content)->obytes;
                ((struct interface *)interfaceList->content)->ibytes = ((struct interface *)root->content)->ibytes;        
            }
            root = root->next;
        }
        if(tmpInterfaceList) freeInterfaces(&tmpInterfaceList);

    } else {
        printVERBOSE("using all interfaces\n");
        interfaces(&interfaceList);
    }

    if(verbose_flag) {
        printf("=== Selected interfaces ===\n");
        loopInterfaces(interfaceList, print);
        printf("=== end ===\n");
        if(command) printf("Using command: %s\n", command);
        if(cmd == MONITOR) {
            if(limit == 0) {
                printf("Limit is unlimited.\n");
            } else {
                printf("Limit is %d\n", limit);
            }
        }
    }

    int ret_status = 0;
    switch(cmd) {
        case UP:
            ret_status = loopInterfaces(interfaceList, set_up);
            if(ret_status != 0) {
                printERR("Failed to turn on interface, make sure you are sudo.\n");
            }
            break;
        case DOWN:
            ret_status = loopInterfaces(interfaceList, set_down);
            if(ret_status != 0) {
                printERR("Failed to shutdown interface, make sure you are sudo.\n");
            }
            break;
        case MONITOR: {
            // Create a new thread for each interface so they 
            // can each have a bfp
            // Then create an additional fork for the command if present
            // Exit the command fork if the limit is reached,
            // if no command is present, then just exit the application
            list *root = interfaceList;
            int threadCounter = 0;
            while(root != NULL) {
                pthread_t thread;
                char * name = (char *) ((struct interface *)root->content)->name;
                printDEBUG("creating pthread for %s\n", name);
                ret_status |= pthread_create(&thread, NULL, monitor, (void *) name);
                pthread_mutex_lock(&thread_mutex);
                threads[threadCounter++] = thread;
                pthread_mutex_unlock(&thread_mutex);
                root = root->next;
            }

            if(ret_status != 0) {
                printERR("Failed to create a thread.");
                if(geteuid() != 0) {
                    printERR("Try again with sudo");
                }
                break;
            }

            // wait for threads for about five seocnds
            // this gives the filters time to get setup
            for(int i = 0; i < 5; i++) {
                sleep(1);
            }

            printDEBUG("thread count: %d\n", threadCount());
            if(threadCount() <= 0) {
                printERR("No threads to monitor.");
                if(geteuid() != 0) {
                    printERR("Try again with sudo");
                }
                if(ret_status == 0) {
                    ret_status = -1;
                }
                break;
            }

            pid_t pid = runCmd(command);
            // if command failed, then stop
            if(pid < 0) {
                printERR("Failed to start command");
                break;
            }

            // run the command and output the total bytes
            if(pid > 0 && runtilComplete) {
                printVERBOSE("Running command to completion...");
                int status = 0; 
                waitpid(pid, &status, 0);
                printVERBOSE("cmd status: %d", status);
                if(verbose_flag || label_flag) {
                    printf("Total RX+TX: ");
                }
                u_int64_t rbytes = bytesRead;
                if(humanFlag == 1) {
                    rbytes = rbytes / 1000000.0;
                }
                printf("%llu", rbytes);
                if(verbose_flag || label_flag) {
                    if(humanFlag == 1) {
                        printf(" Mb");
                    } else {
                        printf(" bytes");
                    }
                }
                printf("\n");
                break;
            }

            // run the command and kill it if it reaches the byte limit
            while(limit >= 0) {
                // check if byte limit reached, 0 is unlimited
                if((int) bytesRead >= limit && limit > 0) {
                        printVERBOSE("Byte limit reached");
                        if(pid > 0) {
                            printVERBOSE("Killing command");
                            kill(pid, SIGKILL);
                        }
                        break;
                }

                 // check if command is done
                if(pid > 0) {
                    if(getpgid(pid) <= 0) {
                        printVERBOSE("Command finished before limit was reached");
                        break;
                    }
                }
            }
            break;
        }
        default: {
            // print the byte information
            list *root = interfaceList;
            float in = 0, out = 0;
            while(root != NULL) {
                in += ((struct interface *)root->content)->ibytes;
                out += ((struct interface *)root->content)->obytes;
                root = root->next;
            }

            if(humanFlag == 1) {
                in = in / 1000000.0;
                out = out / 1000000.0;
            }

            if(inFlag == 1) {
                if(verbose_flag || label_flag) printf("RX: ");
                printf("%0.2f", in);
            } else if(outFlag == 1) {
                if(verbose_flag || label_flag) printf("TX: ");
                printf("%0.2f", out);
            } else {
                if(verbose_flag || label_flag) printf("RX+TX: ");
                printf("%0.2f", in+out);
            }
            if(verbose_flag || label_flag) {
                if(humanFlag == 1) {
                    printf(" Mb");
                } else {
                    printf(" bytes");
                }
            }
            printf("\n");
            break;
        }
    }

    if(interfaceList) freeInterfaces(&interfaceList);

    return ret_status;
}