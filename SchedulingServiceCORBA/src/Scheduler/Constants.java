/**
 * Dascalu Laurentiu, 342C3
 * Tema 4 SPRC, Serviciu de planificare bazat pe CORBA
 */
package Scheduler;

public class Constants {
	public final static String IN_PROGRESS = "Job in progress...";
	public final static String SCHEDULER_SERVER = "SchedulerServer";
	public final static String SERVER = "Server";
	public final static String CLIENT = "Client";
	public final static Integer SLEEP_TIME = 3000;
	public final static Integer JITTY_TIME = 200;
	public final static String SLEEPING = "Sleeping for " + (SLEEP_TIME / 1000)
			+ " second";
	public final static Integer MAX_WORK_TIME = 5000;
	public final static String DEBUG_FILE = "debug.txt";
}
