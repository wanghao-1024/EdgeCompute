/**
 * @file	nat_probe.h
 * @author  <wanghao1@xunlei.com>
 * @date	Nov  9 21:07:06 CST 2016
 *
 * @brief	探测路由器的NAT类型的公共头文件
 *
 */

#ifndef __NAT_PROBE_H__
#define __NAT_PROBE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// 网络类型变化事件
#define NP_NETWORK_TYPE_CHANGE_EVENT	"nat_probe.network_type_change"

enum
{
	NP_UNKNOWN					= 0,	/**< 未知网络类型. */
	NP_PUBLIC_NETWORK,					/**< 公有网络. */
	NP_FULL_CONE_NAT,					/**< 全锥型NAT. */
	NP_RESTRICTED_CONE_NAT,				/**< 限制型NAT. */
	NP_PORT_RESTRICTED_CONE_NAT,		/**< 端口限制型NAT. */
	NP_SYMMETRIC_NAT,					/**< 对称型NAT. */
};

/// 外网服务器的域名.
#define NP_DOMAIN	"t05b022.p2cdn.com"

/// 外网服务器的端口.
#define NP_SERVER_PORT	(61620)

/// 做NAT类型判断需要公网IP的个数.
#define NP_PUBLIC_IP_NUM	(2)

/// 域名最大可以配置IP的个数
#define NP_MAX_PUBLIC_IP_NUM	(10)

struct np_request_msg_t
{
	int			msgid;			/**< 消息id. */
	int			network_type;	/**< 网络类型. */
};

struct np_response_msg_t
{
	int			msgid;			/**< 消息id. */
	uint32_t	ip_addr;		/**< nat后的IP地址. */
	uint16_t	port;			/**< nat后的IP端口. */
};


/// 计算数组元素个数.
#define ARRAY_SIZE(x)	(sizeof(x)/sizeof((x)[0]))

/**
 * 根据域名获取IP
 *
 * @param domain	域名缓存区.
 * @param pip_addr	ip地址缓存区.
 * @param ip_num    ip地址缓存区的大小.
 * @param pret_ip_num   成功解析ip的个数，不超过ip_num
 *
 */
int np_get_server_ip(const char* pdomain, uint32_t pip_addr[], int ip_num, int *pret_ip_num);

/**
 * 获取字符串格式的网络类型
 * @param network_type	网络类型
 * 
 */
const char *get_string_network_type(int network_type);

#ifdef __cplusplus
};
#endif
#endif