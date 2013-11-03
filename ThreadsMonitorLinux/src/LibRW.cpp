/*
 * @author Dascalu Laurentiu, 335CA
 * Implementarea unui monitor pentru problema Readers-Writers
 */
#include <cstdio>
#include <cstdlib>

extern "C"
{

#include "Constante.h"
#include "CallbackRW.h"
#include "LibMonitorInterface.h"

static int nreaders = 0, nwaitingreaders = 0, nwriters = 0, nwaitingwriters = 0;

Monitor* CreateRWMonitor()
{
	// 2 variabile de conditii : readers, writers
	return Create(2, SIGNAL_AND_CONTINUE);
}

int GetNrConds()
{
	return 2;
}

void StartCit(Monitor *m)
{
	Enter(m);

	// Verific daca pot citi
	while (nwriters + nwaitingwriters > 0)
	{
		// astept daca sunt scriitori
		nwaitingreaders++;
		Wait(m, 0);
		nwaitingreaders--;
	}

	nreaders++;

	Leave(m);
	AnnounceStartCit();
}

void StopCit(Monitor *m)
{
	Enter(m);

	nreaders--;

	// Trezesc un scriitor daca numarul de cititori este 0
	if (nreaders == 0 && nwaitingwriters > 0)
		Signal(m, 1);

	Leave(m);
	AnnounceStopCit();
}

void StartScrit(Monitor *m)
{
	Enter(m);

	// Verific daca pot scrie
	while (nreaders + nwriters > 0)
	{
		// Astept daca exista cititori sau scriitori
		nwaitingwriters++;
		Wait(m, 1);
		nwaitingwriters--;
	}

	nwriters++;

	Leave(m);
	AnnounceStartScrit();
}

void StopScrit(Monitor *m)
{
	Enter(m);

	nwriters--;

	// Ofer prioritate altui scriitor
	if (nwaitingwriters > 0)
		Signal(m, 1);
	// Daca nu mai exista scriitori, atunci trezesc toti cititorii
	else if (nwaitingreaders > 0)
		Broadcast(m, 0);

	Leave(m);
	AnnounceStopScrit();
}

void ResetScritCit()
{
	nreaders = nwaitingreaders = nwriters = 0;
}

}
