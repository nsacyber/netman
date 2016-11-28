# netman 
## '''Net'''work '''Man'''agement, Monitoring, and Limiting

### Installation and Usage

1. `make`
2. `make install`
3. `netman --help`

Note: some of `netman`'s functionality requires elevated privileges

### Use Cases
#### Command Data Limiting

     sudo netman --command="wget https://example.com/script | sh" --limit=25 -H monitor

The above command will limit the `wget https://example.com/script | sh` command to 25MB system wide. After that, the command will be terminated. 

#### Command Chaining
'''Example One'''
	 
	 sudo netman --command="wget https://example.com/script | sh" --limit=25 -H monitor && sudo netman down

The above command is similiar to the ''Command Data Limiting'' example, but afterwards it will shutdown all network interfaces.

'''Example Two'''

	 netman down && netman up en0 && sudo netman --command="wget https://example.com/script | sh" --limit=25 -H monitor && sudo netman down

The above command is similiar to example one except the command will only use the `en0` network interface.

'''Example Three'''

	 netman --limit=100 -H && kill -9 6543

The above command is similiar to using the `--command` flag except after 100MB the process with the id 6543 is terminated.


### Technical Details

#### Network Interfaces

Interfaces are retrieved from `getifaddrs (3)`. Interfaces are stored in a custom `interface` `struct`. 

     struct interface {
     	char *name;
     	struct sockaddr *if_addr;
     	u_long obytes;
     	u_long ibytes;
     };
     typedef struct interface interface;

The alternative method is to use `ioctl` with the `SIOCGIFCONF` flag. 

#### Testing

Using a modified version of [MinUnit](http://www.jera.com/techinfo/jtns/jtn002.html) -- a minimal unit testing framework for C.

For debug mode, `make` with `DEBUG=1`. To run tests, `make` with `TEST=1`

#### [BFP - Berkley Packet Filter](https://developer.apple.com/legacy/library/documentation/Darwin/Reference/ManPages/man4/bpf.4.html)

The logging of used bytes is done using Berkley Packet Filters (bfp) with no filters applied.

Basic examples of bfps: 
* [Michael Santos](https://gist.github.com/msantos/939154); Toronto, Canada
* [Manuel Vonthron](https://gist.github.com/manuelvonthron-opalrt/8559997); Montréal, QC, Canada

#### Limitations

macOS does not have eBPFs yet so `netman` cannot monitor specific sockets for specific applications, only interfaces. What does this mean? Well if multiple applications are the network then your byte limit may be reached much faster. [Socket filters](https://developer.apple.com/library/content/documentation/Darwin/Conceptual/NKEConceptual/socket_nke/socket_nke.html#//apple_ref/doc/uid/TP40001858-CH228-SW1) would be a logical next step. 