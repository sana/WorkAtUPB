/*
 * SO2 - Homework #5 - Stateful firewall (Windows)
 *
 * ipfirewall header file
 */

#ifndef _IPFIREWALL_H_
#define _IPFIREWALL_H_

#define FIREWALL_DEVICE		"\\\\.\\ipfirewall"

#define FW_ADD_RULE			CTL_CODE(FILE_DEVICE_UNKNOWN, 1, \
								METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define	FW_ENABLE			CTL_CODE(FILE_DEVICE_UNKNOWN, 2, \
								METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define FW_DISABLE			CTL_CODE(FILE_DEVICE_UNKNOWN, 3, \
								METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define FW_LIST				CTL_CODE(FILE_DEVICE_UNKNOWN, 4, \
								METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

struct fwr
{
	unsigned int ip_src, ip_dst, ip_src_mask, ip_dst_mask;
	unsigned short port_src[2], port_dst[2];
};

#endif /* _IPFIREWALL_H_ */
