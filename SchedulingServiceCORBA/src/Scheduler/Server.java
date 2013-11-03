/**
 * Dascalu Laurentiu, 342C3
 * Tema 4 SPRC, Serviciu de planificare bazat pe CORBA
 * 
 * Clasa ce implementeaza operatiile de server.
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
import ISchedulerCORBA.ISchedulerServer;
import ISchedulerCORBA.ISchedulerServerHelper;
import ISchedulerCORBA.ISchedulerServerPOA;
import ISchedulerCORBA.SJob;
import ISchedulerCORBA.SJobResult;

class ServerImpl extends ISchedulerServerPOA {

	@Override
	public SJobResult executeJob(SJob job) {
		SJobResult result = new SJobResult();
		try {
			long time = System.currentTimeMillis() % Constants.MAX_WORK_TIME;
			Logger.info("Executing a job; this will take " + time);
			Thread.sleep(time);
			Logger.info("Job executed with success");
		} catch (Exception e) {

		}
		result.m_result = "Finished execution of \"" + job.m_job + "\"";
		return result;
	}

	@Override
	public String getCPU() {
		return System.currentTimeMillis() % 100 + "%";
	}

	@Override
	public String getMem() {
		return System.currentTimeMillis() % 100 + "%";
	}
}

public class Server {
	static IScheduler schedulerImpl;

	private static String getServerID(NamingContextExt ncRef) throws Exception {
		// resolve the Object Reference in Naming
		String name = Constants.SCHEDULER_SERVER;

		// The inner call gets the reference and the outer one narrows it
		// to the proper type.
		schedulerImpl = ISchedulerHelper.narrow(ncRef.resolve_str(name));

		Logger.debug("Obtained a handle on server object: " + schedulerImpl);

		return Constants.SERVER + schedulerImpl.getServerID();
	}

	public static void main(String args[]) {
		try {
			// create and initialize the ORB
			ORB orb = ORB.init(args, null);

			// get reference to rootpoa & activate the POAManager
			POA rootpoa = POAHelper.narrow(orb
					.resolve_initial_references("RootPOA"));
			rootpoa.the_POAManager().activate();

			// create servant and register it with the ORB
			ServerImpl serverImpl = new ServerImpl();

			// get object reference from the servant
			org.omg.CORBA.Object ref = rootpoa.servant_to_reference(serverImpl);
			ISchedulerServer href = ISchedulerServerHelper.narrow(ref);

			// get the root naming context
			// The string "NameService" is defined for all CORBA ORBs.
			org.omg.CORBA.Object objRef = orb
					.resolve_initial_references("NameService");

			// objRef is a generic object reference. We must narrow it down
			// to the interface we need.
			// Use NamingContextExt which is part of the Interoperable
			// Naming Service (INS) specification.
			NamingContextExt ncRef = NamingContextExtHelper.narrow(objRef);

			// find out the current server id
			String serverID = getServerID(ncRef);

			// bind the Object Reference with the Naming Service.
			NameComponent path[] = ncRef.to_name(serverID);

			// pas the name to the Naming Service, binding the href to the
			// string
			ncRef.rebind(path, href);

			Logger.info(serverID + " ready and waiting ...");

			// wait for invocations from clients
			orb.run();
		}

		catch (Exception e) {
			System.err.println("ERROR: " + e);
			e.printStackTrace(System.out);
		}

		System.out.println(Constants.SERVER + " Exiting ...");
	}
}
