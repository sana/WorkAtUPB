// Dascalu Laurentiu, 342 C3 - Software RAID
// Tema 3 SO2, Windows
#include <ntddk.h>
#include <ntstrsafe.h>
#include "ssr_util.h"
#include "crc32.h"

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

typedef struct _SSR_PHYSICAL_DEVICE
{
	UNICODE_STRING name;
	PFILE_OBJECT fileObject;
	PDEVICE_OBJECT deviceObject;
	IO_STATUS_BLOCK ioStatus;
} SSR_PHYSICAL_DEVICE, *PSSR_PHYSICAL_DEVICE;

typedef struct _SSR_DEVICE_DATA
{
	PDEVICE_OBJECT DeviceObject;
	char writeBuffer[KERNEL_SECTOR_SIZE];
	char readBuffer[2][KERNEL_SECTOR_SIZE];
	ULONG writeBufferSize;
} SSR_DEVICE_DATA, *PSSR_DEVICE_DATA;

/* physical device data */
static SSR_PHYSICAL_DEVICE physicalDevice[2];

static unsigned char crcBuffer[KERNEL_SECTOR_SIZE];

static NTSTATUS SendIrp(SSR_PHYSICAL_DEVICE *dev, ULONG major, PVOID buffer,
		ULONG length, LARGE_INTEGER offset)
{
	PIRP irp = NULL;
	KEVENT irpEvent;
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("[SendIrp] major %ul, length %u, offset %lld", major, length,
			offset.QuadPart);
	KeInitializeEvent(&irpEvent, NotificationEvent, FALSE);

	irp = IoBuildSynchronousFsdRequest(major, dev->deviceObject, buffer,
			length, &offset, &irpEvent, &dev->ioStatus);
	if (irp == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}

	status = IoCallDriver(dev->deviceObject, irp);

	KeWaitForSingleObject(&irpEvent, Executive, KernelMode, FALSE, NULL);
	status = dev->ioStatus.Status;

	return status;
}

static NTSTATUS SendCRCIrp(SSR_PHYSICAL_DEVICE *dev, ULONG major,
		LARGE_INTEGER offset, unsigned int *crc)
{
	PIRP irp = NULL;
	KEVENT irpEvent;
	NTSTATUS status = STATUS_SUCCESS;
	LARGE_INTEGER crcOffset;
	unsigned long sectorID;
	unsigned int crcSegment, crcSegmentOffset, nCRCPerSector;

	nCRCPerSector = KERNEL_SECTOR_SIZE / sizeof(unsigned int);
	sectorID = (unsigned long) offset.QuadPart / KERNEL_SECTOR_SIZE;
	crcSegment = (unsigned int) sectorID / nCRCPerSector;
	crcSegmentOffset = (unsigned int) ((sectorID % nCRCPerSector)
			* sizeof(unsigned int));

	DbgPrint(
			"[SendCRCIrp] sector id %ul, crc segment %u, crc segment offset %u",
			sectorID, crcSegment, crcSegmentOffset);

	crcOffset.QuadPart = LOGICAL_DISK_SIZE + crcSegment * KERNEL_SECTOR_SIZE;

	if (major == IRP_MJ_WRITE)
	{
		// Citesc sectorul de CRC-uri vechi si salvez noul CRC
		if ((status = SendIrp(dev, IRP_MJ_READ, crcBuffer, KERNEL_SECTOR_SIZE,
				crcOffset)) != STATUS_SUCCESS)
		{
			DbgPrint("[SendCRCIrp] failed to read CRC sector #", sectorID);
			return status;
		}
		*(unsigned int *) (crcBuffer + crcSegmentOffset) = *crc;
	}

	KeInitializeEvent(&irpEvent, NotificationEvent, FALSE);

	irp = IoBuildSynchronousFsdRequest(major, dev->deviceObject, &crcBuffer,
			KERNEL_SECTOR_SIZE, &crcOffset, &irpEvent, &dev->ioStatus);
	if (irp == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}

	status = IoCallDriver(dev->deviceObject, irp);

	KeWaitForSingleObject(&irpEvent, Executive, KernelMode, FALSE, NULL);

	status = dev->ioStatus.Status;

	if (major == IRP_MJ_READ)
		*crc = *(unsigned int *) (crcBuffer + crcSegmentOffset);

	return status;
}

NTSTATUS MyOpen(PDEVICE_OBJECT device, IRP *irp)
{
	/* data is not required; retrieve it for consistency */
	SSR_DEVICE_DATA * data = (SSR_DEVICE_DATA *) device->DeviceExtension;

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	DbgPrint("[MyOpen] Device opened\n");
	return STATUS_SUCCESS;
}

NTSTATUS MyClose(PDEVICE_OBJECT device, IRP *irp)
{
	/* data is not required; retrieve it for consistency */
	SSR_DEVICE_DATA * data = (SSR_DEVICE_DATA *) device->DeviceExtension;

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	DbgPrint("[MyClose] Device closed\n");
	return STATUS_SUCCESS;
}

NTSTATUS MyRead(PDEVICE_OBJECT device, IRP *irp)
{
	SSR_DEVICE_DATA * data = (SSR_DEVICE_DATA *) device->DeviceExtension;
	PIO_STACK_LOCATION pIrpStack;
	PCHAR readBuffer;
	ULONG sizeToRead, sizeRead;
	NTSTATUS status = STATUS_SUCCESS;
	size_t i;
	unsigned int crc[2], crcReal[2];

	pIrpStack = IoGetCurrentIrpStackLocation(irp);
	sizeToRead = pIrpStack->Parameters.Read.Length;
	sizeRead = (sizeToRead < KERNEL_SECTOR_SIZE) ? sizeToRead
			: KERNEL_SECTOR_SIZE;

	DbgPrint("[MyWrite] sizeRead %d, offset %d", sizeRead,
			(int) pIrpStack->Parameters.Read.ByteOffset.QuadPart);

	if (pIrpStack->Parameters.Read.ByteOffset.QuadPart < 0
			|| pIrpStack->Parameters.Read.ByteOffset.QuadPart
					>= LOGICAL_DISK_SIZE)
	{
		DbgPrint("[MyRead] invalid offset");
		status = STATUS_INSUFFICIENT_RESOURCES;
		sizeRead = 0;
		goto end;
	}

	DbgPrint("[MyRead] sizeRead %d", sizeRead);

	for (i = 0; i < 2; i++)
	{
		if ((status = SendIrp(&physicalDevice[i], IRP_MJ_READ,
				&data->readBuffer[i], sizeRead,
				pIrpStack->Parameters.Read.ByteOffset)) != STATUS_SUCCESS)
		{
			DbgPrint("[MyRead] send Irp for the reading operation FAILED");
			status = STATUS_INSUFFICIENT_RESOURCES;
			sizeRead = 0;
			goto end;
		}

		if ((status = SendCRCIrp(&physicalDevice[i], IRP_MJ_READ,
				pIrpStack->Parameters.Write.ByteOffset, &crc[i]))
				!= STATUS_SUCCESS)
		{
			DbgPrint("[MyRead] send CRC Irp read failed");
			status = STATUS_INSUFFICIENT_RESOURCES;
			sizeRead = 0;
			goto end;
		}
	}

	readBuffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress,
			NormalPagePriority);
	crcReal[0]
			= update_crc(0, (unsigned char *) &data->readBuffer[0], sizeRead);
	crcReal[1]
			= update_crc(0, (unsigned char *) &data->readBuffer[1], sizeRead);

	if ((crc[0] == crcReal[0]) && (crc[1] == crcReal[1]) && (crcReal[0]
			== crcReal[1]))
	{
		// Datele de pe ambele partitii sunt corecte
		RtlCopyMemory(readBuffer, data->readBuffer[0], sizeRead);
	}
	else if (crc[0] == crcReal[0])
	{
		// Fac recuperare din eroare folosind prima partitie: scriu datele pe a
		// doua partitie si apoi CRC-ul
		if ((status = SendIrp(&physicalDevice[1], IRP_MJ_WRITE,
				&data->readBuffer[0], sizeRead,
				pIrpStack->Parameters.Read.ByteOffset)) != STATUS_SUCCESS)
		{
			sizeRead = 0;
			goto end;
		}

		if ((status = SendCRCIrp(&physicalDevice[1], IRP_MJ_WRITE,
				pIrpStack->Parameters.Read.ByteOffset, &crc[0]))
				!= STATUS_SUCCESS)
		{
			sizeRead = 0;
			goto end;
		}
	}
	else if (crc[1] == crcReal[1])
	{
		// Fac recuperare din eroare folosind a doua partitie: scriu datele
		// pe prima partitie si apoi CRC-ul
		if ((status = SendIrp(&physicalDevice[0], IRP_MJ_WRITE,
				&data->readBuffer[1], sizeRead,
				pIrpStack->Parameters.Read.ByteOffset)) != STATUS_SUCCESS)
		{
			sizeRead = 0;
			goto end;
		}

		if ((status = SendCRCIrp(&physicalDevice[0], IRP_MJ_WRITE,
				pIrpStack->Parameters.Read.ByteOffset, &crc[1]))
				!= STATUS_SUCCESS)
		{
			sizeRead = 0;
			goto end;
		}
	}
	else
	{
		DbgPrint("%u %u %u %u", crc[0], crc[1], crcReal[0], crcReal[1]);
		status = STATUS_INSUFFICIENT_RESOURCES;
		sizeRead = 0;
		goto end;
	}

	end:
	/* complete IRP */
	irp->IoStatus.Status = status;
	irp->IoStatus.Information = sizeRead;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS MyWrite(PDEVICE_OBJECT device, IRP *irp)
{
	SSR_DEVICE_DATA * data = (SSR_DEVICE_DATA *) device->DeviceExtension;
	PIO_STACK_LOCATION pIrpStack;
	PCHAR writeBuffer;
	ULONG sizeToWrite, sizeWritten;
	NTSTATUS status = STATUS_SUCCESS;
	size_t i;
	unsigned int crc;

	pIrpStack = IoGetCurrentIrpStackLocation(irp);

	sizeToWrite = pIrpStack->Parameters.Write.Length;
	sizeWritten = (sizeToWrite <= KERNEL_SECTOR_SIZE) ? sizeToWrite
			: KERNEL_SECTOR_SIZE;

	DbgPrint("[MyWrite] sizeWritten %d, offset %d", sizeWritten,
			(int) pIrpStack->Parameters.Write.ByteOffset.QuadPart);

	if (pIrpStack->Parameters.Write.ByteOffset.QuadPart < 0
			|| pIrpStack->Parameters.Write.ByteOffset.QuadPart
					>= LOGICAL_DISK_SIZE)
	{
		DbgPrint("[MyWrite] invalid offset");
		status = STATUS_INSUFFICIENT_RESOURCES;
		sizeWritten = 0;
		goto end;
	}

	RtlZeroMemory(data->writeBuffer, KERNEL_SECTOR_SIZE);
	data->writeBufferSize = 0;

	writeBuffer = MmGetSystemAddressForMdlSafe(irp->MdlAddress,
			NormalPagePriority);
	RtlCopyMemory(data->writeBuffer, writeBuffer, sizeWritten);
	data->writeBufferSize = sizeWritten;

	crc = (unsigned int) update_crc(0, data->writeBuffer, sizeWritten);
	DbgPrint("[MyWrite] CRC on buffer %u", crc);
	for (i = 0; i < 2; i++)
	{
		if ((status = SendIrp(&physicalDevice[i], IRP_MJ_WRITE,
				data->writeBuffer, sizeWritten,
				pIrpStack->Parameters.Write.ByteOffset)) != STATUS_SUCCESS)
		{
			sizeWritten = 0;
			goto end;
		}

		if ((status = SendCRCIrp(&physicalDevice[i], IRP_MJ_WRITE,
				pIrpStack->Parameters.Write.ByteOffset, &crc))
				!= STATUS_SUCCESS)
		{
			DbgPrint("Send CRC Irp read failed");
			sizeWritten = 0;
			goto end;
		}
	}

	end:
	/* complete IRP */
	irp->IoStatus.Status = status;
	irp->IoStatus.Information = sizeWritten;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS MyDeviceIoControl(PDEVICE_OBJECT device, IRP *irp)
{
	/* data is not required; retrieve it for consistency */
	SSR_DEVICE_DATA * data = (SSR_DEVICE_DATA *) device->DeviceExtension;

	/* complete IRP */
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

static NTSTATUS OpenPhysicalDisk(PCWSTR diskName, SSR_PHYSICAL_DEVICE *dev)
{
	RtlInitUnicodeString(&dev->name, diskName);
	return IoGetDeviceObjectPointer(&dev->name, GENERIC_READ | GENERIC_WRITE,
			&dev->fileObject, &dev->deviceObject);
}

static void ClosePhysicalDisk(SSR_PHYSICAL_DEVICE *dev)
{
	ObDereferenceObject(dev->fileObject);
}

UNICODE_STRING *TO_UNICODE(PVOID str, UNICODE_STRING *unicodeStr)
{
	ANSI_STRING ansiStr;

	RtlInitAnsiString(&ansiStr, str);
	if (RtlAnsiStringToUnicodeString(unicodeStr, &ansiStr, TRUE)
			!= STATUS_SUCCESS)
		return NULL;

	return unicodeStr;
}

static NTSTATUS LoadPhysicalDisk(PCWSTR physicalDiskName,
		SSR_PHYSICAL_DEVICE *physicalDevice)
{
	NTSTATUS status;

	DbgPrint("[LoadPhysicalDisk]");

	if ((status = OpenPhysicalDisk(physicalDiskName, physicalDevice))
			!= STATUS_SUCCESS)
	{
		DbgPrint("[Driver Entry] Error opening physical disk\n");
		return status;
	}
	return STATUS_SUCCESS;
}

static NTSTATUS LoadLogicalDisk(PDRIVER_OBJECT driver)
{
	UNICODE_STRING devUnicodeName, linkUnicodeName;
	DEVICE_OBJECT *device;
	SSR_DEVICE_DATA *data;
	NTSTATUS status;

	DbgPrint("[LoadLogicalDisk] %s %s\n", LOGICAL_DISK_DEVICE_NAME,
			LOGICAL_DISK_LINK_NAME);

	RtlZeroMemory(&devUnicodeName, sizeof(devUnicodeName));
	RtlZeroMemory(&linkUnicodeName, sizeof(linkUnicodeName));

	if ((status = IoCreateDevice(driver, sizeof(SSR_DEVICE_DATA), TO_UNICODE(
			LOGICAL_DISK_DEVICE_NAME, &devUnicodeName), FILE_DEVICE_DISK, 0,
			FALSE, &device)) != STATUS_SUCCESS)
	{
		DbgPrint("[LoadLogicalDisk] IoCreateDevice status %d %d %d %d", status,
				STATUS_INSUFFICIENT_RESOURCES, STATUS_OBJECT_NAME_EXISTS,
				STATUS_OBJECT_NAME_COLLISION);
		goto error;
	}

	if ((status = IoCreateSymbolicLink(TO_UNICODE(LOGICAL_DISK_LINK_NAME,
			&linkUnicodeName), &devUnicodeName)) != STATUS_SUCCESS)
	{
		DbgPrint("[LoadLogicalDisk] IoCreateSymbolicLink status %d", status);
		goto error;
	}

	if (devUnicodeName.Buffer)
		RtlFreeUnicodeString(&devUnicodeName);
	if (linkUnicodeName.Buffer)
		RtlFreeUnicodeString(&linkUnicodeName);

	data = (SSR_DEVICE_DATA *) device->DeviceExtension;
	data->DeviceObject = device;
	RtlZeroMemory(data->writeBuffer, KERNEL_SECTOR_SIZE);
	data->writeBufferSize = 0;

	device->Flags |= DO_BUFFERED_IO;
	driver->MajorFunction[IRP_MJ_CREATE] = MyOpen;
	driver->MajorFunction[IRP_MJ_READ] = MyRead;
	driver->MajorFunction[IRP_MJ_WRITE] = MyWrite;
	driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MyDeviceIoControl;
	driver->MajorFunction[IRP_MJ_CLOSE] = MyClose;

	return STATUS_SUCCESS;

	error: if (devUnicodeName.Buffer)
		RtlFreeUnicodeString(&devUnicodeName);
	if (linkUnicodeName.Buffer)
		RtlFreeUnicodeString(&linkUnicodeName);
	return status;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registry)
{
	PDEVICE_OBJECT pDevice;
	NTSTATUS status;

	// Primul disc fizic
	if ((status = LoadPhysicalDisk(PHYSICAL_DISK1_DEVICE_NAME,
			&physicalDevice[0])) != STATUS_SUCCESS)
		goto end;

	// Al doilea disc fizic
	if ((status = LoadPhysicalDisk(PHYSICAL_DISK2_DEVICE_NAME,
			&physicalDevice[1])) != STATUS_SUCCESS)
		goto error1;

	// Discul logic
	if ((status = LoadLogicalDisk(driver)) != STATUS_SUCCESS)
		goto error2;

	driver->DriverUnload = DriverUnload;
	DbgPrint("[DriverEntry] Success\n");
	return STATUS_SUCCESS;
	error2: DbgPrint("[DriverEntry] Could not open the logical disk\n");
	ClosePhysicalDisk(&physicalDevice[1]);
	error1: DbgPrint("[DriverEntry] Could not open the second physical disk\n");
	ClosePhysicalDisk(&physicalDevice[0]);
	end: DbgPrint("[DriverEntry] Could not open the first physical disk\n");
	return status;
}

void DriverUnload(PDRIVER_OBJECT driver)
{
	DEVICE_OBJECT *device;
	UNICODE_STRING linkUnicodeName;

	if (TO_UNICODE(LOGICAL_DISK_LINK_NAME, &linkUnicodeName))
		IoDeleteSymbolicLink(&linkUnicodeName);
	if (linkUnicodeName.Buffer)
		RtlFreeUnicodeString(&linkUnicodeName);

	while (TRUE)
	{
		device = driver->DeviceObject;
		if (device == NULL)
			break;
		IoDeleteDevice(device);
	}

	ClosePhysicalDisk(&physicalDevice[0]);
	ClosePhysicalDisk(&physicalDevice[1]);

	DbgPrint("[DriverUnload] Success\n");
}
