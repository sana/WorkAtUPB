#ifndef RULES_H_
#define RULES_H_

#include <ntddk.h>
#include "ip_firewall.h"
#define TIMEOUT                300

struct rules_list_t
{
	struct fwr *rule;
	KTIMER timer;
	KDPC dpc;
	SINGLE_LIST_ENTRY list;
};

NTSTATUS add_rule(struct fwr *rule, SINGLE_LIST_ENTRY *head, int on_stack,
		int proto);
NTSTATUS del_rule(struct fwr *rule, SINGLE_LIST_ENTRY *head);

struct rules_list_t *search_rule(struct fwr *rule, SINGLE_LIST_ENTRY *head);

void destroy_list(SINGLE_LIST_ENTRY *head);

/** Functii folosite de FW_LIST */
int list_size(SINGLE_LIST_ENTRY *head);
void add_rules_to_buffer(void *buffer, SINGLE_LIST_ENTRY *head);

#endif
