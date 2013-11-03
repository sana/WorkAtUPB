/*
 * @author Dascalu Laurentiu, 335CA
 * Interfata unui monitor generic
 */
#ifndef LIBMONITORINTERFACE_H_
#define LIBMONITORINTERFACE_H_

Monitor* Create(int conditions, char policy);
int Destroy(Monitor *_m);
int Enter(Monitor *_m);
int Leave(Monitor *_m);
int Wait(Monitor *_m, int cond);
int Signal(Monitor *_m, int cond);
int Broadcast(Monitor *_m, int cond);

#endif /* LIBMONITORINTERFACE_H_ */
