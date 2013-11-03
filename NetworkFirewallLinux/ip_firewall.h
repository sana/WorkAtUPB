/*
 * SO2 - Homework #5 - Stateful firewall (Linux)
 *
 * ipfirewall header file
 */

#ifndef _IPFIREWALL_H_
#define _IPFIREWALL_H_

#include <asm/ioctl.h>

#define IPFIREWALL_MAJOR	42

#define FW_ADD_RULE		_IOW ('k', 1, struct fwr *)
#define	FW_ENABLE		_IO  ('k', 2)
#define FW_DISABLE		_IO  ('k', 3)
#define FW_LIST			_IOWR('k', 4, void *)

struct fwr {
	unsigned int ip_src, ip_dst, ip_src_mask, ip_dst_mask;
	unsigned short port_src[2], port_dst[2];
};

#endif /* _IPFIREWALL_H_ */
