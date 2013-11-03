/*
 * Laurentiu Dascalu, 342 C3
 * Tema 5 SO2, ipfirewall
 */
#include <ndis.h>
#include <pfhook.h>
#include "ip_firewall.h"
#include "debug.h"
#include "rules.h"
#include "NetHeaders.h"

#define MODULE_NAME    "ipfirewall"
#define FIREWALL_PATH_KERNEL        "\\Device\\ipfirewall"
#define FIREWALL_PATH_LINK          "\\??\\ipfirewall"

SINGLE_LIST_ENTRY static_rules =
{ NULL };
SINGLE_LIST_ENTRY tcp_dynamic_rules =
{ NULL };
SINGLE_LIST_ENTRY udp_dynamic_rules =
{ NULL };

static volatile int ipfirewall_enabled = 0;
KSPIN_LOCK lock;
KIRQL irql;

static UNICODE_STRING *TO_UNICODE(const CHAR *str, UNICODE_STRING *unicodeStr)
{
	ANSI_STRING ansiStr;

	RtlInitAnsiString(&ansiStr, str);
	if (RtlAnsiStringToUnicodeString(unicodeStr, &ansiStr, TRUE)
			!= STATUS_SUCCESS)
		return NULL;

	return unicodeStr;
}

static void new_rule(struct fwr *rule, ULONG ip_src, ULONG ip_dst,
		unsigned int ip_src_mask, unsigned int ip_dst_mask, USHORT port_src0,
		USHORT port_src1, USHORT port_dst0, USHORT port_dst1)
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

static void generate_tcp_dynamic_rule(struct fwr *rule, IPHeader *iph,
		TCPHeader *tcph, int out)
{
	if (out)
		new_rule(rule, iph->destination, iph->source, 0xFFFF, 0xFFFF,
				tcph->destinationPort, tcph->destinationPort, tcph->sourcePort,
				tcph->sourcePort);
	else
		new_rule(rule, iph->source, iph->destination, 0, 0, tcph->sourcePort,
				tcph->sourcePort, tcph->destinationPort, tcph->destinationPort);
}

static void generate_udp_dynamic_rule(struct fwr *rule, IPHeader *iph,
		UDPHeader *udph, int out)
{
	if (out)
		new_rule(rule, iph->destination, iph->source, 0, 0,
				udph->destinationPort, udph->destinationPort, udph->sourcePort,
				udph->sourcePort);
	else
		new_rule(rule, iph->source, iph->destination, 0, 0, udph->sourcePort,
				udph->sourcePort, udph->destinationPort, udph->destinationPort);
}

static FORWARD_ACTION FilterPacket(unsigned char *PacketHeader,
		unsigned char *Packet, unsigned int PacketLength,
		DIRECTION_E direction, unsigned int RecvInterfaceIndex,
		unsigned int SendInterfaceIndex)
{
	IPHeader *iph;
	TCPHeader *tcph;
	UDPHeader *udph;
	struct fwr rule;
	struct rules_list_t *ple;
	LARGE_INTEGER systemDelay;

	//IP header
	iph = (IPHeader *) PacketHeader;
	if (!iph)
		return FORWARD;

	if (RecvInterfaceIndex > 0)
	{
		if (iph->protocol == IPPROTO_TCP)
		{
			//TCP header
			tcph = (TCPHeader *) Packet;
			generate_tcp_dynamic_rule(&rule, iph, tcph, 0);
		}
		else if (iph->protocol == IPPROTO_UDP)
		{
			udph = (UDPHeader *) Packet;
			generate_udp_dynamic_rule(&rule, iph, udph, 0);
		}

		// Caut sa vad daca o regula statica accepta acest pachet
		if (iph->protocol == IPPROTO_TCP || iph->protocol == IPPROTO_UDP)
		{
			if (search_rule(&rule, &static_rules))
				return FORWARD;
		}

		if (iph->protocol == IPPROTO_TCP)
		{
			ple = search_rule(&rule, &tcp_dynamic_rules);
			if (ple)
				return FORWARD;
			return DROP;
		}

		if (iph->protocol == IPPROTO_UDP)
		{
			ple = search_rule(&rule, &udp_dynamic_rules);
			if (ple)
			{
				systemDelay.QuadPart = -TIMEOUT * 1000 * 10;
				KeSetTimer(&ple->timer, systemDelay, &ple->dpc);
				return FORWARD;
			}
			return DROP;
		}
	}
	if (SendInterfaceIndex > 0)
	{
		if (iph->protocol == IPPROTO_TCP)
		{
			LOG("[Filter SendInterfaceIndex] am primit un pachet TCP");
			// get TCP header
			tcph = (TCPHeader *) Packet;
			generate_tcp_dynamic_rule(&rule, iph, tcph, 1);

			if (tcph->syn && !tcph->ack)
			{
				// connection initiating packet
				LOG("[Filter SendInterfaceIndex] accept o conexiune TCP");
				add_rule(&rule, &tcp_dynamic_rules, 1, iph->protocol);
			}
			else if (tcph->fin)
			{
				// connection ending packet
				ple = search_rule(&rule, &tcp_dynamic_rules);
				if (ple)
				{
					systemDelay.QuadPart = -TIMEOUT * 1000 * 10;
					KeSetTimer(&ple->timer, systemDelay, &ple->dpc);
				}
			}
		}
		else if (iph->protocol == IPPROTO_UDP)
		{
			LOG("[Filter SendInterfaceIndex] am primit un pachet UDP");
			udph = (UDPHeader *) Packet;
			generate_udp_dynamic_rule(&rule, iph, udph, 1);

			ple = search_rule(&rule, &udp_dynamic_rules);
			// Daca regula exista atunci rearmez timerul
			if (ple)
			{
				systemDelay.QuadPart = -TIMEOUT * 1000 * 10;
				KeSetTimer(&ple->timer, systemDelay, &ple->dpc);
			}
			else
			{
				LOG("[Filter SendInterfaceIndex] adaug o noua regula UDP");
				add_rule(&rule, &udp_dynamic_rules, 1, iph->protocol);
			}
		}
	}

	return FORWARD;
}

static FORWARD_ACTION callbackFilterFunction(VOID **pData,
		UINT RecvInterfaceIndex, UINT *pSendInterfaceIndex,
		UCHAR *pDestinationType, VOID *pContext, UINT ContextLength,
		struct IPRcvBuf **pRcvBuf)
{
	FORWARD_ACTION result = FORWARD;
	char *packet = NULL;
	int bufferSize;
	struct IPRcvBuf *buffer = (struct IPRcvBuf *) *pData;

	PFIREWALL_CONTEXT_T fwContext = (PFIREWALL_CONTEXT_T) pContext;

	unsigned int offset = 0;
	IPHeader *iph;

	if (buffer == NULL)
	{
		LOG("buffer has no data\n");
		return result;
	}

	bufferSize = buffer->ipr_size;
	while (buffer->ipr_next != NULL)
	{
		buffer = buffer->ipr_next;
		bufferSize += buffer->ipr_size;
	}

	packet = (char *) ExAllocatePoolWithTag(NonPagedPool, bufferSize, 'p');
	if (packet == NULL)
	{
		LOG("ipfirewall.sys: out of memory.\n");
		goto err_out;
	}

	iph = (IPHeader *) packet;

	buffer = (struct IPRcvBuf *) *pData;
	RtlCopyMemory(packet, buffer->ipr_buffer, buffer->ipr_size);
	while (buffer->ipr_next != NULL)
	{
		offset += buffer->ipr_size;

		buffer = buffer->ipr_next;
		RtlCopyMemory(packet + offset, buffer->ipr_buffer, buffer->ipr_size);
	}

	/* call true filtering function */
	result = FilterPacket(packet, packet + (iph->headerLength * 4), bufferSize
			- (iph->headerLength * 4),
			(fwContext != NULL) ? fwContext->Direction : 0, RecvInterfaceIndex,
			(pSendInterfaceIndex != NULL) ? *pSendInterfaceIndex : 0);

	if (packet != NULL)
		ExFreePoolWithTag(packet, 'p');

	return result;

	err_out: return result;
}

static NTSTATUS SetCallback(IPPacketFirewallPtr filterFunction, BOOLEAN load)
{
	NTSTATUS status = STATUS_SUCCESS;
	NTSTATUS waitStatus = STATUS_SUCCESS;
	UNICODE_STRING filterName;
	PDEVICE_OBJECT ipDeviceObject = NULL;
	PFILE_OBJECT ipFileObject = NULL;

	IP_SET_FIREWALL_HOOK_INFO filterData;

	KEVENT event;
	IO_STATUS_BLOCK ioStatus;
	PIRP irp;

	RtlInitUnicodeString(&filterName, DD_IP_DEVICE_NAME);
	status = IoGetDeviceObjectPointer(&filterName, STANDARD_RIGHTS_ALL,
			&ipFileObject, &ipDeviceObject);

	if (NT_SUCCESS(status))
	{
		filterData.FirewallPtr = filterFunction;
		filterData.Priority = 1;
		filterData.Add = load;

		KeInitializeEvent(&event, NotificationEvent, FALSE);

		irp = IoBuildDeviceIoControlRequest(IOCTL_IP_SET_FIREWALL_HOOK,
				ipDeviceObject, (PVOID) & filterData,
				sizeof(IP_SET_FIREWALL_HOOK_INFO), NULL, 0, FALSE, &event,
				&ioStatus);
		if (irp != NULL)
		{
			status = IoCallDriver(ipDeviceObject, irp);
			if (status == STATUS_PENDING)
			{
				waitStatus = KeWaitForSingleObject(&event, Executive,
						KernelMode, FALSE, NULL);
				if (waitStatus != STATUS_SUCCESS)
					LOG("error waiting for IP device.\n");
			}

			status = ioStatus.Status;
			if (!NT_SUCCESS(status))
				LOG("IP device status error.\n");
		}
		else
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			LOG("filter IRP creation failed.\n");
		}

		if (ipFileObject != NULL)
			ObDereferenceObject(ipFileObject);

		ipFileObject = NULL;
		ipDeviceObject = NULL;
	}
	else
		LOG("error getting object pointer to IP device.\n");

	return status;
}

static NTSTATUS RegisterCallback(IPPacketFirewallPtr filterFunction)
{
	return SetCallback(filterFunction, 1);
}

static NTSTATUS UnregisterCallback(IPPacketFirewallPtr filterFunction)
{
	return SetCallback(filterFunction, 0);
}

static NTSTATUS ipfirewall_open(PDEVICE_OBJECT device, IRP *irp)
{
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

static NTSTATUS ipfirewall_cleanup(PDEVICE_OBJECT device, IRP *irp)
{
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

static NTSTATUS ipfirewall_close(PDEVICE_OBJECT device, IRP *irp)
{
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

static NTSTATUS ipfirewall_ioctl(PDEVICE_OBJECT device, IRP *irp)
{
	ULONG controlCode;
	PIO_STACK_LOCATION pIrpStack;
	NTSTATUS status = STATUS_SUCCESS;
	int information = 0;
	void *buffer, *kernel_buffer;
	struct fwr *rule;
	long kernel_size, user_size, s_size, tcp_size, udp_size, aux, size_write;

	/* get control code from stack location and buffer from IRP */
	pIrpStack = IoGetCurrentIrpStackLocation(irp);
	controlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
	buffer = irp->AssociatedIrp.SystemBuffer;

	switch (controlCode)
	{
	case FW_ADD_RULE:
		LOG("FW_ADD_RULE");

		rule = (struct fwr *) ExAllocatePoolWithTag(NonPagedPool,
				sizeof(struct fwr), '1gat');
		if (!rule)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			goto end;
		}

		RtlCopyMemory(rule, buffer, sizeof(struct fwr));
		add_rule(rule, &static_rules, 0, -1);
		break;
	case FW_ENABLE:
		LOG("FW_ENABLE");
		if (!ipfirewall_enabled)
		{
			ipfirewall_enabled = 1;
			if (RegisterCallback(callbackFilterFunction) != STATUS_SUCCESS)
				DbgPrint("[FW_ENABLE] failed to register callback");
		}
		break;
	case FW_DISABLE:
		LOG("FW_DISABLE");
		if (ipfirewall_enabled)
		{
			ipfirewall_enabled = 0;
			if (UnregisterCallback(callbackFilterFunction) != STATUS_SUCCESS)
				DbgPrint("[FW_ENABLE] failed to unregister callback");
		}
		break;
	case FW_LIST:
		LOG("FW_LIST");

		//spin_lock_bh(&lock);
		s_size = list_size(&static_rules);
		tcp_size = list_size(&tcp_dynamic_rules);
		udp_size = list_size(&udp_dynamic_rules);
		DbgPrint("[FW_LIST] static %d, TCP %d, UDP %d", s_size, tcp_size,
				udp_size);
		kernel_size = s_size + tcp_size + udp_size;
		RtlCopyMemory(&user_size, buffer, sizeof(int));

		DbgPrint("[FW_LIST] kernel size %d, user size %d", kernel_size,
				user_size);

		if (kernel_size <= 0 && user_size <= 0)
		{
			kernel_size = 0;
			RtlCopyMemory((void *) buffer, (const void *) &kernel_size,
					sizeof(int));
			information = sizeof(int);
			goto end;
		}
		// copy_to_user se executa in afara spin_lock-ului
		if (kernel_size > user_size)
		{
			//spin_unlock_bh(&lock);
			RtlCopyMemory((void *) buffer, (const void *) &kernel_size,
					sizeof(int));
			information = sizeof(int);
			goto end;
		}

		// Creez un buffer in care pun regulile serializate si apoi
		// transfer buffer-ul in userspace
		kernel_buffer = (struct fwr *) ExAllocatePoolWithTag(NonPagedPool,
				kernel_size * sizeof(struct fwr) + sizeof(int), '1gat');
		if (!kernel_buffer)
		{
			//spin_unlock_bh(&lock);
			status = STATUS_INSUFFICIENT_RESOURCES;
			goto end;
		}

		*(int *) kernel_buffer = kernel_size;
		(void) add_rules_to_buffer((char *) kernel_buffer + sizeof(int),
				&static_rules);
		(void) add_rules_to_buffer((char *) kernel_buffer + s_size
				* sizeof(struct fwr) + sizeof(int), &tcp_dynamic_rules);
		(void) add_rules_to_buffer((char *) kernel_buffer + (tcp_size + s_size)
				* sizeof(struct fwr) + sizeof(int), &udp_dynamic_rules);

		LOG("[FW_LIST] am copiat datele intr-un buffer kernel");

		size_write = kernel_size * sizeof(struct fwr) + sizeof(int);
		information = size_write;
		//spin_unlock_bh(&lock);
		RtlCopyMemory((void *) buffer, (const void *) kernel_buffer, size_write);
		ExFreePoolWithTag(kernel_buffer, '1gat');
		break;
	default:
		status = STATUS_ILLEGAL_FUNCTION;
		information = 0;
	}

	end: irp->IoStatus.Status = status;
	irp->IoStatus.Information = information;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

void DriverUnload(PDRIVER_OBJECT driver);

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registry)
{
	NTSTATUS err = STATUS_SUCCESS;
	UNICODE_STRING devUnicodeName, linkUnicodeName;
	DEVICE_OBJECT *device;

	RtlZeroMemory(&devUnicodeName, sizeof(devUnicodeName));
	RtlZeroMemory(&linkUnicodeName, sizeof(linkUnicodeName));

	err = IoCreateDevice(driver, 0, TO_UNICODE(FIREWALL_PATH_KERNEL,
			&devUnicodeName), FILE_DEVICE_UNKNOWN, 0, FALSE, &device);
	if (err != STATUS_SUCCESS)
		goto error;
	LOG("[DriverEntry] IoCreateDevice");

	err = IoCreateSymbolicLink(
			TO_UNICODE(FIREWALL_PATH_LINK, &linkUnicodeName), &devUnicodeName);
	if (err != STATUS_SUCCESS)
	{
		LOG("[DriverEntry] IoCreateSymbolicLink failed!");
		goto error;
	}
	LOG("[DriverEntry] IoCreateSymbolicLink");

	if (devUnicodeName.Buffer)
		RtlFreeUnicodeString(&devUnicodeName);
	if (linkUnicodeName.Buffer)
		RtlFreeUnicodeString(&linkUnicodeName);

	device->Flags |= DO_BUFFERED_IO;
	driver->MajorFunction[IRP_MJ_CREATE] = ipfirewall_open;
	driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ipfirewall_ioctl;
	driver->MajorFunction[IRP_MJ_CLEANUP] = ipfirewall_cleanup;
	driver->MajorFunction[IRP_MJ_CLOSE] = ipfirewall_close;
	driver->DriverUnload = DriverUnload;

	ipfirewall_enabled = 1;

	/* register callback function */
	LOG("[DriverEntry] Adaug hook-ul");
	if (RegisterCallback(callbackFilterFunction) != STATUS_SUCCESS)
		DbgPrint("[DriverUnload] failed to register callback");
	;

	err = STATUS_SUCCESS;
	goto end;

	error: if (devUnicodeName.Buffer)
		RtlFreeUnicodeString(&devUnicodeName);
	if (linkUnicodeName.Buffer)
		RtlFreeUnicodeString(&linkUnicodeName);
	DriverUnload(driver);
	end: return err;
}

void DriverUnload(PDRIVER_OBJECT driver)
{
	DEVICE_OBJECT *device;
	UNICODE_STRING linkUnicodeName;

	LOG("[DriverUnload] Distrug listele de reguli");
	destroy_list(&static_rules);
	destroy_list(&tcp_dynamic_rules);
	destroy_list(&udp_dynamic_rules);

	if (ipfirewall_enabled)
	{
		LOG("[DriverUnload] Elimin hook-ul");
		if (UnregisterCallback(callbackFilterFunction) != STATUS_SUCCESS)
			DbgPrint("[DriverUnload] failed to unregister callback");
		ipfirewall_enabled = 0;
	}

	if (TO_UNICODE(FIREWALL_PATH_LINK, &linkUnicodeName))
	{
		LOG("[DriverUnload] IoDeleteSymbolicLink");
		IoDeleteSymbolicLink(&linkUnicodeName);
	}
	if (linkUnicodeName.Buffer)
		RtlFreeUnicodeString(&linkUnicodeName);

	while ((device = driver->DeviceObject))
	{
		LOG("[DriverUnload] IoDeleteDevice");
		IoDeleteDevice(device);
	}
}
