#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/skbuff.h>

#include "rules.h"

extern spinlock_t lock;
extern struct list_head tcp_dynamic_rules;
extern struct list_head udp_dynamic_rules;

/**
 *   In functiile de callback ale timereului pur si simplu
 * sterg regula din lista de reguli pentru TCP/UDP
 */
static void udp_timer_callback(unsigned long data)
{
	del_rule((struct fwr *) data, &udp_dynamic_rules);
}

static void tcp_timer_callback(unsigned long data)
{
	del_rule((struct fwr *) data, &tcp_dynamic_rules);
}

static void null_timer_callback(unsigned long data)
{

}

int add_rule(struct fwr *rule, struct list_head *head, int on_stack, int proto)
{
	struct rules_list_t *ple;

	spin_lock_bh(&lock);

	ple = (struct rules_list_t*) kmalloc(sizeof(struct rules_list_t),
			GFP_KERNEL);
	if (!ple)
	{
		spin_unlock_bh(&lock);
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&ple->list);

	if (on_stack)
	{
		/* Copiez datele din structura regulii pentru ca e alocata pe stiva de compilator */
		ple->rule = (struct fwr *) kmalloc(sizeof(struct fwr), GFP_KERNEL);
		if (!ple->rule)
		{
			spin_unlock_bh(&lock);
			return -ENOMEM;
		}
		memcpy(ple->rule, rule, sizeof(struct fwr));
	}
	else
		ple->rule = rule;

	// Regula nu are nevoie de un timer (regula statica)
	if (proto == -1)
		setup_timer(&ple->timer, null_timer_callback,
				(unsigned long) &ple->rule);
	// Adaug un timer; nu-l armez!
	else if (proto == IPPROTO_TCP)
		setup_timer(&ple->timer, tcp_timer_callback, (unsigned long) &ple->rule);
	else if (proto == IPPROTO_UDP)
	{
		// Adaug timer-ul si-l armez
		setup_timer(&ple->timer, udp_timer_callback, (unsigned long) &ple->rule);
		mod_timer(&ple->timer, jiffies + TIMEOUT * HZ / 1000);
	}

	list_add(&ple->list, head);

	spin_unlock_bh(&lock);

	return 0;
}

int del_rule(struct fwr *rule, struct list_head *head)
{
	struct list_head *i, *n;
	struct rules_list_t *ple;

	spin_lock(&lock);

	list_for_each_safe(i, n, head)
	{
		ple = list_entry(i, struct rules_list_t, list);

		if (!memcmp(&ple->rule, rule, sizeof(struct fwr)))
		{
			del_timer(&ple->timer);
			list_del(i);
			kfree(ple);
			spin_unlock(&lock);
			return 0;
		}
	}

	spin_unlock(&lock);
	return -EINVAL;
}

static int match_rule(struct fwr *src, struct fwr *dest)
{
	// Verific daca adresa obtinuta dupa aplicarea mastii e aceeasi,
	// si daca portul se incadreaza in intervalul inchis dat de regula
	if ((src->ip_src & dest->ip_src_mask) == (dest->ip_src & dest->ip_src_mask)
			&& (src->ip_dst & dest->ip_dst_mask) == (dest->ip_dst
					& dest->ip_dst_mask) && src->port_src[0]
			>= dest->port_src[0] && src->port_src[1] <= dest->port_src[1]
			&& src->port_dst[0] >= dest->port_dst[0] && src->port_dst[1]
			<= dest->port_dst[1])
		return 1;
	return 0;
}

struct rules_list_t *search_rule(struct fwr *rule, struct list_head *head)
{
	struct list_head *i, *n;
	struct rules_list_t *ple;

	spin_lock(&lock);

	list_for_each_safe(i, n, head)
	{
		ple = list_entry(i, struct rules_list_t, list);

		if (match_rule(rule, ple->rule))
		{
			spin_unlock(&lock);
			return ple;
		}
	}

	spin_unlock(&lock);
	return NULL;
}

void destroy_list(struct list_head *head)
{
	struct list_head *i, *n;
	struct rules_list_t *ple;

	spin_lock_bh(&lock);

	/**
	 * Nu mai iau spinlockul aici pentru ca il ia ioctl-ul
	 * care ma apeleaza
	 */
	list_for_each_safe(i, n, head)
	{
		ple = list_entry(i, struct rules_list_t, list);
		del_timer(&ple->timer);
		list_del(i);
		kfree(ple);
	}

	spin_unlock_bh(&lock);
}

int list_size(struct list_head *head)
{
	struct list_head *i, *n;
	int size = 0;

	list_for_each_safe(i, n, head)
	{
		size++;
	}

	return size;
}

void add_rules_to_buffer(void *buffer, struct list_head *head)
{
	int j = 0;
	struct list_head *i, *n;
	struct rules_list_t *ple;

	/**
	 * Nu mai iau spinlockul aici pentru ca il ia ioctl-ul
	 * care ma apeleaza
	 */
	list_for_each_safe(i, n, head)
	{
		ple = list_entry(i, struct rules_list_t, list);
		memcpy(buffer + j * sizeof(struct fwr), ple->rule, sizeof(struct fwr));
		j++;
	}
}
