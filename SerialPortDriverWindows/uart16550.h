#ifndef _UART16550_H
#define _UART16550_H

#define UART16550_NO 2

#define	OPTION_COM1			1
#define OPTION_COM2			2
#define OPTION_BOTH			3

#define OPTION_SENDER_ONLY 2
#define OPTION_RECEIVER_ONLY 1

#ifndef _UART16550_REGS_H

#define UART16550_BAUD_1200		96
#define UART16550_BAUD_2400		48
#define UART16550_BAUD_4800		24
#define UART16550_BAUD_9600		12
#define UART16550_BAUD_19200		6
#define UART16550_BAUD_38400		3
#define UART16550_BAUD_56000		2
#define UART16550_BAUD_115200		1

#define UART16550_LEN_5			0x00
#define UART16550_LEN_6			0x01
#define UART16550_LEN_7			0x02
#define UART16550_LEN_8			0x03

#define UART16550_STOP_1		0x00
#define UART16550_STOP_2		0x04

#define UART16550_PAR_NONE		0x00
#define UART16550_PAR_ODD		0x08
#define UART16550_PAR_EVEN		0x18
#define UART16550_PAR_STICK		0x20

#endif 

#ifndef CTL_CODE

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#endif 

#define	UART16550_IOCTL_SET_LINE	CTL_CODE(0xf000, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA)
#define UART16550_IOCTL_FLUSH_OUT	CTL_CODE(0xf000, 0x802, METHOD_BUFFERED, FILE_WRITE_DATA)
#define UART16550_IOCTL_CLEAR_IN	CTL_CODE(0xf000, 0x803, METHOD_BUFFERED, FILE_WRITE_DATA)

struct uart16550_line_info
{
	unsigned char baud, len, par, stop;
};

#endif 
