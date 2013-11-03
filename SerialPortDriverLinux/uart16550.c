// Laurentiu Dascalu, 342C3
// Tema 2 SO2, driver pentru interfata seriala
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include "uart16550.h"

MODULE_AUTHOR("Laurentiu Dascalu");
MODULE_DESCRIPTION("Serial cdev");
MODULE_LICENSE("GPL");

int major, option, info[2] =
{ 0, 0 };
module_param( major, int, S_IRUSR);
MODULE_PARM_DESC(major, "major used to register the device");
module_param( option, int, S_IRUSR);
MODULE_PARM_DESC(option, "OPTION_BOTH (COM1 + COM2), OPTION_COM1 and OPTION_COM2");

#define LOG_LEVEL                 KERN_ALERT

#define IER(baseport)             ((baseport) + 1)
#define IIR(baseport)             ((baseport) + 2)
#define FCR(baseport)             ((baseport) + 2)
#define MCR(baseport)             ((baseport) + 4)
#define LSR(baseport)             ((baseport) + 5)
#define IIR_RDAI(X)               (((X) >> 2) & 0x1)
#define IIR_THREI(X)	          (((X) >> 1) & 0x1)

// Dezactivez intreruperile pentru citire
#define IER_RDAI	          0x01

// Dezactivez intreruperile pentru scriere
#define IER_THREI	          0x02

// COM1
#define COM1_BASEPORT             0x3F8
#define COM1_IRQ                  4

// COM2
#define COM2_BASEPORT             0x2F8
#define COM2_IRQ                  3

#define COM_PORTS                 8
#define SERIAL_MAX_MINORS         2
#define MODULE_NAME               "uart16550"
#define BUFFER_SIZE               PAGE_SIZE
#define SIZE                      512

#define READ_OP                    98
#define WRITE_OP                   99

// Macro dintr-un laborator
#define SET_LINE(baud, len, stop, par, base)\
do\
{\
	outb(0x80,(base)+3); \
	outb((char)((baud)&0x00FF),(base)); \
	outb((char)((baud)&0xFF00),(base)+1); \
	outb(0x0,(base)+3); \
	outb((char)((len)|(stop)|(par)),(base)+3);\
} while (0)

struct serial_dev
{
	struct cdev cdev;
	char read_buffer[SIZE], write_buffer[SIZE];
	atomic_t read_ready, write_ready;
	int read, write;
	int base_port;
	wait_queue_head_t wq_reads, wq_writes;
} uart[SERIAL_MAX_MINORS];

irqreturn_t serial_interrupt_handle(int irq_no, void *dev_id)
{
	struct serial_dev *dev = (struct serial_dev *) dev_id;
	unsigned char iir, ch = 0, i;

	iir = inb(IIR(dev->base_port));	
	//printk(LOG_LEVEL "[serial interrupt handle] read IIR = %d", (int) iir);

	if (IIR_RDAI(iir))
	{
		// Dezactivez întreruperea RDAI - read
		outb(inb(IER(dev->base_port)) & ~IER_RDAI, IER(dev->base_port));
		//printk(LOG_LEVEL "[serial interrupt handle] reading data...");

		while (inb(LSR(dev->base_port)) & 0x01)
		{
			ch = inb(dev->base_port);
			dev->read_buffer[dev->read++] = ch;
		}

		atomic_set(&dev->read_ready, 1);
		wake_up(&dev->wq_reads);
		//printk(LOG_LEVEL "[serial interrupt handle] wake up read()");
	}

	if (IIR_THREI(iir))
	{
		// Dezactivez întreruperea THREI - write
		outb(inb(IER(dev->base_port)) & ~IER_THREI, IER(dev->base_port));
		//printk(LOG_LEVEL "[serial interrupt handle] writing data...");

		for (i = 0; i < dev->write; i++)
			outb(dev->write_buffer[i], dev->base_port);
		
		atomic_set(&dev->write_ready, 0);
		wake_up(&dev->wq_writes);
		//printk(LOG_LEVEL "[serial interrupt handle] wake up write()");
	}
	return IRQ_HANDLED;
}

static int serial_open(struct inode *inode, struct file *file)
{
	struct serial_dev *dev = container_of(inode->i_cdev, struct serial_dev, cdev);
	file->private_data = dev;
	return 0;
}

static int serial_close(struct inode *inode, struct file *file)
{
	return 0;
}

static int serial_read(struct file *file, char *user_buffer, size_t size,
		loff_t *offset)
{
	struct serial_dev *dev = (struct serial_dev*) file->private_data;
	int result;

	//printk(LOG_LEVEL "[serial read] 0x%x reads: %d", dev->base_port, size);
	if (size == 0)
		return 0;

	if (wait_event_interruptible(dev->wq_reads, atomic_cmpxchg(&dev->read_ready, 0, 0) == 1))
	{
		//printk(LOG_LEVEL "[serial read] wait_event_interruptible failed");
		return -ERESTARTSYS;
	}

	result = dev->read;
	if (copy_to_user(user_buffer, dev->read_buffer, result))
	{
		//printk(LOG_LEVEL "[serial read] copy_to_user failed %p %p %d", user_buffer, dev->read_buffer, result);
		return -EFAULT;
	}

	dev->read = 0;
	atomic_set(&dev->read_ready, 0);

	// Reactivez intreruperile pentru citire: ce era inainte + RDAI
	outb(inb(IER(dev->base_port)) | IER_RDAI, IER(dev->base_port));

	//printk(LOG_LEVEL "[serial read] returned %d bytes", result);
	return result;
}

static int serial_write(struct file *file, const char *user_buffer,
		size_t size, loff_t *offset)
{
	struct serial_dev *dev = (struct serial_dev*) file->private_data;
	int result;

	//printk(LOG_LEVEL "[serial write] 0x%x writes: %d", dev->base_port, size);
	if (size == 0)
		return 0;

	// Limitez numarul de octeti de citit la dimensiunea FIFO-ului
	if (size >= 14)
		dev->write = 14;
	else
		dev->write = size;

	// Citesc datele din userspace
	result = dev->write;
	if (copy_from_user(dev->write_buffer, user_buffer, result))
	{
		//printk(LOG_LEVEL "[serial write] copy_to_user failed %p %p %d", dev->write_buffer, user_buffer, result);
		return -EFAULT;
	}

	// OK, write is ready
	atomic_set(&dev->write_ready, 1);

	// Reactivez intreruperile pentru scriere: ce era inainte + THREI
	outb(inb(IER(dev->base_port)) | IER_THREI, IER(dev->base_port));

	// Waiting for the write to complete
	wait_event_interruptible(dev->wq_writes, atomic_cmpxchg(&dev->write_ready, 1, 1) == 0);
	//printk(LOG_LEVEL "[serial write] returned %d bytes", result);
	return result;
}

static int serial_ioctl(struct inode *inode, struct file *file,
		unsigned int cmd, unsigned long arg)
{
	struct uart16550_line_info info;
	struct serial_dev *dev = (struct serial_dev*) file->private_data;

	if (cmd == UART16550_IOCTL_SET_LINE)
	{
		if (copy_from_user(&info, (const void *) arg, sizeof(info)))
			return -EFAULT;
		SET_LINE((int) info.baud, info.len, info.stop, info.par, dev->base_port);
		return 0;
	}
	return -EINVAL;
}

struct file_operations serial_fops =
{ .owner = THIS_MODULE, .open = serial_open, .release = serial_close,
		.read = serial_read, .write = serial_write, .ioctl = serial_ioctl, };

static int do_requests(int baseport, int irq, int index, int minor)
{
	int err;

	atomic_set(&uart[index].read_ready, 0);
	atomic_set(&uart[index].write_ready, 0);
	uart[index].read = 0;
	uart[index].write = 0;
	uart[index].base_port = baseport;
	cdev_init(&uart[index].cdev, &serial_fops);
	init_waitqueue_head(&uart[index].wq_reads);
	init_waitqueue_head(&uart[index].wq_writes);

	if ((err = cdev_add(&uart[index].cdev, MKDEV(major, minor), 1)))
	{
		//printk(LOG_LEVEL "cdev_init %d failed", minor);
		return err;
	}

	if (request_region(baseport, COM_PORTS, MODULE_NAME) == NULL)
	{
		//printk(LOG_LEVEL "request_region failed for COM1");
		return -EINVAL;
	}

	err = request_irq(irq, serial_interrupt_handle, IRQF_SHARED, MODULE_NAME,
			&uart[index]);
	if (err)
	{
		//printk(LOG_LEVEL "ERROR: %s: error %d\n", "request_irq", err);
		return err;
	}

	// Enable FIFO cu 14 octeti
	outb(0xc7, FCR(baseport));

	// Enable interrupts
	outb(0x08, MCR(baseport));
	outb(0x01, IER(baseport));

	//printk(LOG_LEVEL "index %d, baseport %d, irq %d, minor %d", index, baseport, irq, minor);

	return 0;
}

//#define INSANE_DEBUG

int serial_cdev_init(void)
{
	int err, sum, minors = 0;

#ifdef INSANE_DEBUG
	minors = 0;
	major = 42;
	option = OPTION_BOTH;
#endif

	switch (option)
	{
	case OPTION_BOTH:
		//printk(LOG_LEVEL "option both");
		info[0] = info[1] = 1;
		minors = 2;
		break;

	case OPTION_COM1:
		//printk(LOG_LEVEL "option COM1");
		info[0] = 1;
		minors = 1;
		break;

	case OPTION_COM2:
		//printk(LOG_LEVEL "option COM2");
		info[1] = 1;
		minors = 1;
		break;
	}

	//printk( LOG_LEVEL "major = %d, option = %d, COM1 = %d, COM2 = %d, minors = %d\n", major, option, info[0], info[1], minors);

	sum = info[0] + info[1];

	// Something strange happened, nothing activated
	if (sum == 0)
	{
		//printk(LOG_LEVEL "Default value behaviour is stub! FIXME");
		return 0;
	}

	if ((err = register_chrdev_region(MKDEV(major, 0), minors, MODULE_NAME)))
		return err;

	if (sum == 1)
	{
		if (info[0])
		{
			if ((err = do_requests(COM1_BASEPORT, COM1_IRQ, 0, 0)))
				goto l_close;
		}
		else
		{
			if ((err = do_requests(COM2_BASEPORT, COM2_IRQ, 1, 0)))
				goto l_close;
		}
	}
	else
	{
		if ((err = do_requests(COM1_BASEPORT, COM1_IRQ, 0, 0)))
			goto l_close;
		if ((err = do_requests(COM2_BASEPORT, COM2_IRQ, 1, 1)))
			goto l_close;
	}

	return 0;
	l_close: unregister_chrdev_region(MKDEV(major, 0), minors);
	return err;
}

void serial_cdev_exit(void)
{
	int minors;

	minors = info[0] + info[1];

	// Eliberez toate resursele
	if (info[0])
	{
		//printk(LOG_LEVEL "cdev_del %d", 0);
		outb(0x0, MCR(COM1_BASEPORT));
		outb(0x0, IER(COM1_BASEPORT));
		free_irq(COM1_IRQ, &uart[0]);
		release_region(COM1_BASEPORT, COM_PORTS);
		cdev_del(&uart[0].cdev);
	}
	if (info[1])
	{
		//printk(LOG_LEVEL "cdev_del %d", 1);
		outb(0x0, MCR(COM2_BASEPORT));
		outb(0x0, IER(COM2_BASEPORT));
		free_irq(COM2_IRQ, &uart[1]);
		release_region(COM2_BASEPORT, COM_PORTS);
		cdev_del(&uart[1].cdev);
	}

	unregister_chrdev_region(MKDEV(major, 0), minors);
}

module_init( serial_cdev_init);
module_exit( serial_cdev_exit);
