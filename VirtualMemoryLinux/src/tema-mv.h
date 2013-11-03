/*
 * Sisteme de Operare
 * tema-mv.h - header pentru Tema Memorie virtuala
 */

#ifndef TEMA_MV_H
#define TEMA_MV_H		1

#define MAX_PHYSICAL_PAGES		20
#define MAX_VIRTUAL_PAGES		50

#if defined (_WIN32)
	#include <windows.h>
	static DWORD pageSize = 0x10000;
#else
	#include <sys/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined (_WIN32)
	// Windows functions
	__declspec (dllexport) LPVOID VirtualInit (DWORD virtualPages, DWORD physicalPages);
	__declspec (dllexport) HANDLE GetRamHandle (void);
	__declspec (dllexport) HANDLE GetSwapHandle (void);
	__declspec (dllexport) void SynchronizeRam (void);
	__declspec (dllexport) BOOL VirtualEnd (void);
#else
	// Linux functions
	void *vinit (size_t virt_pages, size_t phys_pages);
	int get_ram_fd (void);
	int get_swap_fd (void);
	void ram_sync (void);
	int vend (void);
#endif

#ifdef __cplusplus
}
#endif

#endif
