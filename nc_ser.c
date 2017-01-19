/*
	NAT check Server process, Just receive and deal commands.

	wangxinyu.yy@gmail.com
	2016.9.14

	I will complete it today, and I want to do this long ago.

	Let me go and do.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <errno.h>
#include "common.h"
#include "log_debug.h"

static char local_ip[32] = {0};
static int local_port = SERVER_TEST_PORT;

#pragma pack(1)
struct udp_packet {
	struct iphdr ip;
	struct udphdr udp;
	char data[0];
};
#pragma pack()

u_int16_t checksum(void *addr, int count)
{
	/* Compute Internet Checksum for "count" bytes
	 *         beginning at location "addr".
	 */
	register int32_t sum = 0;
	u_int16_t *source = (u_int16_t *) addr;

	while (count > 1)  {
		/*  This is the inner loop */
		sum += *source++;
		count -= 2;
	}

	/*  Add left-over byte, if any */
	if (count > 0) {
		/* Make sure that the left-over byte is added correctly both
		 * with little and big endian hosts */
		u_int16_t tmp = 0;
		*(unsigned char *) (&tmp) = * (unsigned char *) source;
		sum += tmp;
	}
	/*  Fold 32-bit sum to 16 bits */
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return ~sum;
}



//在该函数中构造整个IP报文，最后调用sendto函数将报文发送出去
int send_nat_type_cmd_raw1(char *src_ip, int src_port, 
    									char *dst_ip, int dst_port, char *msg)
{
    char buf[1024]={0};
    struct ip *ip;
    struct udphdr *udp;
    char *data;
    int ip_len;
    int sock = -1;
    int ret = 0;
    struct sockaddr_in dest;

	TRACE;
    
	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(dst_port);
	dest.sin_addr.s_addr = inet_addr(dst_ip);

	sock = socket(AF_INET, SOCK_RAW,  IPPROTO_UDP );
    if (sock < 0) 
    {
        printf("socket call failed: %s", strerror(errno));
        return -1;
    }

	const int one = 1;
	if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, (char *)&one, sizeof(one))< 0)
	{
		perror("IP_HDRINCL");
        return -1;
	}
    
	ip_len = sizeof(struct ip)+sizeof(struct udphdr) + strlen(msg) + 1;
	//开始填充IP首部
	ip=(struct ip*)buf;


	ip->ip_p=IPPROTO_UDP;
	ip->ip_sum=0;
	ip->ip_dst.s_addr= inet_addr(dst_ip);
    ip->ip_src.s_addr = inet_addr(src_ip);
    ip->ip_len = htons(ip_len - sizeof(struct ip)); //psuedo header length

    udp = (struct udphdr*)(buf+sizeof(struct ip));
    udp->source = htons(src_port);
    udp->dest = htons(dst_port);
    udp->len = htons(sizeof(struct udphdr) + strlen(msg) + 1);
    udp->check = 0;

	data = buf+sizeof(struct ip) + sizeof(struct udphdr);
    memcpy(data, msg, strlen(msg) + 1);
    udp->check=checksum((unsigned char*)buf, ip_len);

	ip->ip_v = IPVERSION;
	ip->ip_hl = sizeof(struct ip)>>2;
	ip->ip_tos = 0;
	ip->ip_len = htons(ip_len);
	ip->ip_id=0;
	ip->ip_off=0;
	ip->ip_ttl=MAXTTL;
    
    ret = sendto(sock,buf,ip_len,0,(struct sockaddr*)&dest,sizeof(struct sockaddr_in));
    print_package(buf,ip_len);
    if (ret <= 0) 
    {
        printf("sendto failed: %s", strerror(errno));
    }
    close(sock);

	printf("%s [%d] ret=%d\n", __FUNCTION__, __LINE__, ret);
    
    return ret;
}


int send_nat_type_cmd_raw(char *src_ip, int src_port, 
    									char *dst_ip, int dst_port, char *msg)
{
	int ret = 0;
    int sock;
    struct sockaddr_in dest;
    struct udp_packet *packet;
    int packet_len = 0;
    char buf[1024] = {0};

    packet = (struct udp_packet *)buf;

	sock = socket(AF_INET, SOCK_RAW,  IPPROTO_UDP );
    if (sock < 0) 
    {
        printf("socket call failed: %s", strerror(errno));
        return -1;
    }

	const int one = 1;
	if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, (char *)&one, sizeof(one))< 0)
	{
		perror("IP_HDRINCL");
        return -1;
	}
    
    memset(&dest, 0, sizeof(dest));
    memset(buf, 0, sizeof(buf));
    packet_len = sizeof(struct udp_packet) + strlen(msg)+1;
    
    dest.sin_family = AF_INET;
    dest.sin_port = htons(dst_port);
    dest.sin_addr.s_addr = inet_addr(dst_ip);

	/* fill package. */
    packet->ip.protocol = IPPROTO_UDP;
    packet->ip.saddr = inet_addr(src_ip);
    packet->ip.daddr = inet_addr(dst_ip);
    packet->udp.source = htons(src_port);
    packet->udp.dest = htons(dst_port);
    
    packet->udp.len = htons(sizeof(packet->udp) + strlen(msg)+1); /* cheat on the psuedo-header */
   	packet->ip.tot_len = packet->udp.len;
    memcpy(packet->data, msg, strlen(msg)+1);
    packet->udp.check = checksum(packet, packet_len);

    packet->ip.tot_len = htons(packet_len);
    packet->ip.ihl = sizeof(packet->ip) >> 2;
    packet->ip.version = IPVERSION;
    packet->ip.ttl = IPDEFTTL;
    packet->ip.check = checksum(&(packet->ip), sizeof(packet->ip));

	TRACE;
    ret = sendto(sock, packet, packet_len, 0, (struct sockaddr *)&dest, sizeof(dest));
    print_package(packet,packet_len);
    if (ret <= 0) 
    {
        printf("sendto failed: %s", strerror(errno));
    }
    close(sock);

	printf("%s [%d] ret=%d\n", __FUNCTION__, __LINE__, ret);
    
    return ret;
}

int nat_type_cmd_deal(int sock, struct sockaddr_in *cli_addr, char *cmd)
{
	int ret = 0;
	char ret_buf[1024] = {0};
    int addr_len = sizeof(struct sockaddr);

	if(0 == strcmp("get ip_port", cmd))
	{
		sprintf(ret_buf, "%s:%d", inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));
		TRACE;
		ret = socket_send_to(sock, ret_buf, strlen(ret_buf)+1, cli_addr, addr_len);
        
		printf("%s [%d] Send[%d] : %s\n", __FUNCTION__, __LINE__,ret,  ret_buf);
        
        return 0;
    }

	if(0 == strncmp("send from", cmd, strlen("send from")))
	{
        char *p_ip;
        char *p_port;
        char *p_text;

		TRACE;
        p_ip = cmd + strlen("send from") + 1; //sip a space
        p_port = strchr(p_ip, ':');
        if(NULL == p_port)
            return -1;
        *p_port++ = '\0';

        p_text = strchr(p_port, ' ');
         if(NULL == p_text)
             return -1;
        *p_text++ = '\0';

		/* Local deal. */
       /* if(0 == strcmp(local_ip, p_ip) && local_port == atoi(p_port))
		{
			TRACE;
            socket_send_to(sock, p_text, strlen(p_text)+1, cli_addr, addr_len);
			return 0;
        }*/

		TRACE;
        /* Others. Use raw socket. */
        char dst_ip[32] = {0};
        strcpy(dst_ip, inet_ntoa(cli_addr->sin_addr));
		ret = send_nat_type_cmd_raw( p_ip, atoi(p_port), 
									dst_ip,ntohs(cli_addr->sin_port),p_text);
        
        return ret;
	}

    printf("%s [%d] Unknow command.\n", __FUNCTION__, __LINE__);

    return -1;
}


int main(int argc, char *argv[])
{
	int ret = -1;
	int sock = -1;
	struct sockaddr_in cli_addr;
    int addr_len;
    char recv_buf[1024] = {0};

	if(argc < 2)
	{
		printf("Need local listen IP.\nlike : nc_ser 192.168.226.64\n");
        return -1;
    }
	strcpy(local_ip, argv[1]);
    
	sock = socket_init();
	if(sock < 0)
    {
		return -1;
    }
	ret = socket_bind(sock, local_ip, local_port);
    if(ret < 0)
    {
        return -1;
    }

	while(1)
    {
	    ret = socket_select(sock, 2);
	    if(ret > 0)
		{
			memset(&cli_addr, 0x0, sizeof(cli_addr));
            addr_len = sizeof(cli_addr);
			memset(recv_buf, 0x0, sizeof(recv_buf));
            
			socket_receive_from(sock, recv_buf, sizeof(recv_buf),&cli_addr, &addr_len);
            
	        printf("%s [%d] Receive : %s\n", __FUNCTION__, __LINE__, recv_buf);
            
	        ret = nat_type_cmd_deal(sock, &cli_addr, recv_buf);
            if(ret < 0)
                printf("%s [%d] Deal message Error.\n", __FUNCTION__, __LINE__);
	    }
	    else
	    {
	        printf("%s [%d] ret=%d.\n", __FUNCTION__, __LINE__, ret);
	    }
    }
    
    socket_close(sock);

	return ret;
}
