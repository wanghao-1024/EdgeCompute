/**
 * @file	nat_probe_client.c
 * @author  <wanghao1>
 * @date	Nov  9 21:07:06 CST 2016
 *
 * @brief	探测路由器的NAT类型客户端
 *
 */



#include "nat_probe.h"
#include "log.h"
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <netdb.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <linux/ethtool.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include "cjson/cJSON.h"


/// 向外发包重试的次数，因为udp不可靠，所有多重试几次
#define NP_RETRY_SEND_NUM (2)

/// 收发包的个数，必须与NP_PUBLIC_IP_NUM一致
#define NP_SEND_AND_RECV_NUM (NP_PUBLIC_IP_NUM)

/// 检测NAT类型的间隔时间，24小时.
#define NP_NAT_PROBE_INTERVAL_TIME	(24*60*60*1000)

struct np_client_t
{
	int							sock;								/**< socket. */	
	int                         ip_addr_num;						/**< 目的服务器有效IP的个数. >*/
	uint32_t					ip_addr[NP_MAX_PUBLIC_IP_NUM];		/**< 与赚钱宝同运营商的外网IP. */
	struct np_request_msg_t		send_msg[NP_SEND_AND_RECV_NUM];		/**< 待发送的数据. */
	struct np_response_msg_t	recv_msg[NP_SEND_AND_RECV_NUM];		/**< 接收到的数据. */
};

/// 赚钱宝所在的网络类型.
static int s_network_type = NP_UNKNOWN;

/// 生成消息id
static int generate_msgid()
{
	static int msgid = 0;

	if (msgid == 0)
	{
		msgid = (int)getpid();
	}
	msgid++;
	XL_DEBUG(EN_PRINT_DEBUG, "msgid: %d", msgid);
	return msgid;
}

/// 判断是否是赚钱宝自己配置的IP
static int is_local_ip(const uint32_t ip_addr)
{
	int sock = -1;
	struct ifreq ifr, *it = NULL, *end = NULL;
	struct ifconf ifc;
	struct  sockaddr_in local_addr;
	int addr_len = sizeof(local_addr);
	char tmp_buf[128] = { '\0' };

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
	{
		XL_DEBUG(EN_PRINT_ERROR, "create socket failed, err: %s", strerror(errno));
		goto ERR;
	}

	ifc.ifc_len = sizeof(tmp_buf);
	ifc.ifc_buf = tmp_buf;

	// 获取每一个网卡的配置
	if (-1 == ioctl(sock, SIOCGIFCONF, &ifc))
	{
		XL_DEBUG(EN_PRINT_ERROR, "ioctl SIOCGIFCONF failed, err: %s\n", strerror(errno));
		goto ERR;
	}

	it = ifc.ifc_req;
	end = it + (ifc.ifc_len / sizeof(struct ifreq));
	for (; it != end; ++it) 
	{
		strcpy(ifr.ifr_name, it->ifr_name);
		if (-1 == ioctl(sock, SIOCGIFFLAGS, &ifr))
		{
			XL_DEBUG(EN_PRINT_ERROR, "ioctl SIOCGIFFLAGS failed, ifr_name: %s, err: %s\n", ifr.ifr_name, strerror(errno));
			continue;
		}
		if ((ifr.ifr_flags & IFF_LOOPBACK))
		{
			continue;
		}
		//获取IP地址
		if (-1 == ioctl(sock, SIOCGIFADDR, &ifr))
		{
			XL_DEBUG(EN_PRINT_ERROR, "ioctl SIOCGIFADDR failed, ifr_name: %s, err: %s\n", ifr.ifr_name, strerror(errno));
			continue;
		}
		memcpy(&local_addr, &ifr.ifr_addr, addr_len);
		XL_DEBUG(EN_PRINT_DEBUG, "nic name: %s, ip_addr: %u, ip_addr: %u", ifr.ifr_name, local_addr.sin_addr.s_addr, ip_addr);
		if (local_addr.sin_addr.s_addr == ip_addr)
		{
			close(sock);
			return 1;
		}
	}
	close(sock);
	return 0;
ERR:
	close(sock);
	return 0;
}

/// 创建socket
static int create_socket()
{
	struct timeval timeout = { 0, 500000 }; //500ms
	int sock = -1, retval = -1;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		XL_DEBUG(EN_PRINT_ERROR, "call socket() failed, err: %s", strerror(errno));
		goto ERR;
	}
	/// 设置收到的超时时间
	retval = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	if (retval < 0)
	{
		XL_DEBUG(EN_PRINT_ERROR, "call setsockopt() failed, set SO_RCVTIMEO failed, err: %s", strerror(errno));
		goto ERR;
	}
	return sock;
ERR:
	if (sock >= 0)
	{
		close(sock);
	}
	return -1;
}

/// 发送消息
static int send_msg(struct np_client_t *pnp_client, int idx, const struct sockaddr_in *pdst_addr)
{
	assert(pnp_client != NULL);
	assert(pdst_addr != NULL);

	cJSON *proot = NULL;
	struct np_request_msg_t *psend = &(pnp_client->send_msg[idx]);
	char *psend_msg = NULL;
	int send_len = 0, tmp_len = 0;
	socklen_t addr_len = sizeof(struct sockaddr_in);

	proot = cJSON_CreateObject();
	cJSON_AddNumberToObject(proot, "msgid", psend->msgid);
	cJSON_AddNumberToObject(proot, "network_type", psend->network_type);
	psend_msg = cJSON_Print(proot);
	cJSON_Delete(proot);

	XL_DEBUG(EN_PRINT_DEBUG, "psend_msg: %s", psend_msg);

	send_len = strlen(psend_msg);

	while (1)
	{
		tmp_len = sendto(pnp_client->sock, psend_msg, send_len, 0, (const struct sockaddr *)pdst_addr, addr_len);
		if (-1 == tmp_len)
		{
			if (errno == EINTR)
			{
				continue;
			}
			else
			{
				XL_DEBUG(EN_PRINT_DEBUG, "[error] call sendto() failed, tmp_len: %d, err: %s", tmp_len, strerror(errno));
				goto ERR;
			}
		}
		if (send_len != tmp_len)
		{
			XL_DEBUG(EN_PRINT_DEBUG, "[error] call sendto() failed, tmp_len: %d, send_len: %d", tmp_len, send_len);
			goto ERR;
		}
		break;
	}

	free(psend_msg);
	return 0;
ERR:
	free(psend_msg);
	return -1;
}

/// 接收消息
static int recv_msg(struct np_client_t *pnp_client, int idx)
{
	assert(pnp_client != NULL);
	char *precv_msg = NULL;
	int tmp_len = 0;
	cJSON *proot = NULL, *pmsgid = NULL,  *pip = NULL, *pport = NULL;
	struct np_response_msg_t *precv = &(pnp_client->recv_msg[idx]);

	precv_msg = malloc(128);
	if (NULL == precv_msg)
	{
		XL_DEBUG(EN_PRINT_DEBUG, "[error] call malloc() failed, err: %s", strerror(errno));
		goto ERR;
	}
	while (1)
	{
		tmp_len = recvfrom(pnp_client->sock, precv_msg, 128, 0, NULL, NULL);
		if (tmp_len == -1)
		{
			if (errno == EINTR)
			{
				continue;
			}
			else
			{
				XL_DEBUG(EN_PRINT_DEBUG, "[error] call recvfrom() failed, tmp_len: %d, err: %s", tmp_len, strerror(errno));
				goto ERR;
			}
		}
		break;
	}

	proot = cJSON_Parse(precv_msg);
	if (NULL == proot)
	{
		XL_DEBUG(EN_PRINT_ERROR, "call cJSON_Parse() failed, precv_msg: %s", precv_msg);
		goto ERR;
	}
	pmsgid = cJSON_GetObjectItem(proot, "msgid");
	if (NULL == pmsgid)
	{
		XL_DEBUG(EN_PRINT_ERROR, "get msgid failed, precv_msg: %s", precv_msg);
		goto ERR;
	}
	pip = cJSON_GetObjectItem(proot, "ip");
	if (NULL == pip)
	{
		XL_DEBUG(EN_PRINT_ERROR, "get ip failed, precv_msg: %s", precv_msg);
		goto ERR;
	}
	pport = cJSON_GetObjectItem(proot, "port");
	if (NULL == pport)
	{
		XL_DEBUG(EN_PRINT_ERROR, "get port failed, precv_msg: %s", precv_msg);
		goto ERR;
	}
	precv->msgid = pmsgid->valueint;
	precv->port = pport->valueint;
	precv->ip_addr = strtoul(pip->valuestring, NULL, 10);

	cJSON_Delete(proot);
	free(precv_msg);
	return 0;
ERR:
	cJSON_Delete(proot);
	free(precv_msg);
	return -1;
}

/// 发送&接收消息
static int np_send_and_recv_msg(struct np_client_t *pnp_client, int network_type, int send_msg_num)
{
	assert(pnp_client != NULL);
	assert(send_msg_num <= NP_PUBLIC_IP_NUM);

	int i = 0, j = 0;
	struct sockaddr_in dst_addr;

	for (i = 0, j = 0; i < pnp_client->ip_addr_num && j < send_msg_num; i++)
	{
		memset(&dst_addr, 0, sizeof(dst_addr));
		dst_addr.sin_family = AF_INET;
		dst_addr.sin_port = htons(NP_SERVER_PORT);
		dst_addr.sin_addr.s_addr = pnp_client->ip_addr[i];

		XL_DEBUG(EN_PRINT_DEBUG, "dst_addr, ip: %s", inet_ntoa(dst_addr.sin_addr));

		pnp_client->send_msg[j].msgid = generate_msgid();
		pnp_client->send_msg[j].network_type = network_type;

		int retry_num = NP_RETRY_SEND_NUM;
		while (retry_num > 0)
		{
			XL_DEBUG(EN_PRINT_DEBUG, "retry_num: %d", retry_num);
			retry_num--;

			if (-1 == send_msg(pnp_client, j, &dst_addr))
			{
				// XXX:此IP不通，换下一个IP发送
				XL_DEBUG(EN_PRINT_DEBUG, "call send_msg() failed");
				break;
			}

			if (0 == recv_msg(pnp_client, j))
			{
				if (pnp_client->recv_msg[j].msgid == pnp_client->send_msg[j].msgid)
				{
					// 收到正确的回复
					j++;
					break;
				}
			}
			sleep(1);
		}
	}
	if (j < send_msg_num)
	{
		XL_DEBUG(EN_PRINT_DEBUG, "recvfrom msg failed");
		return -1;
	}
	return 0;
}

static int np_is_public_network(struct np_client_t *pnp_client, int *pnetwork_type)
{
	assert(pnp_client != NULL);
	assert(pnetwork_type != NULL);

	struct np_response_msg_t *precv = &(pnp_client->recv_msg[0]);
	struct in_addr in = { 0 };

	if (-1 == np_send_and_recv_msg(pnp_client, NP_PUBLIC_NETWORK, 1))
	{
		XL_DEBUG(EN_PRINT_DEBUG, "[error] call np_send_and_recv_msg() failed");
		return -1;
	}
	
	in.s_addr = precv->ip_addr;
	XL_DEBUG(EN_PRINT_NOTICE, "nat after the conversion, ip: %s, port: %u", inet_ntoa(in), precv->port);

	if (!is_local_ip(precv->ip_addr))
	{
		return -1;
	}
	XL_DEBUG(EN_PRINT_NOTICE, "nat type is NP_PUBLIC_NETWORK");
	*pnetwork_type = NP_PUBLIC_NETWORK;
	return 0;
}

/// 判断网络类型是否是对称型nat
static int np_is_symmetric_nat(struct np_client_t *pnp_client, int *pnetwork_type)
{
	assert(pnp_client != NULL);
	assert(pnetwork_type != NULL);

	struct np_response_msg_t *precv1 = &(pnp_client->recv_msg[0]);
	struct np_response_msg_t *precv2 = &(pnp_client->recv_msg[1]);
	struct in_addr in = { 0 };

	if (-1 == np_send_and_recv_msg(pnp_client, NP_SYMMETRIC_NAT, 2))
	{
		XL_DEBUG(EN_PRINT_DEBUG, "[error] call np_send_and_recv_msg() failed");
		return -1;
	}
	
	in.s_addr = precv1->ip_addr;
	XL_DEBUG(EN_PRINT_NOTICE, "nat after the conversion 1, ip: %s, port: %u", inet_ntoa(in), precv1->port);
	in.s_addr = precv2->ip_addr;
	XL_DEBUG(EN_PRINT_NOTICE, "nat after the conversion 2, ip: %s, port: %u", inet_ntoa(in), precv2->port);

	if (precv1->ip_addr != precv2->ip_addr || precv1->port != precv2->port)
	{
		XL_DEBUG(EN_PRINT_NOTICE, "nat type is SYMMETRIC_NAT");
		*pnetwork_type = NP_SYMMETRIC_NAT;
		return 0;
	}
	return -1;
}

/// 判断网络类型是否是全锥型nat
static int np_is_full_cone_nat(struct np_client_t *pnp_client, int *pnetwork_type)
{
	assert(pnp_client != NULL);
	assert(pnetwork_type);

	if (-1 == np_send_and_recv_msg(pnp_client, NP_FULL_CONE_NAT, 1))
	{
		XL_DEBUG(EN_PRINT_DEBUG, "[error] call np_send_and_recv_msg() failed");
		return -1;
	}
	XL_DEBUG(EN_PRINT_NOTICE, "nat type is NP_FULL_CONE_NAT");
	*pnetwork_type = NP_FULL_CONE_NAT;
	return 0;
}

/// 判断网络类型是否是限制锥型nat
static int np_is_restricted_cone_nat(struct np_client_t *pnp_client, int *pnetwork_type)
{
	assert(pnp_client != NULL);
	assert(pnetwork_type);

	if (-1 == np_send_and_recv_msg(pnp_client, NP_RESTRICTED_CONE_NAT, 1))
	{
		XL_DEBUG(EN_PRINT_DEBUG, "[error] call np_send_and_recv_msg() failed");
		return -1;
	}
	XL_DEBUG(EN_PRINT_NOTICE, "nat type is NP_RESTRICTED_CONE_NAT");
	*pnetwork_type = NP_RESTRICTED_CONE_NAT;
	return 0;
}

/// 判断网络类型是否是端口限制锥型nat
static int np_is_port_restricted_cone_nat(struct np_client_t *pnp_client, int *pnetwork_type)
{
	assert(pnp_client != NULL);
	assert(pnetwork_type);

	if (-1 == np_send_and_recv_msg(pnp_client, NP_PORT_RESTRICTED_CONE_NAT, 1))
	{
		XL_DEBUG(EN_PRINT_DEBUG, "[error] call np_send_and_recv_msg() failed");
		return -1;
	}
	XL_DEBUG(EN_PRINT_NOTICE, "nat type is NP_PORT_RESTRICTED_CONE_NAT");
	*pnetwork_type = NP_PORT_RESTRICTED_CONE_NAT;
	return 0;
}

/// 网络类型探测
static void np_network_type_probe()
{
	XL_DEBUG(EN_PRINT_NOTICE, "nat probe start...");

	int network_type = NP_UNKNOWN;
	struct np_client_t np_client;

	memset(&np_client, 0, sizeof(np_client));

	np_client.sock = create_socket();
	if (np_client.sock == -1)
	{
		goto EXIT;
	}

	if (-1 == np_get_server_ip(NP_DOMAIN, np_client.ip_addr, ARRAY_SIZE(np_client.ip_addr), &(np_client.ip_addr_num)))
	{
		XL_DEBUG(EN_PRINT_ERROR, "call np_get_server_ip() failed");
		goto EXIT;
	}
	XL_DEBUG(EN_PRINT_DEBUG, "ip_addr_num: %d", np_client.ip_addr_num);

	XL_DEBUG(EN_PRINT_NOTICE, "np_is_public_network...");
	if (0 == np_is_public_network(&np_client, &network_type))
	{
		goto EXIT;
	}
	XL_DEBUG(EN_PRINT_NOTICE, "np_is_symmetric_nat...");
	if (0 == np_is_symmetric_nat(&np_client, &network_type))
	{
		goto EXIT;
	}	
	XL_DEBUG(EN_PRINT_NOTICE, "np_is_full_cone_nat...");	
	if (0 == np_is_full_cone_nat(&np_client, &network_type))
	{
		goto EXIT;
	}
	XL_DEBUG(EN_PRINT_NOTICE, "np_is_restricted_cone_nat...");	
	if (0 == np_is_restricted_cone_nat(&np_client, &network_type))
	{
		goto EXIT;
	}
	XL_DEBUG(EN_PRINT_NOTICE, "np_is_port_restricted_cone_nat...");	
	if (0 == np_is_port_restricted_cone_nat(&np_client, &network_type))
	{
		goto EXIT;
	}
EXIT:
	if (network_type != NP_UNKNOWN && s_network_type != network_type)
	{
		s_network_type = network_type;
	}
	if (np_client.sock >= 0)
	{
		close(np_client.sock);
	}
	XL_DEBUG(EN_PRINT_NOTICE, "current network_type: %d", s_network_type);
}

int main(int __attribute__((unused))argc, char __attribute__((unused))*argv[])
{
	configure_log(EN_PRINT_NOTICE, NULL, 1);
	np_network_type_probe();
	XL_DEBUG(EN_PRINT_NOTICE, "nat type is %s", get_string_network_type(s_network_type));
	destroy_log();
	return 0;
}
