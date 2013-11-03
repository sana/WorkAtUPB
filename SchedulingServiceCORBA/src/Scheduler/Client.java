/**
 * Dascalu Laurentiu, 342C3
 * Tema 4 SPRC, Serviciu de planificare bazat pe CORBA
 * 
 * Clientul executa urmatorii pasi
 * - construieste o comanda de executat pe scheduler server
 * - trimite comanda catre scheduler server
 * - asteapta rezultat de la scheduler
 *  - daca acesta este nul atunci job-ul inca
 *  se executa si astept un nou rezultat
 *  - daca nu, atunci afisez rezultatul si ies
 */

package Scheduler;

import org.omg.CORBA.ORB;
import org.omg.CosNaming.NamingContextExt;
import org.omg.CosNaming.NamingContextExtHelper;

import ISchedulerCORBA.IScheduler;
import ISchedulerCORBA.ISchedulerHelper;
import ISchedulerCORBA.SJob;
import ISchedulerCORBA.SJobResult;

public class Client {
	static IScheduler schedulerImpl;

	public static void main(String args[]) {
		try {
			// A client needs its own ORB to perform all
			// marshalling and IIOP work.
			// create and initialize the ORB
			ORB orb = ORB.init(args, null);

			// get the root naming context
			org.omg.CORBA.Object objRef = orb
					.resolve_initial_references("NameService");
			// Use NamingContextExt instead of NamingContext. This is
			// part of the Interoperable naming Service.
			NamingContextExt ncRef = NamingContextExtHelper.narrow(objRef);

			// resolve the Object Reference in Naming
			String name = Constants.SCHEDULER_SERVER;

			// The inner call gets the reference and the outer one narrows it
			// to the proper type.
			schedulerImpl = ISchedulerHelper.narrow(ncRef.resolve_str(name));

			Logger
					.debug("Obtained a handle on server object: "
							+ schedulerImpl);

			SJob job = new SJob();

			job.m_job = "command " + Math.random();
			short id = schedulerImpl.submitJob(job);
			Logger.info("Job received id " + id);

			while (true) {
				SJobResult result = schedulerImpl.getResult(id);
				if (result.m_result.equals(Constants.IN_PROGRESS)) {
					Logger.info(Constants.IN_PROGRESS);
					Thread.sleep(Constants.SLEEP_TIME);
				} else {
					Logger.info("Final result is: [" + result.m_result + "]");
					break;
				}
			}

		} catch (Exception e) {
			System.out.println("ERROR : " + e);
			e.printStackTrace(System.out);
		}
	}
}