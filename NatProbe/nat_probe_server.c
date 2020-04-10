/**
 * @file	nat_probe_client.c
 * @author  <wanghao1>
 * @date	Nov  9 21:07:06 CST 2016
 *
 * @brief	探测路由器的NAT类型服务端
 *
 */

#include "log.h"
#include "nat_probe.h"
#include <event.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "cjson/cJSON.h"

/// SERVER端需要的socket的个数
#define NP_SERVER_SOCKET_NUM	(NP_MAX_PUBLIC_IP_NUM*2)

struct np_addr_t
{
	uint32_t			ip;				/**< ip. */
	uint16_t			port;			/**< port. */
	int					sock;			/**< socket. */
	struct event		event_read;		/**< 与socket关联的读事件. */
};

struct np_server_t
{
	struct np_addr_t			addr[NP_SERVER_SOCKET_NUM];	/**< 服务端对外提供的socket地址. */
	struct np_response_msg_t	send_msg;		/**< 待发送的数据. */
	struct np_request_msg_t		recv_msg;		/**< 接收到的数据. */
};

/// 销毁socket
static void destroy_socket(struct np_server_t *pnp_server)
{
	assert(pnp_server != NULL);

	int i = 0, size = ARRAY_SIZE(pnp_server->addr);

	for (i = 0; i < size; i++)
	{
		if (pnp_server->addr[i].sock >= 0)
		{
			close(pnp_server->addr[i].sock);
		}
	}
}

/// 创建socket
static int create_socket(struct np_server_t *pnp_server)
{
	assert(pnp_server != NULL);

	struct timeval timeout = { 0, 500000 };
	int sock = -1, retval = -1;
	struct sockaddr_in local_addr;
	uint32_t tmp_ip_addr[NP_MAX_PUBLIC_IP_NUM] = { 0 };
	int i = 0, j = 0, ret_ip_num = 0, create_sock_num = 0;

	if (-1 == np_get_server_ip(NP_DOMAIN, tmp_ip_addr, ARRAY_SIZE(tmp_ip_addr), &ret_ip_num))
	{
		XL_DEBUG(EN_PRINT_ERROR, "call get_server_ip() failed");
		goto ERR;
	}
	create_sock_num = ret_ip_num * 2;
	
	j = 0;
	for (i = 0; i < create_sock_num; i++)
	{
		pnp_server->addr[j].ip = tmp_ip_addr[i];
		pnp_server->addr[j].port = NP_SERVER_PORT;
		pnp_server->addr[j].sock = -1;
		pnp_server->addr[j+1].ip = tmp_ip_addr[i];
		pnp_server->addr[j+1].port = NP_SERVER_PORT + 1;
		pnp_server->addr[j+1].sock = -1;
		j += 2;
	}

	for (i = 0; i < create_sock_num; i++)
	{
		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock < 0)
		{
			XL_DEBUG(EN_PRINT_ERROR, "call socket() failed, err: %s", strerror(errno));
			goto ERR;
		}

		memset(&local_addr, 0, sizeof(local_addr));
		local_addr.sin_family = AF_INET;          
		local_addr.sin_addr.s_addr = pnp_server->addr[i].ip;
		local_addr.sin_port = htons(pnp_server->addr[i].port);

		XL_DEBUG(EN_PRINT_NOTICE, "call bind(), sock: %d, ip: %s, port: %u", sock, inet_ntoa(local_addr.sin_addr), pnp_server->addr[i].port);

		retval = bind(sock, (const struct sockaddr *)&local_addr, sizeof(struct sockaddr));
		if (retval < 0)
		{
			XL_DEBUG(EN_PRINT_WARN, "call bind() failed, bind address failed, err: %s", strerror(errno));
			close(sock);
			continue;
		}

		/// 设置收包的超时时间.
		retval = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		if (retval < 0)
		{
			XL_DEBUG(EN_PRINT_ERROR, "call setsockopt() failed, set SO_RCVTIMEO failed, err: %s", strerror(errno));
			close(sock);
			continue;
		}
		pnp_server->addr[i].sock = sock;
	}
	return 0;
ERR:
	destroy_socket(pnp_server);
	return -1;
}

/// 收消息
static int recv_msg(int sock, struct np_server_t *pnp_server, struct sockaddr_in *pclient_addr)
{
	assert(pnp_server != NULL);
	assert(pclient_addr != NULL);

	char *precv_msg = NULL;
	cJSON *proot = NULL, *pmsgid = NULL, *pnetwork_type = NULL;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	int tmp_len = 0;
	struct np_request_msg_t *precv = &(pnp_server->recv_msg);
	struct np_response_msg_t *psend = &(pnp_server->send_msg);
	
	precv_msg = (char *)malloc(128);
	if (NULL == precv_msg)
	{
		XL_DEBUG(EN_PRINT_ERROR, "call malloc() failed, err: %s", strerror(errno));
		goto ERR;
	}
	while (1)
	{
		tmp_len = recvfrom(sock, precv_msg, 128, 0, (struct sockaddr *)pclient_addr, &addr_len);
		if (tmp_len == -1)
		{
			if (errno == EINTR)
			{
				XL_DEBUG(EN_PRINT_DEBUG, "errno: %d", errno);
				continue;
			}
			else
			{
				XL_DEBUG(EN_PRINT_ERROR, "call recvfrom() failed, tmp_len: %d, errno: %d, err: %s", tmp_len, errno, strerror(errno));
				goto ERR;
			}
			goto ERR;
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
	pnetwork_type = cJSON_GetObjectItem(proot, "network_type");
	if (NULL == pnetwork_type)
	{
		XL_DEBUG(EN_PRINT_ERROR, "get msgid failed, precv_msg: %s", precv_msg);
		goto ERR;
	}
	precv->msgid = pmsgid->valueint;
	precv->network_type = pnetwork_type->valueint;
	psend->msgid = precv->msgid;
	// XXX:此处是网络序
	psend->ip_addr = pclient_addr->sin_addr.s_addr;
	psend->port = pclient_addr->sin_port;

	XL_DEBUG(EN_PRINT_DEBUG, "recv msg, msgid: %d, network_type: %s, ip_addr: %s, port: %u",
		precv->msgid, get_string_network_type(precv->network_type), inet_ntoa(pclient_addr->sin_addr),
		psend->port);

	free(precv_msg);
	cJSON_Delete(proot);
	return 0;
ERR:
	free(precv_msg);
	cJSON_Delete(proot);
	return -1;
}

/// 发送消息
static int send_msg(int sock, struct np_server_t *pnp_server, struct sockaddr_in *pclient_addr)
{
	assert(pnp_server != NULL);
	assert(pclient_addr != NULL);

	struct np_response_msg_t *psend = &(pnp_server->send_msg);
	cJSON *proot = NULL;
	char *psend_msg = NULL;
	int send_len = 0, tmp_len = 0;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	char ip_addr[32] = { '\0' };

	snprintf(ip_addr, sizeof(ip_addr)-1, "%u", psend->ip_addr);
	ip_addr[sizeof(ip_addr)-1] = '\0';

	proot = cJSON_CreateObject();
	cJSON_AddNumberToObject(proot, "msgid", psend->msgid);
	cJSON_AddStringToObject(proot, "ip", ip_addr);
	cJSON_AddNumberToObject(proot, "port", psend->port);
	psend_msg = cJSON_Print(proot);

	send_len = strlen(psend_msg);
	while (1)
	{
		tmp_len = sendto(sock, psend_msg, send_len, 0, (const struct sockaddr *)pclient_addr, addr_len);
		if (tmp_len == -1)
		{
			if (errno == EINTR)
			{
				continue;
			}
			else
			{
				XL_DEBUG(EN_PRINT_ERROR, "call recvfrom() failed, tmp_len: %d, err: %s", tmp_len, strerror(errno));
				goto ERR;
			}
		}
		if (tmp_len != send_len)
		{
			XL_DEBUG(EN_PRINT_DEBUG, "call sendto() failed, tmp_len: %d, send_len: %d", tmp_len, send_len);
			goto ERR;
		}
		break;
	}
	cJSON_Delete(proot);
	free(psend_msg);
	return 0;
ERR:
	cJSON_Delete(proot);
	free(psend_msg);
	return -1;
}

/// 全锥型nat处理
static int process_full_cone_nat(int sock, struct np_server_t *pnp_server)
{
	assert(pnp_server != NULL);

	int i = 0, size = ARRAY_SIZE(pnp_server->addr);
	for (i = 0; i < size; i++)
	{
		if (pnp_server->addr[i].port == NP_SERVER_PORT && pnp_server->addr[i].sock == sock)
		{
			// 同端口不同IP的socket回复
			return pnp_server->addr[(i+1+NP_PUBLIC_IP_NUM)%size].sock;
		}
	}
	return -1;
}

/// 限制锥型nat处理
static int process_restricted_cone_nat(int sock, struct np_server_t *pnp_server)
{
	assert(pnp_server != NULL);
	int i = 0, size = ARRAY_SIZE(pnp_server->addr);
	for (i = 0; i < size; i++)
	{
		if (pnp_server->addr[i].port == NP_SERVER_PORT && pnp_server->addr[i].sock == sock)
		{
			// 同IP不同端口的socket回复
			return pnp_server->addr[i+1].sock;
		}
	}
	return -1;
}

/// 首包处理函数
static int process(int sock, struct np_server_t *pnp_server)
{
	assert(pnp_server != NULL);

	struct np_request_msg_t *precv = &(pnp_server->recv_msg);

	switch(precv->network_type)
	{
	case NP_PUBLIC_NETWORK:
	case NP_SYMMETRIC_NAT:
	case NP_PORT_RESTRICTED_CONE_NAT:
		/// 收发同一个socket
		return sock;
	case NP_FULL_CONE_NAT:
		return process_full_cone_nat(sock, pnp_server);
	case NP_RESTRICTED_CONE_NAT:
		return process_restricted_cone_nat(sock, pnp_server);
	default:
		XL_DEBUG(EN_PRINT_ERROR, "error net type, %d", precv->network_type);
		return -1;
	}
	return -1;
}

/// read事件回调函数
static void read_callback(evutil_socket_t sock, short __attribute__((unused))event_type, void *args)
{
	struct np_server_t* pnp_server = (struct np_server_t *)args;
	struct sockaddr_in client_addr;

	memset(&client_addr, 0, sizeof(client_addr));

	XL_DEBUG(EN_PRINT_DEBUG, "sock: %d, recv_msg ...", sock);
	if (-1 == recv_msg(sock, pnp_server, &client_addr))
	{
		XL_DEBUG(EN_PRINT_ERROR, "call recv_msg() failed");
		return ;
	}
	XL_DEBUG(EN_PRINT_NOTICE, "client ip: %s, client port: %u", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);

	sock = process(sock, pnp_server);
	if (sock == -1)
	{
		return ;
	}

	XL_DEBUG(EN_PRINT_DEBUG, "sock: %d, send_msg ...\n", sock);
	if (-1 == send_msg(sock, pnp_server, &client_addr))
	{
		XL_DEBUG(EN_PRINT_ERROR, "call send_msg() failed");
		return ;
	}
}

/// 退出时间循环
static void exit_callback(evutil_socket_t __attribute__((unused))sock,
						  short __attribute__((unused))event_type,
						  void __attribute__((unused))*args)
{
	event_loopbreak();
}

int main(int __attribute__((unused))argc, char __attribute__((unused))*argv[])
{
	struct np_addr_t *tmp_addr = NULL;
	struct np_server_t np_server;
	struct event event_exit;
	int retval = -1;
	int i = 0, size = 0;
	int is_daemon = 1;

	
	retval = configure_log(EN_PRINT_NOTICE, "/var/log/nat_probe_server.log", 1);
	if (-1 == retval)
	{
		printf("call configure_log() failed");
		goto ERR;
	}

	if (argc > 1 && 0 == strcmp(argv[1], "nodaemon"))
	{
		is_daemon = 0;
	}
	if (is_daemon && -1 == daemon(0, 0))
	{
		XL_DEBUG(EN_PRINT_ERROR, "call daemon() failed, err: %s", strerror(errno));
		goto ERR;
	}

	memset(&np_server, 0, sizeof(np_server));
	retval = create_socket(&np_server);
	if (retval == -1)
	{
		XL_DEBUG(EN_PRINT_ERROR, "call create_socket() failed");
		goto ERR;
	}
	if (NULL == event_init())
	{
		XL_DEBUG(EN_PRINT_ERROR, "call event_init() failed");
		goto ERR;
	}

	size =  ARRAY_SIZE(np_server.addr);
	for (i = 0; i < size; i++)
	{
		tmp_addr = &(np_server.addr[i]);
		if (tmp_addr->port != NP_SERVER_PORT || tmp_addr->sock == -1)
		{
			continue;
		}
		event_set(&(tmp_addr->event_read), tmp_addr->sock, EV_READ|EV_PERSIST, read_callback, &np_server);
		retval = event_add(&(tmp_addr->event_read), NULL);
		if (-1 == retval)
		{
			XL_DEBUG(EN_PRINT_ERROR, "call event_add() failed");
			goto ERR;
		}
	}
	
	evsignal_set(&event_exit, SIGINT, exit_callback, NULL);
	event_add(&event_exit, NULL);
	
	XL_DEBUG(EN_PRINT_NOTICE, "enter event_dispatch()...");
	retval = event_dispatch();
	if (-1 == retval)
	{
		XL_DEBUG(EN_PRINT_ERROR, "call event_dispatch() failed");
		goto ERR;
	}
	destroy_socket(&np_server);
	destroy_log();
	return 0;
ERR:
	destroy_socket(&np_server);
	destroy_log();
	return -1;
}
