/**
 * Dascalu Laurentiu, 342C3
 * Tema 3 SPRC
 * 
 * Server
 */

/**
	Sisteme de programe pentru retele de calculatoare
	
	Copyright (C) 2008 Ciprian Dobre & Florin Pop
	Univerity Politehnica of Bucharest, Romania

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 */

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.RandomAccessFile;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;
import java.security.KeyStore;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.Set;
import java.util.StringTokenizer;
import java.util.logging.Logger;

import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLServerSocket;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.TrustManagerFactory;
import java.util.Vector;
import java.io.Serializable;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.io.FileOutputStream;
import java.io.FileInputStream;

class ListOpRecord implements Serializable
{
	public final static long serialVersionUID = 0; 
	public String numeFisier;
	public String idProprietar;
	public String departament;
	
	public String toString()
	{
		return numeFisier + " " + idProprietar + " " + departament;
	}
	
	private final static String UPLOAD_LIST = "__upload_list.bin";
	
	public static void serialize(Vector<ListOpRecord> data)
	{
		try {
			FileOutputStream fos = new FileOutputStream(UPLOAD_LIST);
			ObjectOutputStream oos = new ObjectOutputStream(fos);
			oos.writeInt(data.size());
			for (int i = 0 ; i < data.size() ; i++)
				oos.writeObject(data.elementAt(i));
			oos.close();
		} catch(Exception e)
		{
			e.printStackTrace();
		}
	}
	
	public static Vector<ListOpRecord> deserialize()
	{
		Vector<ListOpRecord> data = new Vector<ListOpRecord>();
		try {
			FileInputStream fos = new FileInputStream(UPLOAD_LIST);
			ObjectInputStream ois = new ObjectInputStream(fos);
			int size = ois.readInt();
			for (int i = 0 ; i < size ; i++)
				data.add((ListOpRecord) ois.readObject());
			ois.close();
	
			return data;
		} catch(Exception e)
		{
			//e.printStackTrace();
			return data;
		}
	}
}

public class Server implements Runnable {
	private static Logger logger = Logger.getLogger(Server.class.getName());
	
	// the port on which the authorization server is waiting for incoming connections
	private static final int authorizationServerPort = 7000;
	
	// the name of the department this server is managing chat service for
	private String name;
	
	// the port on which this server is listening for incoming connections
	private int port;
	
	// the hostname of the authorization server
	private String hostname;
	
	// the name of this server's keystore
	private String keyStoreName;
	
	// the password to access this server's keystore
	private String password;
	
	private KeyStore serverKeyStore = null;
	private KeyManagerFactory keyManagerFactory = null;
	private TrustManagerFactory trustManagerFactory = null;
	private SSLContext sslContext = null;
	private SSLServerSocket serverSocket = null;
	private Certificate CACertificate = null;
	
	private Set<ConnectionHandler> connections = null;
	
	// Lista de fisiere uploadate in sistem
	private Vector<ListOpRecord> uploadedFiles;
	
	public Server (String name, int port, String hostname) {
		this.name = name;
		this.port = port;
		this.hostname = hostname;
		this.keyStoreName = "security/" + name + "/" + name + ".ks";
		this.password = name + "_password";
		this.connections = new LinkedHashSet<ConnectionHandler>();
		this.uploadedFiles = ListOpRecord.deserialize();
	}
	
	public Vector<ListOpRecord> getUploadList()
	{
		return uploadedFiles;
	}
	
	public void serializeUploadList()
	{
		ListOpRecord.serialize(this.uploadedFiles);
	}
	
	public void putFileRecord(ListOpRecord record)
	{
		this.uploadedFiles.add(record);
	}
	
	public void debugUpload()
	{
		System.out.println(uploadedFiles);
	}
	
	private int initialize () {
		try {
			// get a reference to this server's keystore
			this.serverKeyStore = KeyStore.getInstance("JKS");
			this.serverKeyStore.load(new FileInputStream(this.keyStoreName), password.toCharArray());
		} catch (Exception e) {
			logger.severe("[" + this.name + "] Failed to get a reference to this server's keystore: keyStoreName = " + this.keyStoreName + ", password = " + this.password);
			e.printStackTrace();
			return 1;
		}
		logger.info("[" + this.name + "] Successfully obtained a reference to the server's keystore");
		
		try {
			// get a key manager factory
			this.keyManagerFactory = KeyManagerFactory.getInstance("SunX509");
		} catch (Exception e) {
			logger.severe("[" + this.name + "] Failed to obtain a key manager factory");
			e.printStackTrace();
			return 1;
		}
		logger.info("[" + this.name + "] Successfully obtained a key manager factory");
		
		try {
			// initialize the key manager factory with a source of key material
			this.keyManagerFactory.init(this.serverKeyStore, this.password.toCharArray());
		} catch (Exception e) {
			logger.severe("[" + this.name + "] Failed to initialize the key manager factory with the key material coming from the server's keystore");
			e.printStackTrace();
			return 1;
		}
		logger.info("[" + this.name + "] Successfully initialized the key manager factory");
		
		try {
			// get a trust manager factory
			this.trustManagerFactory = TrustManagerFactory.getInstance("SunX509");
		} catch (Exception e) {
			logger.severe("[" + this.name + "] Failed to obtain a trust manager factory");
			e.printStackTrace();
			return 1;
		}
		logger.info("[" + this.name + "] Successfully obtained a trust manager factory");
		
		try {
			// initialize the trust manager factory with a source of key material
			this.trustManagerFactory.init(this.serverKeyStore);
		} catch (Exception e) {
			logger.severe("[" + this.name + "] Failed to initialize the trust manager factory with the key material coming from the server's keystore");
			e.printStackTrace();
			return 1;
		}
		logger.info("[" + this.name + "] Successfully initialized the trust manager factory");
		
		try {
			// set the SSL context
			this.sslContext = SSLContext.getInstance("TLS");
			this.sslContext.init(this.keyManagerFactory.getKeyManagers(), this.trustManagerFactory.getTrustManagers(), null);
		} catch (Exception e) {
			logger.severe("[" + this.name + "] Failed to initialize the SSl context");
			e.printStackTrace();
			return 1;
		}
		logger.info("[" + this.name + "] Successfully initialized a SSL context");
		
		try {
			// get the certificate of this server's certification authority
			this.CACertificate = this.serverKeyStore.getCertificate("certification_authority");
		} catch (Exception e) {
			logger.severe("[" + this.name + "] Failed to retrive the certificate of this server's certificate authority");
			e.printStackTrace();
			return 1;
		}
		return 0;
	}
	
	// send a message to the authorization server and wait for a response
	String communicateWithAuth (String message) {
		// connect to the authorization server
		SSLSocket auth = null;
		try {
			auth = (SSLSocket) sslContext.getSocketFactory().createSocket(this.hostname, Server.authorizationServerPort);
		} catch (Exception e) {
			logger.severe("[" + this.name + "] Failed to create a SSL socket to communicate with the authentication server");
			e.printStackTrace();
			return null;
		}
		logger.info("[" + this.name + "] Successfully created a SSL socket to communicate with the authentication server");
		BufferedReader authReader = null;
		BufferedWriter authWriter = null;
		// send the information about the client to the authentication server
		try {
			authReader = new BufferedReader(new InputStreamReader(auth.getInputStream()));
			authWriter = new BufferedWriter(new OutputStreamWriter(auth.getOutputStream()));
		} catch (Exception e) {
			logger.severe("[" + this.name + "] Failed to properly handle the communication with the authentication server");
			e.printStackTrace();
			return null;
		}
		
		try {
			authWriter.write(message);
			authWriter.newLine();
			authWriter.flush();
		} catch (IOException e) {
			logger.severe("[" + this.name + "] Failed to send message to the authentication server");
			e.printStackTrace();
			try {
				auth.close();
			} catch (IOException ioe) {
				logger.severe("[" + this.name + "] Failed to close the socket");
				ioe.printStackTrace();	
			}
			return null;
		}
		// wait for the response from the authentication server
		String response = null;
		try {
			response = authReader.readLine();
		} catch (IOException e) {
			logger.severe("[" + this.name + "] Failed to read the response the authentication server has sent");
			e.printStackTrace();
			try {
				auth.close();
			} catch (IOException ioe) {
				logger.severe("[" + this.name + "] Failed to close the socket");
				ioe.printStackTrace();	
			}
			return null;
		}
		if (response == null) {
			try {
				auth.close();
			} catch (IOException ioe) {
				logger.severe("[" + this.name + "] Failed to close the socket");
				ioe.printStackTrace();	
			}
		}
		
		return response;
	}
	
	// say goodbye to a client
	void goodbyeClient (Socket client) {
		BufferedWriter writer = null;
		try {
			writer = new BufferedWriter(new OutputStreamWriter(client.getOutputStream()));	
		} catch (Exception e) {
			logger.severe("[" + this.name + "] Failed to say goodbye to a client");
			e.printStackTrace();
			try {
				client.close();	
			} catch (IOException ioe) {
				logger.severe("[" + this.name + "] Failed to close the socket");
				ioe.printStackTrace();
				return;
			}
		}
		try {
			writer.write("DENIED");
			writer.newLine();
			writer.flush();	
		} catch (IOException e) {
			logger.severe("[" + this.name + "] Failed to say goodbye to a client");
			e.printStackTrace();
		}
		try {
			client.close();	
		} catch (IOException e) {
			logger.severe("[" + this.name + "] Failed to close the socket");
			e.printStackTrace();
		}
	}
	
	public void run () {
		if (initialize() != 0) {
			return;
		}
		try {
			this.serverSocket = (SSLServerSocket) sslContext.getServerSocketFactory().createServerSocket(this.port);
		} catch (Exception e) {
			logger.severe("[" + this.name + "] Failed to create a SSL server socket");
			e.printStackTrace();
			return;
		}
		logger.info("[" + this.name + "] Successfully created a SSL server socket");
		
		serverSocket.setNeedClientAuth(true);
		logger.info("[" + this.name + "] Listening for incoming connections... ");
		while (true) {
			try {
				Socket client = serverSocket.accept();
				logger.info("Just accepted a connection");
				try {
					((SSLSocket) client).startHandshake();
				} catch (IOException e) {
					logger.severe("[" + this.name + "] Failed to complete a handshake");
					e.printStackTrace();
					continue;	
				}
				logger.info("[" + this.name + "] Successful handshake");
				X509Certificate[] peerCertificates = null;
				try {
					peerCertificates = (X509Certificate[])(((SSLSocket) client).getSession()).getPeerCertificates();
					if (peerCertificates.length < 2) {
						logger.severe("'peerCertificates' should contain at least two certificates");
						continue;
					}
				} catch (Exception e) {
					logger.severe("[" + this.name + "] Failed to get peer certificates");
					e.printStackTrace();
					continue;
				}
				
				if (CACertificate.equals(peerCertificates[1])) {
					logger.info("[" + this.name + "] The peer's CA certificate matches this server's CA certificate");
				}
				else {
					logger.severe("[" + this.name + "] The peer's CA certificate doesn't match this server's CA certificate");
					continue;
				}
				// retrieve the client CN and OU from its certificate
				String clientDN = peerCertificates[0].getSubjectX500Principal().getName();
				StringTokenizer st = new StringTokenizer(clientDN, " \t\r\n,=");
				String clientCN = null;
				String clientOU = null;
				int count = 0;
				while (st.hasMoreTokens()) {
					String token = st.nextToken();
					count++;
					if (count == 2) {
						clientCN = token;
					}
					if (count == 4) {
						clientOU = token;
						break;
					}
				}
				if (clientCN == null || clientOU == null) {
					logger.severe("[" + this.name + "] 'clientCN' or 'clientOU' remained null");
					continue;
				}
				String message = this.name + " " + clientCN + " " + clientOU;
				// the client has proper authorization?
				String response = communicateWithAuth(message);
				if (response == null) {
					logger.severe("[" + this.name + "] Failed to communicate with the authorization server");
					return;
				}
				System.out.println("Response from authorization server for client '" + clientCN + "': " + response);
				if (response.compareTo("DENIED") == 0 || response.compareTo("ERROR") == 0) {
					goodbyeClient(client);
					continue;
				}

				if (connections.add(new ConnectionHandler(this, client, clientCN, clientOU)) == false) {
					logger.info("[" + this.name + "] Failed to add this client's associated ConnectionHandler");	
				}
				else {
					logger.info("[" + this.name + "] Successfully added this client's associated ConnectionHandler");	
				}
			} catch (Exception e) {
				logger.severe("Failed to properly handle the communication with a client");
				e.printStackTrace();
				continue;
			}
		}
	}
	
	public Iterator<ConnectionHandler> getConnectionsIterator () {
		return connections.iterator();
	}
	
	public void removeConnection (ConnectionHandler connectionHandler) {
		connections.remove(connectionHandler);
	}
	
	public static void main (String args[]) throws Exception {
		if (args.length != 3) {
			System.err.println("Usage: java Server department_name port authentication_server_hostname");
			System.exit(1);
		}

		Runtime.getRuntime().exec("mkdir -p downloads/").waitFor();
		(new Thread(new Server(args[0], Integer.parseInt(args[1]), args[2]))).start();
	}
}

//helper class
//a 'ConnectionHandler' instance is associated with each client that is connected to a 'Server' instance
//a 'ConnectionHandler' retrieves the messages sent by the associated client, and transmits all the messages the other clients sent to the associated server
class ConnectionHandler implements Runnable {
	private static Logger logger = Logger.getLogger(ConnectionHandler.class.getName());
	
	// the server we're handling the connections for
	private Server server;
	// the client socket we're handling
	private Socket clientSocket;
	// the name of the client
	private String clientName;
	// the name of the department the client is part of
	private String department;
	private BufferedReader reader = null;
	private BufferedWriter writer = null;
	
	private final static String LIST_COMMAND = "list";
	private final static String DOWNLOAD_COMMAND = "download";
	private final static String UPLOAD_COMMAND = "upload";
	public final static String QUIT_COMMAND = "quit";
	
	public ConnectionHandler (Server server, Socket clientSocket, String clientName, String department) {
		this.server = server;
		this.clientSocket = clientSocket;
		this.clientName = clientName;
		this.department = department;
		
		(new Thread(this)).start();
	}
	
	private boolean sendMessage (String line) {
		try {
			if (line.equals(QUIT_COMMAND))
			{
				this.writer.write(line);
				this.writer.newLine();
				this.writer.flush();
			}
			else if (line.equals(LIST_COMMAND))
			{
				this.writer.write(line);
				this.writer.newLine();
				server.debugUpload();
				sendUploadList(server.getUploadList());
				this.writer.flush();
			}
			else if (line.startsWith(UPLOAD_COMMAND))
			{
				String filename = reader.readLine();
				if (!downloadFile(this, filename))
				{
					System.out.println("Client '" + this.clientName + "' will be banned");
					logger.info("Client '" + this.clientName + "' will be banned for 15 s");
					String message = "BAN" + " " + this.clientName + " " + this.department;
					this.server.communicateWithAuth(message);
					this.server.goodbyeClient(this.clientSocket);
					this.server.removeConnection(this);
					return false;
				}

				ListOpRecord record = new ListOpRecord();
				record.numeFisier = filename;
				record.idProprietar = clientName;
				record.departament = department;
				server.putFileRecord(record);
				server.serializeUploadList();
				this.writer.write(line);
				this.writer.newLine();
				this.writer.flush();
			}
			else if (line.startsWith(DOWNLOAD_COMMAND))
			{
				this.writer.write(line);
				this.writer.newLine();
				this.writer.flush();
				uploadFile(this, line.substring(line.indexOf(' ') + 1));
			}
		} catch (IOException e) {
			logger.severe("Failed to send the message <" + line + "> to the associated client");
			return false;
		}
		logger.info("Successfully sent the message <" + line + "> to the associated client");
		return true;
	}
	
	public void run () {
		// wait for messages from the client
		try {
			reader = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
		} catch (IOException e) {
			logger.severe("Failed to create a reader for a client socket");
			e.printStackTrace();
			return;
		}
		logger.info("Successfully created a reader for a client socket");
		
		try {
			writer = new BufferedWriter(new OutputStreamWriter(clientSocket.getOutputStream()));
		} catch (IOException e) {
			logger.severe("Failed to create a writer for a client socket");
			e.printStackTrace();
			return;
		}
		logger.info("Successfully created a writer for a client socket");
		
		String line = null;
		while (true) {
			try {
				line = reader.readLine();
			} catch (IOException e) {
				logger.info("An exception was caught while trying to read a message from the client socket");
				e.printStackTrace();
				break;
			}
			if (line == null) {
				logger.severe("The end of the input stream has been reached?!");
				break;
			}
			logger.info("Preparing to send the message <" + line + ">  to current client");
			if (!sendMessage(line))
				return;
		}
		try {
			this.clientSocket.close();
		} catch (IOException e) {
			logger.info("Failed to close the client socket");
			e.printStackTrace();
		}
		this.server.removeConnection(this);
	}
	
	private static char[] loadFile(String filename)
	{
		try
		{
			RandomAccessFile fin = new RandomAccessFile(filename, "rw");
			byte[] b = new byte[(int) fin.length()];
			char[] c = new char[(int) fin.length()];
			fin.readFully(b);
			
			for (int i = 0 ; i < b.length ; i++)
				c[i] = (char) b[i];
			
			return c;
		} catch(Exception e)
		{
			e.printStackTrace();
			return null;
		}
	}
	
	public BufferedReader getReader()
	{
		return reader;
	}
	
	public BufferedWriter getWriter()
	{
		return writer;
	}
	
	public static void uploadFile(ConnectionHandler conn, String filename)
	{
		System.out.println("[Server] uploading file " + filename);
		try {
			// Scriu numele fisierului
			conn.getWriter().write(filename);
			conn.getWriter().newLine();
			
			char []cbuf = loadFile("downloads/" + filename);
			conn.getWriter().write(cbuf.length);
			conn.getWriter().write(cbuf, 0, cbuf.length);
			conn.getWriter().flush();
		} catch (IOException e) {
			e.printStackTrace();	
		}		
	}
	
	private final static String []ILLEGAL_STRINGS = {"greva", "bomba"};
	
	public static boolean downloadFile(ConnectionHandler conn, String filename)
	{
		System.out.println("[Server] downloading file " + filename);
		try {
			int len = conn.getReader().read();
			char []c = new char[len];
			
			conn.getReader().read(c, 0, len);
			String str = new String(c);
			
			for (int i = 0 ; i < ILLEGAL_STRINGS.length ; i++)
				if (str.indexOf(ILLEGAL_STRINGS[i]) != -1)
					return false;
			
			RandomAccessFile fout = new RandomAccessFile("downloads/" + filename, "rw");
			fout.writeBytes(str);
			fout.close();
		} catch (Exception e)
		{
			e.printStackTrace();
			return false;
		}
		return true;
	}
	
	private void sendUploadList(Vector<ListOpRecord> uploadList)
	{
		try {
			int size = uploadList.size();
			
			// Scriu numarul de inregistrari
			// si apoi fiecare inregistrare in parte
			getWriter().write(size);
		
			for (ListOpRecord record : uploadList)
			{
				getWriter().write(record.numeFisier,
						0, record.numeFisier.length());
				getWriter().newLine();
				getWriter().write(record.idProprietar,
						0, record.idProprietar.length());
				getWriter().newLine();
				getWriter().write(record.departament,
						0, record.departament.length());
				getWriter().newLine();
			}
		} catch (Exception e)
		{
			e.printStackTrace();
		}
	}
}
