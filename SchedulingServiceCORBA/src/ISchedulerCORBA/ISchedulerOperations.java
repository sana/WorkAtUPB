package ISchedulerCORBA;


/**
* ISchedulerCORBA/ISchedulerOperations.java .
* Generated by the IDL-to-Java compiler (portable), version "3.2"
* from IScheduler.idl
* Monday, December 28, 2009 7:50:06 PM EET
*/

public interface ISchedulerOperations 
{
  short submitJob (ISchedulerCORBA.SJob job);
  short getServerID ();
  ISchedulerCORBA.SJobResult getResult (short jobID);
} // interface ISchedulerOperations
