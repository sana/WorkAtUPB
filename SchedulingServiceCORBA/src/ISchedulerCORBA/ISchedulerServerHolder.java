package ISchedulerCORBA;

/**
* ISchedulerCORBA/ISchedulerServerHolder.java .
* Generated by the IDL-to-Java compiler (portable), version "3.2"
* from IScheduler.idl
* Monday, December 28, 2009 7:50:06 PM EET
*/

public final class ISchedulerServerHolder implements org.omg.CORBA.portable.Streamable
{
  public ISchedulerCORBA.ISchedulerServer value = null;

  public ISchedulerServerHolder ()
  {
  }

  public ISchedulerServerHolder (ISchedulerCORBA.ISchedulerServer initialValue)
  {
    value = initialValue;
  }

  public void _read (org.omg.CORBA.portable.InputStream i)
  {
    value = ISchedulerCORBA.ISchedulerServerHelper.read (i);
  }

  public void _write (org.omg.CORBA.portable.OutputStream o)
  {
    ISchedulerCORBA.ISchedulerServerHelper.write (o, value);
  }

  public org.omg.CORBA.TypeCode _type ()
  {
    return ISchedulerCORBA.ISchedulerServerHelper.type ();
  }

}
