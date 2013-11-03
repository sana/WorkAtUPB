# Dascalu Laurentiu, 342C3
# Tema 4 SPRC, Serviciu de planificare bazat pe CORBA

PORT=15000
classpath="bin/"

all:
	cd src ; idlj -fall IScheduler.idl
	ant

# Simple loaders
start_ns:
	tnameserv -ORBInitialPort ${PORT}

# Server on which jobs are executed
run_server:
	java -cp ${classpath} Scheduler.Server -ORBInitialPort ${PORT} -ORBInitialHost localhost

# Client which submit jobs
run_client:
	java -cp ${classpath} Scheduler.Client -ORBInitialPort ${PORT} -ORBInitialHost localhost

# Scheduler which maps jobs on servers
run_admin:
	java -cp ${classpath} Scheduler.Scheduler -ORBInitialPort ${PORT} -ORBInitialHost localhost

# Debug tester
run_debugger:
	# Starts the nameserver and the debugger
	tnameserv -ORBInitialPort ${PORT} &
	java -cp ${classpath} Scheduler.Debugger -ORBInitialPort ${PORT} -ORBInitialHost localhost

clean:
	rm -rf src/ISchedulerCORBA

