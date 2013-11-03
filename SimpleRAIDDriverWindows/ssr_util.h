/*
 * Simple Software Raid - Windows header file
 */

#ifndef SSR_H_
#define SSR_H_		1

#define PHYSICAL_DISK1_DEVICE_NAME	(L"\\DosDevices\\PhysicalDrive1")
#define PHYSICAL_DISK2_DEVICE_NAME	(L"\\DosDevices\\PhysicalDrive2")
#define PHYSICAL_DISK1_USER_NAME	"\\\\.\\PhysicalDrive1"
#define PHYSICAL_DISK2_USER_NAME	"\\\\.\\PhysicalDrive2"

/* physical partition size - 95 MB (more than this results in error) */
#define LOGICAL_DISK_DEVICE_NAME	("\\Device\\SoftwareRAID")
#define LOGICAL_DISK_LINK_NAME		("\\??\\SoftwareRAID")
#define LOGICAL_DISK_USER_NAME		"\\\\.\\SoftwareRAID"

/* sector size */
#define KERNEL_SECTOR_SIZE		512

#define LOGICAL_DISK_SIZE		(95 * 1024 * 1024)
#define LOGICAL_DISK_SECTORS		((LOGICAL_DISK_SIZE) / (KERNEL_SECTOR_SIZE))

#endif
