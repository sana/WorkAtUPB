#include "rules.h"
#include "NetHeaders.h"

extern KSPIN_LOCK lock;
extern KIRQL irql;
extern SINGLE_LIST_ENTRY tcp_dynamic_rules;
extern SINGLE_LIST_ENTRY udp_dynamic_rules;

/**
 *   In functiile de callback ale timereului pur si simplu
 * sterg regula din lista de reguli pentru TCP/UDP
 */
static void udp_timer_callback(KDPC* dpc, PVOID context, PVOID arg1, PVOID arg2)
{
	del_rule((struct fwr *) context, &udp_dynamic_rules);
}

static void tcp_timer_callback(KDPC* dpc, PVOID context, PVOID arg1, PVOID arg2)
{
	del_rule((struct fwr *) context, &tcp_dynamic_rules);
}

static void null_timer_callback(KDPC* dpc, PVOID context, PVOID arg1,
		PVOID arg2)
{

}

NTSTATUS add_rule(struct fwr *rule, SINGLE_LIST_ENTRY *head, int on_stack,
		int proto)
{
	struct rules_list_t *ple;
	LARGE_INTEGER systemDelay;

	KeAcquireSpinLock(&lock, &irql);

	ple = (struct rules_list_t*) ExAllocatePoolWithTag(NonPagedPool,
			sizeof(struct rules_list_t), '1gat');
	if (!ple)
	{
		KeReleaseSpinLock(&lock, irql);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	ple->list.Next = NULL;

	if (on_stack)
	{
		/* Copiez datele din structura regulii pentru ca e alocata pe stiva de compilator */
		ple->rule = (struct fwr *) ExAllocatePoolWithTag(NonPagedPool,
				sizeof(struct fwr), '1gat');
		if (!ple->rule)
		{
			KeReleaseSpinLock(&lock, irql);
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		memcpy(ple->rule, rule, sizeof(struct fwr));
	}
	else
		ple->rule = rule;

	// Regula nu are nevoie de un timer (regula statica)
	if (proto == -1)
	{
		KeInitializeTimer(&ple->timer);
		KeInitializeDpc(&ple->dpc, null_timer_callback, (PVOID) & ple->rule);
	}
	// Adaug un timer; nu-l armez!
	else if (proto == IPPROTO_TCP)
	{
		KeInitializeTimer(&ple->timer);
		KeInitializeDpc(&ple->dpc, tcp_timer_callback, (PVOID) & ple->rule);
	}
	else if (proto == IPPROTO_UDP)
	{
		// Adaug timer-ul si-l armez
		KeInitializeTimer(&ple->timer);
		KeInitializeDpc(&ple->dpc, udp_timer_callback, (PVOID) & ple->rule);
		systemDelay.QuadPart = -TIMEOUT * 1000 * 10;
		KeSetTimer(&ple->timer, systemDelay, &ple->dpc);
	}

	PushEntryList(head, &ple->list);
	KeReleaseSpinLock(&lock, irql);

	return 0;
}

NTSTATUS del_rule(struct fwr *rule, SINGLE_LIST_ENTRY *head)
{
	SINGLE_LIST_ENTRY *i, *j;
	struct rules_list_t *ple;

	KeAcquireSpinLock(&lock, &irql);

	for (j = head, i = head->Next; i; j = i, i = i->Next)
	{
		ple = CONTAINING_RECORD(i, struct rules_list_t, list);

		if (!memcmp(&ple->rule, rule, sizeof(struct fwr)))
		{
			KeCancelTimer(&ple->timer);
			PopEntryList(j);
			ExFreePoolWithTag(ple, '1gat');
			KeReleaseSpinLock(&lock, irql);
			return STATUS_SUCCESS;
		}
	}

	KeReleaseSpinLock(&lock, irql);
	return STATUS_INSUFFICIENT_RESOURCES;
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

struct rules_list_t *search_rule(struct fwr *rule, SINGLE_LIST_ENTRY *head)
{
	SINGLE_LIST_ENTRY *i, *j;
	struct rules_list_t *ple;

	KeAcquireSpinLock(&lock, &irql);

	for (j = head, i = head->Next; i; j = i, i = i->Next)
	{
		ple = CONTAINING_RECORD(i, struct rules_list_t, list);

		if (match_rule(rule, ple->rule))
		{
			KeReleaseSpinLock(&lock, irql);
			return ple;
		}
	}

	KeReleaseSpinLock(&lock, irql);
	return NULL;
}

void destroy_list(SINGLE_LIST_ENTRY *head)
{
	SINGLE_LIST_ENTRY *i, *j;
	struct rules_list_t *ple;

	KeAcquireSpinLock(&lock, &irql);

	for (j = head, i = head->Next; i; j = i, i = i->Next)
	{
		ple = CONTAINING_RECORD(i, struct rules_list_t, list);
		KeCancelTimer(&ple->timer);
		PopEntryList(j);
		ExFreePoolWithTag(ple, '1gat');
	}

	KeReleaseSpinLock(&lock, irql);
}

int list_size(SINGLE_LIST_ENTRY *head)
{
	SINGLE_LIST_ENTRY *i, *j;
	int size = 0;

	for (j = head, i = head->Next; i; j = i, i = i->Next)
	{
		size++;
	}

	return size;
}

void add_rules_to_buffer(void *buffer, SINGLE_LIST_ENTRY *head)
{
	int k = 0;
	SINGLE_LIST_ENTRY *i, *j;
	struct rules_list_t *ple;

	for (j = head, i = head->Next; i; j = i, i = i->Next)
	{
		ple = CONTAINING_RECORD(i, struct rules_list_t, list);
		memcpy((char *) buffer + k * sizeof(struct fwr), ple->rule,
				sizeof(struct fwr));
		k++;
	}
}
