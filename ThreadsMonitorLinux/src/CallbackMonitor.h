/* CallbackMonitor.h */
#ifndef _CALLBACKMONITOR_H_
#define _CALLBACKMONITOR_H_

void IncEnter();
void DecEnter();
void IncSignal();
void DecSignal();
void IncWait();
void DecWait();
void IncCond(int nrCond);
void DecCond(int nrCond);

#endif
