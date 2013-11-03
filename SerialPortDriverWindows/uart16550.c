// Laurentiu Dascalu, 342C3
// Tema 2 SO2, driver pentru interfata seriala
#include <ntddk.h>
#include "uart16550.h"

/* Disable deprecated warning for HalGetInterruptVector */
#pragma warning(disable:4996)

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

// Optiuni COM1
#define OPTION_COM1               1
#define COM1_PATH_KERNEL          "\\Device\\uart0"
#define COM1_PATH_LINK            "\\??\\uart0"
#define COM1_BASEPORT             0x3F8
#define COM1_IRQ                  4

// Optiuni COM2
#define OPTION_COM2               2
#define COM2_PATH_KERNEL          "\\Device\\uart1"
#define COM2_PATH_LINK            "\\??\\uart1"
#define COM2_BASEPORT             0x2F8
#define COM2_IRQ                  3

int option = -1, info[2] =
{ 0, 0 };

#define IER(baseport)             ((baseport) + 1)
#define IIR(baseport)             ((baseport) + 2)
#define FCR(baseport)             ((baseport) + 2)
#define MCR(baseport)             ((baseport) + 4)
#define LSR(baseport)             ((baseport) + 5)
#define IIR_RDAI(X)               (((X) >> 2) & 0x1)
#define IIR_THREI(X)	          (((X) >> 1) & 0x1)

// Dezactivez intreruperile pentru citire
#define IER_RDAI	              0x01

// Dezactivez intreruperile pentru scriere
#define IER_THREI	              0x02

#define COM_PORTS                 8
#define SERIAL_MAX_MINORS         2
#define MODULE_NAME               "uart16550"
#define BUFFER_SIZE               PAGE_SIZE
#define SIZE                      512

#define READ_OP                   98
#define WRITE_OP                  99

// Macro dintr-un laborator
#define SET_LINE(baud, len, stop, par, base)\
do\
{\
	WRITE_PORT_UCHAR((PUCHAR)((base) + 3), (UCHAR) 0x80); \
	WRITE_PORT_UCHAR((PUCHAR)((base))    , (UCHAR) ((baud)&0x00FF)); \
	WRITE_PORT_UCHAR((PUCHAR)((base) + 1), (UCHAR) ((baud)&0xFF00)); \
	WRITE_PORT_UCHAR((PUCHAR)((base) + 3), (UCHAR) 0x0); \
	WRITE_PORT_UCHAR((PUCHAR)((base) + 3), (UCHAR) ((len) | (stop) | (par)));\
} while (0)

typedef struct _UART_DEVICE_DATA
{
	PDEVICE_OBJECT DeviceObject;
	PKINTERRUPT interruptObj;
	char read_buffer[SIZE], write_buffer[SIZE];
	int base_port;
	ULONG bufferSize, read, write;
	KEVENT event_reads, event_writes;
} UART_DEVICE_DATA, *PUART_DEVICE_DATA;

RTL_QUERY_REGISTRY_TABLE table[2];

// Cam hackish sa duplic codul, dar nu am gasit o varianta mai simpla
// care sa nu reduca din generalitate
BOOLEAN ReadUartSyncRoutine(PVOID Context)
{
	UART_DEVICE_DATA *data = (UART_DEVICE_DATA *) Context;
	KeSetEvent(&data->event_reads, IO_NO_INCREMENT, FALSE);
	return TRUE;
}

BOOLEAN WriteUartSyncRoutine(PVOID Context)
{
	UART_DEVICE_DATA *data = (UART_DEVICE_DATA *) Context;
	KeSetEvent(&data->event_writes, IO_NO_INCREMENT, FALSE);
	return TRUE;
}

static NTSTATUS loadParameters(void)
{
	WCHAR path[100] = L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\hello\\Parameters";
	char message[20] = "def";
	WCHAR keyname[20] = L"parametru";
	CHAR def[20] = "";
	WCHAR buffer[100];
	NTSTATUS status;
	UNICODE_STRING answer;

	RtlZeroMemory(&info, sizeof(info));
	RtlZeroMemory(&table, 2 * sizeof(RTL_QUERY_REGISTRY_TABLE));

	table[0].QueryRoutine = NULL;
	table[0].Name = keyname;
	table[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
	table[0].EntryContext = buffer;
	table[0].DefaultType = REG_SZ;
	table[0].DefaultData = def;
	table[0].DefaultLength = strlen(def) + 1;

	// second table must have special NULL values
			table[1].QueryRoutine = NULL;
			table[1].Name = NULL;

			status = RtlQueryRegistryValues(
					RTL_REGISTRY_ABSOLUTE,
					path,
					table,
					NULL,
					NULL);

			switch(status)
			{
				case STATUS_SUCCESS:
				option = *(long *) buffer;
				return STATUS_SUCCESS;
				case STATUS_OBJECT_NAME_NOT_FOUND:
				DbgPrint("Object not found");
				break;
				case STATUS_INVALID_PARAMETER:
				DbgPrint("Invalid Parameter");
				break;
				case STATUS_BUFFER_TOO_SMALL:
				DbgPrint("Buffer too small");
				break;
			}
			return STATUS_INVALID_PARAMETER;
		}

BOOLEAN UartInterruptHandler(PKINTERRUPT interruptObj, PVOID serviceContext)
{
	UART_DEVICE_DATA *dev = (UART_DEVICE_DATA *) serviceContext;
	unsigned char iir, ch = 0, i;

	iir = READ_PORT_UCHAR((PUCHAR)(IIR(dev->base_port)));
	//DbgPrint("[serial interrupt handle] read IIR = %d", (int) iir);

	if (IIR_RDAI(iir))
	{
		// Dezactivez �ntreruperea RDAI - read
		WRITE_PORT_UCHAR((PUCHAR) IER(dev->base_port), READ_PORT_UCHAR(
				(PUCHAR) IER(dev->base_port)) & ~IER_RDAI);

		while (READ_PORT_UCHAR((PUCHAR)(LSR(dev->base_port))) & 0x01)
		{
			ch = READ_PORT_UCHAR((PUCHAR) dev->base_port);
			dev->read_buffer[dev->read++] = ch;
		}

		KeSynchronizeExecution(dev->interruptObj, ReadUartSyncRoutine, dev);
	}

	if (IIR_THREI(iir))
	{
		// Dezactivez �ntreruperea THREI - write
		WRITE_PORT_UCHAR((PUCHAR) IER(dev->base_port), READ_PORT_UCHAR(
				(PUCHAR) IER(dev->base_port)) & ~IER_THREI);

		for (i = 0; i < dev->write; i++)
			WRITE_PORT_UCHAR((PUCHAR) dev->base_port, dev->write_buffer[i]);

		KeSynchronizeExecution(dev->interruptObj, WriteUartSyncRoutine, dev);
	}
	return TRUE;
}

NTSTATUS UartOpen(PDEVICE_OBJECT device, IRP *irp)
{
	UART_DEVICE_DATA *data = (UART_DEVICE_DATA *) device->DeviceExtension;

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS UartClose(PDEVICE_OBJECT device, IRP *irp)
{
	UART_DEVICE_DATA *data = (UART_DEVICE_DATA *) device->DeviceExtension;

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS UartRead(PDEVICE_OBJECT device, IRP *irp)
{
	UART_DEVICE_DATA * data = (UART_DEVICE_DATA *) device->DeviceExtension;
	PIO_STACK_LOCATION pIrpStack;
	PCHAR readBuffer;
	ULONG result;
	NTSTATUS status;

	//DbgPrint("[UART Read] Device read\n");

	pIrpStack = IoGetCurrentIrpStackLocation(irp);

	status = KeWaitForSingleObject(&data->event_reads, Executive, KernelMode,
			TRUE, NULL);
	if (status != STATUS_SUCCESS)
	{
		//DbgPrint("KeWaitForSingleObject() failed with status %d", (int) status);
		return status;
	}

	result = data->read;
	readBuffer = irp->AssociatedIrp.SystemBuffer;
	RtlCopyMemory(readBuffer, data->read_buffer, result);
	data->read = 0;

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = result;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	// Reactivez intreruperile pentru citire: ce era inainte + RDAI
	WRITE_PORT_UCHAR((PUCHAR) IER(data->base_port), READ_PORT_UCHAR((PUCHAR)(
			IER(data->base_port))) | IER_RDAI);

	return STATUS_SUCCESS;
}

NTSTATUS UartWrite(PDEVICE_OBJECT device, IRP *irp)
{
	UART_DEVICE_DATA *data = (UART_DEVICE_DATA *) device->DeviceExtension;
	PIO_STACK_LOCATION pIrpStack;
	PCHAR writeBuffer;
	ULONG result;
	NTSTATUS status;

	pIrpStack = IoGetCurrentIrpStackLocation(irp);
	if (pIrpStack->Parameters.Write.Length < 14)
		data->write = pIrpStack->Parameters.Write.Length;
	else
		data->write = 14;
	writeBuffer = irp->AssociatedIrp.SystemBuffer;

	result = data->write;
	RtlCopyMemory(data->write_buffer, writeBuffer, result);

	//DbgPrint("[UART Write] Device write %d\n", result);
	status = KeWaitForSingleObject(&data->event_writes, Executive, KernelMode,
			TRUE, NULL);

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = result;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	// Reactivez intreruperile pentru scriere: ce era inainte + THREI
	WRITE_PORT_UCHAR((PUCHAR) IER(data->base_port), READ_PORT_UCHAR(
			(PUCHAR) IER(data->base_port)) | IER_THREI);

	return STATUS_SUCCESS;
}

NTSTATUS UartDeviceIoControl(PDEVICE_OBJECT device, IRP *irp)
{
	UART_DEVICE_DATA *data = (UART_DEVICE_DATA *) device->DeviceExtension;
	ULONG controlCode;
	PIO_STACK_LOCATION pIrpStack;
	NTSTATUS status = STATUS_SUCCESS;
	struct uart16550_line_info *info;

	/* get control code from stack location and buffer from IRP */
	pIrpStack = IoGetCurrentIrpStackLocation(irp);
	controlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
	info = (struct uart16550_line_info *) irp->AssociatedIrp.SystemBuffer;

	switch (controlCode)
	{
	case UART16550_IOCTL_SET_LINE:
		/*DbgPrint("[UartDeviceIoControl] baud %d, len %d, stop %d, par %d, base_port %d\n",
		 (int) info->baud,
		 (int) info->len,
		 (int) info->stop,
		 (int) info->par,
		 (int) data->base_port);*/
		SET_LINE(info->baud, info->len, info->stop, info->par, data->base_port);
		break;
	default:
		status = STATUS_INVALID_PARAMETER;
		break;
	}

	/* complete IRP */
	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return status;
}

UNICODE_STRING *TO_UNICODE(const CHAR *str, UNICODE_STRING *unicodeStr)
{
	ANSI_STRING ansiStr;

	RtlInitAnsiString(&ansiStr, str);
	if (RtlAnsiStringToUnicodeString(unicodeStr, &ansiStr, TRUE)
			!= STATUS_SUCCESS)
		return NULL;

	return unicodeStr;
}

static int do_requests(PDRIVER_OBJECT driver, int id)
{
	DEVICE_OBJECT *device;
	UART_DEVICE_DATA *data;
	UNICODE_STRING devUnicodeName, linkUnicodeName;
	NTSTATUS status;
	KIRQL kirql;
	KAFFINITY kaf;
	ULONG kv;

	RtlZeroMemory(&devUnicodeName, sizeof(devUnicodeName));
	RtlZeroMemory(&linkUnicodeName, sizeof(linkUnicodeName));

	kv = HalGetInterruptVector(Internal, 0, id == 0 ? COM1_IRQ : COM2_IRQ, 0,
			&kirql, &kaf);

	status = IoCreateDevice(driver, sizeof(UART_DEVICE_DATA), TO_UNICODE((id
			== 0 ? COM1_PATH_KERNEL : COM2_PATH_KERNEL), &devUnicodeName),
			FILE_DEVICE_UNKNOWN, 0, FALSE, &device);
	if (status != STATUS_SUCCESS)
		goto error;

	status = IoCreateSymbolicLink(TO_UNICODE((id == 0 ? COM1_PATH_LINK
			: COM2_PATH_LINK), &linkUnicodeName), &devUnicodeName);
	if (status != STATUS_SUCCESS)
		goto error;

	data = (UART_DEVICE_DATA *) device->DeviceExtension;
	data->base_port = id == 0 ? COM1_BASEPORT : COM2_BASEPORT;
	data->bufferSize = 14;
	data->DeviceObject = device;

	// Read
	data->read = 0;
	KeInitializeEvent(&data->event_reads, SynchronizationEvent, FALSE);

	// Write
	data->write = 0;
	KeInitializeEvent(&data->event_writes, SynchronizationEvent, FALSE);

	RtlZeroMemory(data->read_buffer, SIZE);
	RtlZeroMemory(data->write_buffer, SIZE);

	if (devUnicodeName.Buffer)
		RtlFreeUnicodeString(&devUnicodeName);
	if (linkUnicodeName.Buffer)
		RtlFreeUnicodeString(&linkUnicodeName);

	device->Flags |= DO_BUFFERED_IO;
	driver->MajorFunction[IRP_MJ_CREATE] = UartOpen;
	driver->MajorFunction[IRP_MJ_READ] = UartRead;
	driver->MajorFunction[IRP_MJ_WRITE] = UartWrite;
	driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = UartDeviceIoControl;
	driver->MajorFunction[IRP_MJ_CLOSE] = UartClose;

	if ((status = IoConnectInterrupt(&data->interruptObj, UartInterruptHandler,
			data, NULL, kv, kirql, kirql, Latched, TRUE, kaf, FALSE))
			!= STATUS_SUCCESS)
	{
		//DbgPrint("IoConnectInterrupt failed (%08x)\n", status);
		goto error;
	}

	WRITE_PORT_UCHAR((PUCHAR) FCR(data->base_port), 0xc7);
	WRITE_PORT_UCHAR((PUCHAR) MCR(data->base_port), 0x08);
	WRITE_PORT_UCHAR((PUCHAR) IER(data->base_port), IER_THREI | IER_RDAI);

	return STATUS_SUCCESS;

	error: if (devUnicodeName.Buffer)
		RtlFreeUnicodeString(&devUnicodeName);
	if (linkUnicodeName.Buffer)
		RtlFreeUnicodeString(&linkUnicodeName);
	DriverUnload(driver);
	return status;
}

//#define INSANE_DEBUG

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registry)
{
	NTSTATUS status;
	int i;

#ifdef INSANE_DEBUG
	option = OPTION_COM2;
#else
	if ((status = loadParameters()) != STATUS_SUCCESS)
		return status;
#endif

	if (option & OPTION_COM1)
	{
		//DbgPrint("option COM1");
		info[0] = 1;
	}

	if (option & OPTION_COM2)
	{
		//DbgPrint("option COM2");
		info[1] = 1;
	}

	//DbgPrint("option = %d, COM1 = %d, COM2 = %d\n", option, info[0], info[1]);
	driver->DriverUnload = DriverUnload;

	for (i = 0; i < 2; i++)
	{
		if (info[i] == 1)
		{
			if ((status = do_requests(driver, i)) != STATUS_SUCCESS)
				return status;
		}
	}

	//DbgPrint("[DriverEntry] Driver loaded\n");
	return STATUS_SUCCESS;
}

void DriverUnload(PDRIVER_OBJECT driver)
{
	DEVICE_OBJECT *device;
	UNICODE_STRING linkUnicodeName;
	UART_DEVICE_DATA *data;

	// Dezactivez intreruperile
	if (info[0])
	{
		WRITE_PORT_UCHAR((PUCHAR) MCR(COM1_BASEPORT), 0x00);
		WRITE_PORT_UCHAR((PUCHAR) IER(COM1_BASEPORT), 0x00);
	}
	if (info[1])
	{
		WRITE_PORT_UCHAR((PUCHAR) MCR(COM2_BASEPORT), 0x00);
		WRITE_PORT_UCHAR((PUCHAR) IER(COM2_BASEPORT), 0x00);
	}

	if (TO_UNICODE(((option == OPTION_COM1) ? COM1_PATH_LINK : COM2_PATH_LINK),
			&linkUnicodeName))
		IoDeleteSymbolicLink(&linkUnicodeName);
	if (linkUnicodeName.Buffer)
		RtlFreeUnicodeString(&linkUnicodeName);

	while (TRUE)
	{
		device = driver->DeviceObject;

		if (device == NULL)
			break;

		data = (UART_DEVICE_DATA *) device->DeviceExtension;
		if (data->interruptObj)
			IoDisconnectInterrupt(data->interruptObj);
		IoDeleteDevice(device);
	}

	//DbgPrint("[DriverUnload] Driver unloaded\n");
}
