/**
 * Dascalu Laurentiu, 342C3
 * Tema 4 SPRC, Serviciu de planificare bazat pe CORBA
 */

package Scheduler;

public class Logger {
	// Debug mode
	private final static boolean DEBUG = false;

	public static void debug(String str) {
		if (DEBUG)
			System.out.println(str);
	}

	public static void info(String str) {
		System.out.println(str);
	}
}
