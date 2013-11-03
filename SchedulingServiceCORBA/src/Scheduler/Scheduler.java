/**
 * Dascalu Laurentiu, 342C3
 * Tema 4 SPRC, Serviciu de planificare bazat pe CORBA
 * 
 * Clasa dintre Client, Server si JobMapper. Logica Schedulerului
 * nu este implementata aici, din cauza motivelor de modularitate.
 */

package Scheduler;

import org.omg.CORBA.ORB;
import org.omg.CosNaming.NameComponent;
import org.omg.CosNaming.NamingContextExt;
import org.omg.CosNaming.NamingContextExtHelper;
import org.omg.PortableServer.POA;
import org.omg.PortableServer.POAHelper;

import ISchedulerCORBA.IScheduler;
import ISchedulerCORBA.ISchedulerHelper;
import ISchedulerCORBA.ISchedulerPOA;
import ISchedulerCORBA.SJob;
import ISchedulerCORBA.SJobResult;

class SchedulerImpl extends ISchedulerPOA {
	private JobMapper m_mapper;

	public SchedulerImpl() {
		m_mapper = new JobMapper();
		m_mapper.start();
	}

	@Override
	public short submitJob(SJob job) {
		Logger.debug("submitJob() " + job.m_job);
		return m_mapper.putJob(job);
	}

	private static SJobResult newJobResult() {
		SJobResult jobResult = new SJobResult();
		jobResult.m_result = Constants.IN_PROGRESS;
		return jobResult;
	}

	@Override
	public SJobResult getResult(short jobID) {
		Logger.debug("getResult() " + jobID);
		SJobResult result = m_mapper.getResult(jobID);
		if (result == null)
			return newJobResult();
		return result;
	}

	static short __server_id_counter = 0;

	@Override
	public short getServerID() {
		return __server_id_counter++;
	}

	public short getServers() {
		return __server_id_counter;
	}
}

public class Scheduler {
	private static SchedulerImpl schedulerImpl;
	private static NamingContextExt ncRef;

	public static void main(String args[]) {
		try {
			// create and initialize the ORB
			ORB orb = ORB.init(args, null);

			// get reference to rootpoa & activate the POAManager
			POA rootpoa = POAHelper.narrow(orb
					.resolve_initial_references("RootPOA"));
			rootpoa.the_POAManager().activate();

			// create servant and register it with the ORB
			schedulerImpl = new SchedulerImpl();

			// get object reference from the servant
			org.omg.CORBA.Object ref = rootpoa
					.servant_to_reference(schedulerImpl);
			IScheduler href = ISchedulerHelper.narrow(ref);

			// get the root naming context
			// The string "NameService" is defined for all CORBA ORBs.
			org.omg.CORBA.Object objRef = orb
					.resolve_initial_references("NameService");

			// objRef is a generic object reference. We must narrow it down
			// to the interface we need.
			// Use NamingContextExt which is part of the Interoperable
			// Naming Service (INS) specification.
			ncRef = NamingContextExtHelper.narrow(objRef);

			// bind the Object Reference with the Naming Service.
			NameComponent path[] = ncRef.to_name(Constants.SCHEDULER_SERVER);

			// pas the name to the Naming Service, binding the href to the
			// string
			ncRef.rebind(path, href);

			Logger.info(Constants.SCHEDULER_SERVER + " ready and waiting ...");

			// wait for invocations from clients
			orb.run();
		}

		catch (Exception e) {
			System.err.println("ERROR: " + e);
			e.printStackTrace(System.out);
		}

		System.out.println(Constants.SCHEDULER_SERVER + " Exiting ...");
	}

	public static short getServers() {
		return schedulerImpl.getServers();
	}

	public static NamingContextExt getNCRef() {
		return ncRef;
	}
}
