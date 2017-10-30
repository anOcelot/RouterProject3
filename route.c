#include <sys/socket.h> 
#include <netpacket/packet.h> 
#include <net/ethernet.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/if_ether.h>
#include <string.h>
struct interface{
	
	char *ifa_name;
	struct sockaddr *ifa_addr;
};

void buildResponse(struct interface *inter, struct ether_header *ether, struct ether_arp *arp);

struct sockaddr eth0; 

int main(){
  int packet_socket;
  //get list of interfaces (actually addresses)
  struct ifaddrs *ifaddr, *tmp;
  if(getifaddrs(&ifaddr)==-1){
    perror("getifaddrs");
    return 1;
  }
  //have the list, loop over the list
  for(tmp = ifaddr; tmp!=NULL; tmp=tmp->ifa_next){
    //Check if this is a packet address, there will be one per
    //interface.  There are IPv4 and IPv6 as well, but we don't care
    //about those for the purpose of enumerating interfaces. We can
    //use the AF_INET addresses in this list for example to get a list
    //of our own IP addresses
    if(tmp->ifa_addr->sa_family==AF_PACKET){
      printf("Interface: %s ",tmp->ifa_name);
      printf("Family: %u\n", tmp->ifa_addr->sa_family);
      //create a packet socket on interface r?-eth1
      if(!strncmp(&(tmp->ifa_name[3]),"eth0",4)){
	printf("Creating Socket on interface %s\n",tmp->ifa_name);
	
	//create a packet socket
	//AF_PACKET makes it a packet socket
	//SOCK_RAW makes it so we get the entire packet
	//could also use SOCK_DGRAM to cut off link layer header
	//ETH_P_ALL indicates we want all (upper layer) protocols
	//we could specify just a specific one
	packet_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if(packet_socket<0){
	  perror("socket");
	  return 2;
	}
	//Bind the socket to the address, so we only get packets
	//recieved on this specific interface. For packet sockets, the
	//address structure is a struct sockaddr_ll (see the man page
	//for "packet"), but of course bind takes a struct sockaddr.
	//Here, we can use the sockaddr we got from getifaddrs (which
	//we could convert to sockaddr_ll if we needed to)
	if(bind(packet_socket,tmp->ifa_addr,sizeof(struct sockaddr_ll))==-1){
	  perror("bind");
	}
	memcpy(&eth0, tmp->ifa_addr, sizeof(struct sockaddr));
	//eth0 = (struct interface*) malloc(sizeof(struct interface));
	//*eth0->ifa_addr = tmp->ifa_addr;
	//eth0->ifa_name = tmp->ifa_name;



      }
    }
  }
  //free the interface list when we don't need it anymore
   freeifaddrs(ifaddr);

  //loop and recieve packets. We are only looking at one interface,
  //for the project you will probably want to look at more (to do so,
  //a good way is to have one socket per interface and use select to
  //see which ones have data)
  printf("Ready to recieve now\n");
  while(1){
    char buf[1500];
    struct sockaddr_ll recvaddr;
    int recvaddrlen=sizeof(struct sockaddr_ll);
    //we can use recv, since the addresses are in the packet, but we
    //use recvfrom because it gives us an easy way to determine if
    //this packet is incoming or outgoing (when using ETH_P_ALL, we
    //see packets in both directions. Only outgoing can be seen when
    //using a packet socket with some specific protocol)
    int n = recvfrom(packet_socket, buf, 1500,0,(struct sockaddr*)&recvaddr, &recvaddrlen);
    //ignore outgoing packets (we can't disable some from being sent
    //by the OS automatically, for example ICMP port unreachable
    //messages, so we will just ignore them here)
    if(recvaddr.sll_pkttype==PACKET_OUTGOING)
      continue;
    //start processing all others
    printf("Got a %d byte packet\n", n);
    //what else to do is up to you, you can send packets with send,
    //just like we used for TCP sockets (or you can use sendto, but it
    //is not necessary, since the headers, including all addresses,
    //need to be in the buffer you are sending)


    struct ether_header *etherH = (struct ether_header*)(buf);
    struct ether_arp *arpH = (struct ether_arp*)(buf+14);
    
    
    printf("Mac: %x:%x:%x:%x:%x:%x\n", etherH->ether_shost[0], etherH->ether_shost[1],
    etherH->ether_shost[2], etherH->ether_shost[3], etherH->ether_shost[4], etherH->ether_shost[5]);
	printf("%d\n", arpH->arp_op);
    printf("type: %x\n", etherH->ether_type);
    printf("hardware: %x\n", ntohs(arpH->arp_hrd));
    printf("protocol: %x\n", ntohs(arpH->arp_pro));
    printf("hlen: %x\n", arpH->arp_hln);
    printf("plen: %x\n", arpH->arp_pln);
    printf("arp op: %x\n", ntohs(arpH->arp_op));
    printf("sender mac: %x:%x:%x:%x:%x:%x\n", arpH->arp_sha[0], arpH->arp_sha[1],
    	arpH->arp_sha[2], arpH->arp_sha[3], arpH->arp_sha[4], arpH->arp_sha[5]);
    printf("%d\n", arpH->arp_op);
    printf("sender protoc: %d\n", arpH->arp_spa[0]); 
       
    
    char replyBuffer[42];
    struct ether_header *outEther = (struct ether_header*)(replyBuffer);       
    struct ether_arp *arpResp = (struct ether_arp*)(replyBuffer+14);
    
    memcpy(outEther->ether_dhost, etherH->ether_shost, 6); 
    memcpy(outEther->ether_shost, etherH->ether_dhost, 6);
    outEther->ether_type=0x0608;
    arpResp->ea_hdr.ar_hrd = 0x100;
    arpResp->ea_hdr.ar_pro = 0x8;
    arpResp->ea_hdr.ar_hln = 0x6;
    arpResp->ea_hdr.ar_pln = 0x4;
    arpResp->ea_hdr.ar_op = 0x2;
   

   
// memcpy(outEther->ether_type, etherH->ether_type, 2);
    printf("\n");
    send(packet_socket, outEther, 42, 0);
   
    int i = send(packet_socket, replyBuffer, 14, 0);
    printf("%d/n", i);
     //arpResp->arp_sha =
    //arpResp->arp_spa =
    //arpResp->arp_tha =
    //arpResp->arp_tpa =
    fprintf(stderr, "value of error: %d\n", errno);	
    //memcpy(arpResp->arp_sha,  
   // memcpy(arpResp->arp_tha, arpH->arp_sha, sizeof(arpH->arp_sha));
    //memcpy(arpResp->arp_sha, 
	    

 //struct ether_arp if_arp
//struct iphdr *iph = (struct iphdr*)(buf);
    //struct ether_arp *header = (struct ether_arp*)(buf);
    //printf("%u\n", ntohs(eth->ether_type));   
    }
  //exit
  return 0;
}
void buildResponse(struct interface *inter, struct ether_header *ether, struct ether_arp *arp){

		printf("%s\n", inter->ifa_name);
		printf("%d\n", inter->ifa_addr->sa_family);

}




