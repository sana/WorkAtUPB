Dascalu Laurentiu, 342 C3
Tema 2 SO2

  Implementarea este foarte asemanatoare cu cea de pe Linux, diferentele
fiind utilizarea unui DPC-urilor si a event-urilor pentru blocarea
functiilor de read si write.
  DPC-uri folosesc in intrerupere pentru deblocarea functiilor de read si
write:
	ReadUartSyncRoutine() { signal event }
	WriteUartSyncRoutine() { signal event }
	
	UartInterruptHandler()
	{
		// Read
		KeSynchronizeExecution(dev->interruptObj, ReadUartSyncRoutine, dev);
		
		// Write
		KeSynchronizeExecution(dev->interruptObj, WriteUartSyncRoutine, dev);
	}
  Event-urile deblocheaza functiile de read/write:
	// Am terminat de citit datele
	status = KeWaitForSingleObject(&data->event_reads, Executive, KernelMode, TRUE, NULL);
  
	// Write data in userspace
	
  In rest, logica este aceeasi ca pe Linux; am refolosit functiile si macro-urile
de lucru cu interfata seriala.
