/**
 * Dascalu Laurentiu, 342C3
 * Tema 4 SPRC, Serviciu de planificare bazat pe CORBA
 * 
 * Clasa folosita pentru testarea automata a temei
 */

package Scheduler;

import java.io.PrintStream;

public class Debugger {
	static String[] m_args;

	static class SchedulerTest extends Thread {
		@Override
		public void run() {
			Scheduler.main(m_args);
		}
	}

	static class ServerTest extends Thread {
		@Override
		public void run() {
			Server.main(m_args);
		}
	}

	static class ClientTest extends Thread {
		@Override
		public void run() {
			Client.main(m_args);
		}
	}

	private static int nServers = 3;
	private static int nClients = 100;

	public static void main(String[] args) throws Exception {
		System.setOut(new PrintStream(Constants.DEBUG_FILE));
		m_args = args;

		Thread.sleep(Constants.SLEEP_TIME);
		
		System.err.println("Starting SchedulerServer");
		new SchedulerTest().start();

		// Wait some time for scheduler
		// to get up and running
		Thread.sleep(Constants.JITTY_TIME * 20);

		System.err.println("Starting " + nServers + " Servers");
		for (int i = 0; i < nServers; i++)
		{
			new ServerTest().start();
			Thread.sleep(Constants.JITTY_TIME);
		}

		Thread.sleep(Constants.JITTY_TIME * 10);

		System.err.println("Starting " + nClients + " Clients");
		ClientTest[] clients = new ClientTest[nClients];
		for (int j = 0; j < nClients; j++) {
			clients[j] = new ClientTest();
			clients[j].start();
		}

		System.err.println("Waiting for completion");
		for (int j = 0; j < nClients; j++)
			clients[j].join();

		System.err.println("Test execution finished");
		System.err.println("Cancel this test with CTRL + C");
	}
}
