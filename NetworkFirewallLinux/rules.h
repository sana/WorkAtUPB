#ifndef RULES_H_
#define RULES_H_

#include <linux/list.h>
#include <linux/ip.h>

#include "ip_firewall.h"
#define TIMEOUT                300

struct rules_list_t
{
	struct fwr *rule;
	struct timer_list timer;
	struct list_head list;
};

int add_rule(struct fwr *rule, struct list_head *head, int on_stack, int proto);
int del_rule(struct fwr *rule, struct list_head *head);

struct rules_list_t *search_rule(struct fwr *rule, struct list_head *head);

void destroy_list(struct list_head *head);

/** Functii folosite de FW_LIST */
int list_size(struct list_head *head);
void add_rules_to_buffer(void *buffer, struct list_head *head);

#endif
