/**
 * Dascalu Laurentiu, 342C3
 * Tema 4 SPRC, Serviciu de planificare bazat pe CORBA
 * 
 * Clasa ce abstractizeaza job-uri, prin folosirea de thread-uri.
 * Fiecare thread apeleaza metode de pe serverul cu incarcarea
 * cea mai mica.
 */

package Scheduler;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Vector;

import ISchedulerCORBA.ISchedulerServer;
import ISchedulerCORBA.ISchedulerServerHelper;
import ISchedulerCORBA.SJob;
import ISchedulerCORBA.SJobResult;

class SJobThread extends Thread {
	private SJob job;
	private short jobID;
	private SJobResult result;
	private ISchedulerServer server;
	private volatile boolean threadFinished;

	public SJobThread(SJob job, short jobID, ISchedulerServer server) {
		this.job = job;
		this.jobID = jobID;
		this.server = server;
		threadFinished = false;
	}

	@Override
	public void run() {
		Logger.debug("[SJobThread] starting job execution on server " + server);
		if (server != null)
			result = server.executeJob(job);
		else
			Logger
					.info("No server available to run this task, please try again later");
		threadFinished = true;
	}

	public boolean finished() {
		return threadFinished;
	}

	public SJobResult getResult() {
		if (threadFinished)
			return result;
		return null;
	}

	public short getJobID() {
		return jobID;
	}
}

public class JobMapper extends Thread {
	private volatile HashMap<Short, SJob> m_jobs;
	private volatile HashMap<Short, SJobResult> m_results;
	private Vector<SJobThread> m_threads;
	private short m_id;

	public JobMapper() {
		m_jobs = new HashMap<Short, SJob>();
		m_results = new HashMap<Short, SJobResult>();
		m_threads = new Vector<SJobThread>();
		m_id = 0;
	}

	private ISchedulerServer getScheduledServer() {
		short servers = Scheduler.getServers();
		int load = 200;
		ISchedulerServer server = null;
		int picked = -1;

		for (int i = 0; i < servers; i++) {
			try {
				ISchedulerServer serverImpl = ISchedulerServerHelper
						.narrow(Scheduler.getNCRef().resolve_str(
								Constants.SERVER + i));
				String cpu = serverImpl.getCPU();
				int cpuLoad = Integer.parseInt(cpu.substring(0,
						cpu.length() - 1));

				String mem = serverImpl.getMem();
				int memLoad = Integer.parseInt(mem.substring(0,
						mem.length() - 1));

				Logger.info("Server #" + i + " has CPU: " + cpu + ", memory: "
						+ mem);

				if ((cpuLoad + memLoad) < load) {
					load = cpuLoad + memLoad;
					server = serverImpl;
					picked = i;
				}
			} catch (Exception e) {

			}
		}

		Logger.info("[JopMapper] picked up server #" + picked);
		return server;
	}

	@Override
	public void run() {
		try {
			while (true) {
				// Accept incoming requests
				Thread.sleep(Constants.JITTY_TIME);

				synchronized (m_jobs) {
					// Mapping SJobs on servers
					if (m_jobs.size() > 0)
						Logger.info("Available jobs to be executed: " + m_jobs);

					Iterator<Short> it = m_jobs.keySet().iterator();

					while (it.hasNext()) {
						short key = it.next();
						SJob job = m_jobs.get(key);
						SJobThread jobThread = new SJobThread(job, key,
								getScheduledServer());
						m_threads.add(jobThread);
						jobThread.start();
					}
					m_jobs.clear();
				}

				synchronized (m_results) {
					// Gather finished jobs by the servers
					for (SJobThread thread : m_threads) {
						if (thread.finished())
							m_results
									.put(thread.getJobID(), thread.getResult());
					}

					for (SJobThread thread : m_threads) {
						if (thread.finished())
							m_results.remove(thread);
					}
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public short putJob(SJob job) {
		synchronized (m_jobs) {
			m_jobs.put(m_id, job);
			return m_id++;
		}
	}

	public SJobResult getResult(short jobID) {
		synchronized (m_results) {
			if (m_results.containsKey(jobID)) {
				SJobResult result = m_results.get(jobID);
				m_results.remove(jobID);
				return result;
			}
		}
		return null;
	}
}
