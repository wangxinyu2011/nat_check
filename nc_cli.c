/*
	NAT check client process.

	wangxinyu.yy@gmail.com
	2016.9.14

	I will complete it today, and I want to do this long ago.

	Let me go and do.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "log_debug.h"
#include "common.h"

static char ser_ip[32] = {0};
static int ser_port = SERVER_TEST_PORT;

static char local_ip[32] = {0};
static int local_port = CLIENT_TEST_PORT;


char *nat_type_str[] = {
	"No NAT",
	"Full Cone NAT",
	"Address Restricted Cone NAT",
    "Port Restricted Cone NAT",
    "Symmetric NAT"
};


void usage(void)
{
    printf("nc_cli localIp serIp[:serPort]\n");	
    
    printf("\t like : \n\t\tnc_cli 192.168.226.64 192.168.100.2:8867\n");	
}

int get_remote_ser_info(char *cmd)
{
    char *p = NULL;
    int no_port = 1;

	p = strchr(cmd, ':');
    if( p)
    {
        no_port = 0;
	    *p = '\0';
	    p++;
	}
    strncpy(ser_ip, cmd, sizeof(ser_ip) -1);
    ser_ip[sizeof(ser_ip) -1] = '\0';

	if(no_port)
        return 0;

	ser_port = strtoul(p, NULL, 10);

    if(ser_port < 1 || ser_port > 0xFFFF)
    {
    	printf("%s [%d] server port error[%d].\n", __FUNCTION__, __LINE__, ser_port);
		return -1;
    }
    
	return 0;
}

int send_nat_type_cmd(char *dst_ip, int dst_port, char *in_cmd, char *out_buf, int out_len)
{
	int ret = -1;
	int sock = -1;
    struct sockaddr_in addr;
    int addr_len;

	sock = socket_init();
	if(sock < 0)
    {
    	TRACE;
		return -1;
    }
	ret = socket_bind(sock, local_ip, local_port);
    if(ret < 0)
    {
    	TRACE;
        return -1;
    }
	addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(dst_ip);
    addr.sin_port = htons(dst_port);
    addr_len = sizeof(addr);
	//ret = socket_connect(sock, dst_ip, dst_port);  
	
    ret = socket_send_to(sock, in_cmd, strlen(in_cmd) + 1, &addr, addr_len);
    if(ret < 0)
    {
    	TRACE;
        return -1;
    }
	printf("%s [%d] Send : %s\n", __FUNCTION__, __LINE__, in_cmd);
    
    //ret = get_local_sock_info(sock, local_ip, &local_port);
    //printf("%s [%d] local %s:%d\n", __FUNCTION__, __LINE__, local_ip, local_port);
    
    ret = socket_select(sock, 2);
    if(ret > 0)
	{
		memset(&addr, 0x0, sizeof(addr));
        addr_len = sizeof(addr);
		socket_receive_from(sock, out_buf, out_len, &addr, &addr_len);
        printf("%s [%d] Receive(%s:%d) : %s\n", __FUNCTION__, __LINE__, 
            	inet_ntoa(addr.sin_addr), ntohs(addr.sin_port) , out_buf);
        
    }
    else
    {
        printf("%s [%d] Receive Time out.\n", __FUNCTION__, __LINE__);
    }
    
    socket_close(sock);

	return ret;
}

/*
	Return : Success return Nat type, if error happens return -1.
*/

int nat_type_check()
{
	int ret = -1;
	char rtn_buf[1024] = {0};
    char buf_tmp[1024] = {0};
    char buf_tmp1[1024] = {0};
    int nat_type = NAT_TYPE_MAX;
    char tmp_ser_ip[32] = {0};
    int tmp_ser_port = 0;
	int i = 0;
	/*
		Send command : get ip_port
    */

    ret = send_nat_type_cmd(ser_ip, ser_port, "get ip_port", rtn_buf, sizeof(rtn_buf));
    if(ret <= 0)
	{
		printf("%s [%d] Error happen.\n", __FUNCTION__, __LINE__);
		return -1;
    }
  
    sprintf(buf_tmp, "%s:%d", local_ip, local_port);
    if( 0 == strcmp(buf_tmp, rtn_buf))
	{
		return NO_NAT;
    }

	/*
		Send command : send from ip:port test_text
		ip is not server IP.
    */
	memset(rtn_buf, 0x0, sizeof(rtn_buf));
    // Just little change, from 10.0.0.1 to 10.0.0.2
    struct in_addr in;
    in.s_addr = inet_addr(ser_ip);
    in.s_addr = ntohl(in.s_addr) + 1;  
    in.s_addr = htonl(in.s_addr);
   	strcpy(tmp_ser_ip, (char *)inet_ntoa(in));     
    tmp_ser_port = ser_port;
    
    sprintf(buf_tmp, "send from %s:%d %s", tmp_ser_ip, tmp_ser_port, TEST_TEXT);
    ret = send_nat_type_cmd(ser_ip, ser_port, buf_tmp, rtn_buf, sizeof(rtn_buf));
    if(ret < 0)
	{
		printf("%s [%d] Error happen.\n", __FUNCTION__, __LINE__);
		return -1;
    }
    if( 0 == strcmp(TEST_TEXT, rtn_buf))
	{
		return FULL_CONE_NAT;
    }



	/*
		Send command : send from ip:port test_text
		ip is  server IP, buf port is not Default port.
    */
    memset(rtn_buf, 0x0, sizeof(rtn_buf));
    // Just little change, change port.
    strcpy(tmp_ser_ip, ser_ip);     
    tmp_ser_port = ser_port + 1;
    
    sprintf(buf_tmp, "send from %s:%d %s", tmp_ser_ip, tmp_ser_port, TEST_TEXT);
    ret = send_nat_type_cmd(ser_ip, ser_port, buf_tmp, rtn_buf, sizeof(rtn_buf));
    if(ret < 0)
	{
		printf("%s [%d] Error happen.\n", __FUNCTION__, __LINE__);
		return -1;
    }
    if( 0 == strcmp(TEST_TEXT, rtn_buf))
    {
        return ADDR_STRICT_CONE_NAT;
	}

	/*
		Use "get ip_port" command to check whether Symmetric  NAT.
    */
    memset(buf_tmp, 0x0, sizeof(buf_tmp));
    memset(buf_tmp1, 0x0, sizeof(buf_tmp1));
    ret = send_nat_type_cmd(ser_ip, ser_port, "get ip_port", buf_tmp, sizeof(buf_tmp));
    if(ret < 0)
	{
		printf("%s [%d] Error happen.\n", __FUNCTION__, __LINE__);
		return -1;
    }
    printf("%s [%d] Give you 60 seconds to clear ConnectTrace.\n", __FUNCTION__, __LINE__);
	for(i = 60 ; i > 0; i-=2)	
    {
		sleep(2);
		printf("%d seconds remaind.\n", i);
	}
    ret = send_nat_type_cmd(ser_ip, ser_port, "get ip_port", buf_tmp1, sizeof(buf_tmp1));
    if(ret < 0)
	{
		printf("%s [%d] Error happen.\n", __FUNCTION__, __LINE__);
		return -1;
    }
    if( 0 == strcmp(buf_tmp, buf_tmp1))
	{
		return PORT_ADDR_STRICT_CONE_NAT;
    }

	return SYMMETRIC_NAT;
}

int main(int argc, char *argv[])
{
	int ret = -1;

    if((argc < 3) || (get_remote_ser_info(argv[2]) < 0))
	{
		usage();
        return -1;
    }

    strcpy(local_ip, argv[1]);

	printf("%s [%d] (%s) Begin to Connet server %s:%d\n", __FUNCTION__, __LINE__,
        											local_ip, ser_ip, ser_port);

	/* Real check work here. */
	ret = nat_type_check();
	if(ret < 0 || ret > NAT_TYPE_MAX)
	{
		printf("%s [%d] NAT check error happen.\n", __FUNCTION__, __LINE__);
    }
	else
    {
		printf("\n*************************\n", __FUNCTION__, __LINE__);
        printf("Result : %s", nat_type_str[ret]);
		printf("\n*************************\n", __FUNCTION__, __LINE__);
    }
    
	return 0;
}

