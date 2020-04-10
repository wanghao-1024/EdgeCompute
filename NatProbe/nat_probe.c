/**
 * @file	nat_probe_client.c
 * @author  <wanghao1>
 * @date	Nov  9 21:07:06 CST 2016
 *
 * @brief	探测路由器的NAT类型的公共代码
 *
 */

#include "nat_probe.h"
#include "log.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>


int np_get_server_ip(const char* pdomain, uint32_t pip_addr[], int ip_num, int *pret_ip_num)
{
	assert(pdomain != NULL);
	assert(pip_addr != NULL);
	
	int retval = -1, count = 0;
	struct addrinfo *answer = NULL, *curr = NULL;
	struct addrinfo hint;

	memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_DGRAM;

	/// 获取服务器域名对应的 IP 地址.
	retval = getaddrinfo(pdomain, NULL, &hint, &answer);
	if (retval != 0)
	{
		XL_DEBUG(EN_PRINT_ERROR, "call getaddrinfo() failed, domain: %s, err: %s", pdomain, gai_strerror(retval));
		goto ERR;
	}

	for (curr = answer; curr != NULL; curr = curr->ai_next) {
		XL_DEBUG(EN_PRINT_DEBUG, "count: %d, ip_addr: %s", count, inet_ntoa((((struct sockaddr_in *)(curr->ai_addr))->sin_addr)));
		/// XXX: 注意此处是网络序
		pip_addr[count++] = ((struct sockaddr_in *)(curr->ai_addr))->sin_addr.s_addr;
		if (count == ip_num)
		{
			break;
		}
	}
	if (count < NP_PUBLIC_IP_NUM)
	{
		XL_DEBUG(EN_PRINT_ERROR, "ip number is not enough, need: %d, count: %d", ip_num, count);
		goto ERR;
	}
	if (pret_ip_num != NULL)
	{
		*pret_ip_num = count;
		XL_DEBUG(EN_PRINT_NOTICE, "server ip num is %d", *pret_ip_num);
	}
	
	freeaddrinfo(answer);
	return 0;
ERR:
	freeaddrinfo(answer);
	return -1;
}

const char *get_string_network_type(int network_type)
{
	static char *s_network_type[] = {
		"UNKNOWN",
		"PUBLIC_NETWORK",
		"FULL_CONE_NAT",
		"RESTRICTED_CONE_NAT",
		"PORT_RESTRICTED_CONE_NAT",
		"SYMMETRIC_NAT",
	};
	if (network_type < NP_UNKNOWN || network_type > NP_SYMMETRIC_NAT)
	{
		network_type = NP_UNKNOWN;
	}
	return s_network_type[network_type];
}
