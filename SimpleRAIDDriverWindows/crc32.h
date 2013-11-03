#ifndef __CRC32_H_
#define __CRC32_H_

unsigned long update_crc(unsigned long crc,
		unsigned char * buf, unsigned long len);

#endif
