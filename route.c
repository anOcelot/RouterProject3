#include <sys/socket.h> 
#include <netpacket/packet.h> 
#include <net/ethernet.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/if_ether.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
struct interface{
	
	char *ifa_name;
	struct sockaddr *ifa_addr;
};

void buildResponse(struct interface *inter, struct ether_header *ether, struct ether_arp *arp);

struct sockaddr eth0; 

int main(){
  int packet_socket;
  struct sockaddr_ll *mymac;

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

	mymac = (struct sockaddr_ll*)tmp->ifa_addr;
	printf("Our Mac: %02x:%02x:%02x:%02x:%02x;%02x\n", mymac->sll_addr[0], mymac->sll_addr[1], mymac->sll_addr[2], mymac->sll_addr[3], mymac->sll_addr[4], mymac->sll_addr[5]);

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
    
    printf("+++++++++++++++Recieving Info+++++++++++++++\n");    
    
    printf("Sender Mac: %02x:%02x:%02x:%02x:%02x:%02x\n", etherH->ether_shost[0], etherH->ether_shost[1],
    etherH->ether_shost[2], etherH->ether_shost[3], etherH->ether_shost[4], etherH->ether_shost[5]);
    //printf("%d\n", arpH->arp_op);
    printf("type: %x\n", etherH->ether_type);
     if(ntohs(etherH->ether_type)==ETHERTYPE_ARP){
	struct ether_arp *arpH = (struct ether_arp*)(buf+14);
	printf("hardware: %x\n", ntohs(arpH->arp_hrd));
	printf("protocol: %x\n", ntohs(arpH->arp_pro));
     	printf("hlen: %x\n", arpH->arp_hln);
    	printf("plen: %x\n", arpH->arp_pln);
    	printf("arp op: %x\n", ntohs(arpH->arp_op));
    	printf("sender mac: %02x:%02x:%02x:%02x:%02x:%02x\n", arpH->arp_sha[0], arpH->arp_sha[1],
    		arpH->arp_sha[2], arpH->arp_sha[3], arpH->arp_sha[4], arpH->arp_sha[5]);
	printf("sender IP: %02d:%02d:%02d:%02d\n", arpH->arp_spa[0], arpH->arp_spa[1],
    		arpH->arp_spa[2], arpH->arp_spa[3]);
	printf("Target IP: %02d:%02d:%02d:%02d\n", arpH->arp_tpa[0], arpH->arp_tpa[1],
    		arpH->arp_tpa[2], arpH->arp_tpa[3]);
	
	
    	printf("sender protoc: %d\n", arpH->arp_spa[0]); 
	//arpResp->arp_tha[0] = arpH->arp_sha;
	//arpResp->arp_tpa = arpH->arp_spa;
	//arpResp->arp_spa = arpH->arp_tpa;
	//arpResp->arp_sha = //my mac
	
	char replyBuffer[42];
	struct ether_header *outEther = (struct ether_header *)(replyBuffer);
	struct ether_arp *arpResp = (struct ether_arp *)(replyBuffer+14);
	memcpy(outEther->ether_dhost, etherH->ether_shost,6);
	memcpy(outEther->ether_shost, mymac->sll_addr,6);
	outEther->ether_type = 1544;
	printf("-------------------------------Sending Info-----------------------\n");
	printf("ETHER HEADER:_________________________\n");
	printf("My Mac: %02x:%02x:%02x:%02x:%02x:%02x\n", outEther->ether_shost[0], outEther->ether_shost[1],
    	outEther->ether_shost[2], outEther->ether_shost[3], outEther->ether_shost[4], outEther->ether_shost[5]);
    
	printf("Dest Mac: %02x:%02x:%02x:%02x:%02x:%02x\n", outEther->ether_dhost[0], outEther->ether_dhost[1],
    	outEther->ether_dhost[2], outEther->ether_dhost[3], outEther->ether_dhost[4], outEther->ether_dhost[5]);

	printf("Protocol: %x\n",outEther->ether_type);
 	
	arpResp->ea_hdr.ar_hrd = 0x100;
	arpResp->ea_hdr.ar_pro = 0x8;
	arpResp->ea_hdr.ar_hln = 0x6;
	arpResp->ea_hdr.ar_pln = 0x4;
	arpResp->ea_hdr.ar_op = htons(0x2);
	memcpy(arpResp->arp_tha,arpH->arp_sha,6);
	memcpy(arpResp->arp_tpa,arpH->arp_spa,4);
	memcpy(arpResp->arp_sha,outEther->ether_shost,6);
	memcpy(arpResp->arp_spa,arpH->arp_tpa,4);

	printf("ARPRESP HEADER:__________________\n");
	
	printf("Hardware: %x\n", ntohs(arpResp->arp_hrd));
	printf("Protocol: %x\n", ntohs(arpResp->arp_pro));
     	printf("Hlen: %x\n", arpResp->arp_hln);
    	printf("Plen: %x\n", arpResp->arp_pln);
    	printf("Arp Op: %x\n", ntohs(arpResp->arp_op));
    	printf("Sender Mac: %02x:%02x:%02x:%02x:%02x:%02x\n", arpResp->arp_sha[0], arpResp->arp_sha[1],
    		arpResp->arp_sha[2], arpResp->arp_sha[3], arpResp->arp_sha[4], arpResp->arp_sha[5]);
    	printf("Sender Mac: %02x:%02x:%02x:%02x:%02x:%02x\n", arpResp->arp_tha[0], arpResp->arp_tha[1],
    		arpResp->arp_tha[2], arpResp->arp_tha[3], arpResp->arp_tha[4], arpResp->arp_tha[5]);
	printf("Sender IP: %02d:%02d:%02d:%02d\n", arpResp->arp_spa[0], arpResp->arp_spa[1],
    		arpResp->arp_spa[2], arpResp->arp_spa[3]);
	printf("Target IP: %02d:%02d:%02d:%02d\n", arpResp->arp_tpa[0], arpResp->arp_tpa[1],
    		arpResp->arp_tpa[2], arpResp->arp_tpa[3]);

	int sent = send(packet_socket,&replyBuffer,42,0);	

	
    if(sent<0) {perror("SEND");}


	    

    }

     if(ntohs(etherH->ether_type)==ETHERTYPE_IP){
	printf("Got IPV4 packet!\n");   
	struct ip *ipH = (struct ip *)(buf+14);
	
	printf("IP HEADER: --------------------------------- \n");
	//printf("Target IP: %02d:%02d:%02d:%02d\n", ipH->ip_src[0],ipH->ip_src[1],ipH->ip_src[2],ipH->ip_src[3]);
	//printf("Target IP: %02d:%02d:%02d:%02d\n", ipH->ip_dst[0],ipH->ip_dst[1],ipH->ip_dst[2],ipH->ip_dst[3]);
//	char *sip = inet_ntoa(ipH->ip_src);
//	char *dip = inet_ntoa(ipH->ip_dst);
//	printf("%s\n",sip);
//	printf("%s\n",dip); 
//	printf("%d\n",(unsigned int)ipH->ip_p);
	if((unsigned int)ipH->ip_p==1){
		printf("Got ICMP Packet\n");
		//getting and building ICMP
		char replyBuffer[98];
		struct ip *ipreply = (struct ip *)(replyBuffer + 14);
		ipreply->ip_v=0x4;
		ipreply->ip_hl=0x5;
		ipreply->ip_tos=0x00;
		ipreply->ip_len=0x5400;
		ipreply->ip_id=0x0000;
                ipreply->ip_off=0x40;
		ipreply->ip_ttl=0x40;
		memcpy(ipreply->ip_dst.s_addr, ipH->ip_src.s_addr, 4);
//		struct ether_header *outEther = (struct ether_header *)(replyBuffer);
//		struct ip *ipHR = (struct ip *)(replyBuffer+14);
//		memcpy(outEther->ether_dhost, etherH->ether_shost,6);
//		memcpy(outEther->ether_shost, mymac->sll_addr,6);
//		outEther->ether_type = 0x800;
//		//IP building
//		ipHR->ip_hl = 5;
//		ipHR->ip_v = 4;
//		memcpy(ipHR->ip_tos,ipH->ip_tos,1);
//		ipHR->ip_len = sizeof(struct ip) + sizeof(struct icmphdr);
//		ipHR->ip_id = 56;
//		ipHR->ip_off=0;
//		ipHR->ip_ttl = 64;
//		//ipHR->ip_sum;
//		memcpy(ipHR->ip_src,ipH->ip_dst,4);
//		memcpy(ipHR->ip_src,ipH->ip_dst,4);
//		memcpy(ipHR->ip_src,ipH->ip_dst,4);
//		memcpy(ipHR->ip_src,ipH->ip_dst,4);
	}
	
	} 
  return 0;
}


}
