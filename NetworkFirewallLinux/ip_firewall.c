/*
 * Laurentiu Dascalu, 342 C3
 * Tema 5 SO2, ipfirewall
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/list.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>

#include "ip_firewall.h"
#include "debug.h"
#include "rules.h"

#define MODULE_NAME    "ipfirewall"

MODULE_DESCRIPTION(MODULE_NAME);
MODULE_AUTHOR("Laurentiu Dascalu");
MODULE_LICENSE("GPL");

#define LOG_LEVEL       KERN_ALERT
#define MODULE_MAJOR            42

LIST_HEAD( static_rules);
LIST_HEAD( tcp_dynamic_rules);
LIST_HEAD( udp_dynamic_rules);

static int ipfirewall_enabled = 0;
spinlock_t lock;

static void new_rule(struct fwr *rule, unsigned int ip_src,
		unsigned int ip_dst, unsigned int ip_src_mask,
		unsigned int ip_dst_mask, unsigned short port_src0,
		unsigned short port_src1, unsigned short port_dst0,
		unsigned short port_dst1)
{
	rule->ip_src = ip_src;
	rule->ip_dst = ip_dst;
	rule->ip_src_mask = ip_src_mask;
	rule->ip_dst_mask = ip_dst_mask;
	rule->port_src[0] = port_src0;
	rule->port_src[1] = port_src1;
	rule->port_dst[0] = port_dst0;
	rule->port_dst[1] = port_dst1;
}

static void generate_tcp_dynamic_rule(struct fwr *rule, struct iphdr *iph,
		struct tcphdr *tcph, int out)
{
	if (out)
		new_rule(rule, iph->daddr, iph->saddr, 0xFFFF, 0xFFFF, tcph->dest,
				tcph->dest, tcph->source, tcph->source);
	else
		new_rule(rule, iph->saddr, iph->daddr, 0, 0, tcph->source,
				tcph->source, tcph->dest, tcph->dest);
}

static void generate_udp_dynamic_rule(struct fwr *rule, struct iphdr *iph,
		struct udphdr *udph, int out)
{
	if (out)
		new_rule(rule, iph->daddr, iph->saddr, 0, 0, udph->dest, udph->dest,
				udph->source, udph->source);
	else
		new_rule(rule, iph->saddr, iph->daddr, 0, 0, udph->source,
				udph->source, udph->dest, udph->dest);
}

static unsigned int my_nf_hookfn_out(unsigned int hooknum, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out, int(*okfn)(
				struct sk_buff *))
{
	struct fwr rule;
	struct udphdr *udph;
	struct tcphdr *tcph;
	struct iphdr *iph;
	struct rules_list_t *ple;

	/* get IP header */
	iph = ip_hdr(skb);

	if (!iph)
		return NF_ACCEPT;

	if (iph->protocol == IPPROTO_TCP)
	{
		/* get TCP header */
		tcph = tcp_hdr(skb);
		generate_tcp_dynamic_rule(&rule, iph, tcph, 1);

		if (tcph->syn && !tcph->ack)
		{
			/* connection initiating packet */
			add_rule(&rule, &tcp_dynamic_rules, 1, iph->protocol);
		}
		else if (tcph->fin)
		{
			/* connection ending packet */
			ple = search_rule(&rule, &tcp_dynamic_rules);
			if (ple)
				mod_timer(&ple->timer, jiffies + TIMEOUT * HZ / 1000);
		}
	}
	else if (iph->protocol == IPPROTO_UDP)
	{
		udph = udp_hdr(skb);
		generate_udp_dynamic_rule(&rule, iph, udph, 1);

		ple = search_rule(&rule, &udp_dynamic_rules);
		/**
		 * Daca regula exista atunci rearmez timerul
		 */
		if (ple)
			mod_timer(&ple->timer, jiffies + TIMEOUT * HZ / 1000);
		else
			add_rule(&rule, &udp_dynamic_rules, 1, iph->protocol);
	}

	return NF_ACCEPT;
}

static unsigned int my_nf_hookfn_in(unsigned int hooknum, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out, int(*okfn)(
				struct sk_buff *))
{
	struct fwr rule;
	struct udphdr *udph;
	struct tcphdr *tcph;
	struct iphdr *iph;
	struct rules_list_t *ple;

	iph = ip_hdr(skb);

	if (!iph)
		return NF_ACCEPT;

	if (iph->protocol == IPPROTO_TCP)
	{
		tcph = (struct tcphdr *) ((__u32 *) iph + iph->ihl);
		generate_tcp_dynamic_rule(&rule, iph, tcph, 0);
	}
	else if (iph->protocol == IPPROTO_UDP)
	{
		udph = (struct udphdr *) ((__u32 *) iph + iph->ihl);
		generate_udp_dynamic_rule(&rule, iph, udph, 0);
	}

	// Caut sa vad daca o regula statica accepta acest pachet
	if (iph->protocol == IPPROTO_TCP || iph->protocol == IPPROTO_UDP)
	{
		if (search_rule(&rule, &static_rules))
			return NF_ACCEPT;
	}

	if (iph->protocol == IPPROTO_TCP)
	{
		ple = search_rule(&rule, &tcp_dynamic_rules);
		if (ple)
			return NF_ACCEPT;
		return NF_DROP;
	}

	if (iph->protocol == IPPROTO_UDP)
	{
		ple = search_rule(&rule, &udp_dynamic_rules);
		if (ple)
		{
			mod_timer(&ple->timer, jiffies + TIMEOUT * HZ / 1000);
			return NF_ACCEPT;
		}
		return NF_DROP;
	}

	return NF_ACCEPT;
}

static int ipfirewall_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int ipfirewall_close(struct inode *inode, struct file *file)
{
	return 0;
}

static struct nf_hook_ops my_nfho_out =
{ .owner = THIS_MODULE, .hook = my_nf_hookfn_out, .hooknum = NF_INET_LOCAL_OUT,
		.pf = PF_INET, .priority = NF_IP_PRI_FIRST };

static struct nf_hook_ops my_nfho_in =
{ .owner = THIS_MODULE, .hook = my_nf_hookfn_in, .hooknum = NF_INET_LOCAL_IN,
		.pf = PF_INET, .priority = NF_IP_PRI_FIRST };

static int ipfirewall_ioctl(struct inode *inode, struct file *file,
		unsigned int cmd, unsigned long arg)
{
	long kernel_size, user_size, s_size, tcp_size, udp_size, aux, size_write;
	void *buffer;
	struct fwr *rule;

	switch (cmd)
	{
	case FW_ADD_RULE:
		LOG("FW_ADD_RULE");
		rule = (struct fwr *) kmalloc(sizeof(struct fwr), GFP_KERNEL);
		if (!rule)
			return -EFAULT;
		if (copy_from_user(rule, (const void *) arg, sizeof(struct fwr)))
			return -EFAULT;
		add_rule(rule, &static_rules, 0, -1);
		break;
	case FW_ENABLE:
		LOG("FW_ENABLE");
		if (!ipfirewall_enabled)
		{
			ipfirewall_enabled = 1;
			if (nf_register_hook(&my_nfho_out))
				return -ENOTTY;
			if (nf_register_hook(&my_nfho_in))
				return -ENOTTY;
		}
		break;
	case FW_DISABLE:
		LOG("FW_DISABLE");
		if (ipfirewall_enabled)
		{
			ipfirewall_enabled = 0;
			nf_unregister_hook(&my_nfho_out);
			nf_unregister_hook(&my_nfho_in);
		}
		break;
	case FW_LIST:
		LOG("FW_LIST");
		spin_lock_bh(&lock);
		s_size = list_size(&static_rules);
		tcp_size = list_size(&tcp_dynamic_rules);
		udp_size = list_size(&udp_dynamic_rules);
		kernel_size = s_size + tcp_size + udp_size;
		user_size = *((int *) arg);

		// copy_to_user se executa in afara spin_lock-ului
		if (kernel_size > user_size)
		{
			spin_unlock_bh(&lock);
			if (copy_to_user((void *) arg, (const void *) &kernel_size,
					sizeof(int)))
				return -EFAULT;
			return -ENOSPC;
		}

		/*
		 * Creez un buffer in care pun regulile serializate si apoi
		 * transfer buffer-ul in userspace
		 */
		buffer = (struct fwr *) kmalloc(kernel_size * sizeof(struct fwr),
				GFP_ATOMIC);
		if (!buffer)
		{
			spin_unlock_bh(&lock);
			return -ENOMEM;
		}
		add_rules_to_buffer(buffer, &static_rules);
		add_rules_to_buffer(buffer + s_size * sizeof(struct fwr),
				&tcp_dynamic_rules);
		add_rules_to_buffer(buffer + (tcp_size + s_size) * sizeof(struct fwr),
				&udp_dynamic_rules);

		size_write = kernel_size * sizeof(struct fwr);
		aux = size_write;
		spin_unlock_bh(&lock);
		while (1)
		{
			aux = copy_to_user((void *) arg + size_write - aux,
					(const void *) (buffer + size_write - aux), aux);
			if (!aux)
				break;
		}

		kfree(buffer);
		return kernel_size;
	default:
		return -ENOTTY;
	}
	return 0;
}

struct file_operations ipfirewall_fops =
{ .owner = THIS_MODULE, .open = ipfirewall_open, .release = ipfirewall_close,
		.ioctl = ipfirewall_ioctl, };

static struct cdev ipfirewall_cdev;

int ipfirewall_init(void)
{
	int err;

	if ((err = register_chrdev_region(MKDEV(MODULE_MAJOR, 0), 1, MODULE_NAME)))
		goto end;

	spin_lock_init(&lock);
	cdev_init(&ipfirewall_cdev, &ipfirewall_fops);

	if ((err = cdev_add(&ipfirewall_cdev, MKDEV(MODULE_MAJOR, 0), 1)))
		goto cdev_add_failed;

	if ((err = nf_register_hook(&my_nfho_out)))
		goto hook_failed;

	if ((err = nf_register_hook(&my_nfho_in)))
		goto hook_failed;

	ipfirewall_enabled = 1;
	return 0;

	hook_failed: cdev_del(&ipfirewall_cdev);
	cdev_add_failed: unregister_chrdev_region(MKDEV(MODULE_MAJOR, 0), 1);
	end: return err;
}

void ipfirewall_exit(void)
{
	// Distrug listele de reguli
	destroy_list(&static_rules);
	destroy_list(&tcp_dynamic_rules);
	destroy_list(&udp_dynamic_rules);

	// Elimin hook-urile
	if (ipfirewall_enabled)
	{
		nf_unregister_hook(&my_nfho_out);
		nf_unregister_hook(&my_nfho_in);
	}

	cdev_del(&ipfirewall_cdev);
	unregister_chrdev_region(MKDEV(MODULE_MAJOR, 0), 1);
}

module_init( ipfirewall_init);
module_exit( ipfirewall_exit);
