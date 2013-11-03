/**
 * Dascalu Laurentiu, 342C3
 * Tema 3 SPRC
 * 
 * Client
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
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.security.KeyStore;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.util.logging.Logger;

import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.TrustManagerFactory;

// helper class
// reads user messages from standard input and sends them on the socket
class MessageReader implements Runnable {
	private Client client;
	
	public final static String LIST_COMMAND = "list";
	public final static String DOWNLOAD_COMMAND = "download";
	public final static String UPLOAD_COMMAND = "upload";
	public final static String QUIT_COMMAND = "quit";

	public final static String CLIENT_START_HEADER = "Enter command to be executed [list/download file/upload file/quit]";
	public final static String CLIENT_STOP_HEADER = "Waiting for command completion";
	
	public MessageReader (Client client) {
		this.client = client;
	}
	
	public void run () {
		Client.logger.info("[" + this.client.getName() + "] " + "Starting stdin input thread");
		BufferedReader inputReader = new BufferedReader(new InputStreamReader(System.in));
		String line = null;
		boolean quit = false;
		
		while (!quit) {
			Client.logger.info("[" + this.client.getName() + "] " + CLIENT_STOP_HEADER);
			try {
				line = inputReader.readLine();	
			} catch (IOException e) {
				Client.logger.info("[" + this.client.getName() + "] An exception was caught while trying to read the message the user has typed; retrying...");
				e.printStackTrace();
				continue;	
			}
			if (line == null) {
				Client.logger.severe("[" + this.client.getName() + "] The end of the input stream has been reached?!");
				return;
			}
			else if (line.equals(QUIT_COMMAND))
			{
				quitHandler();
				quit = true;
			}
			else if (line.equals(LIST_COMMAND))
				listHandler();
			else if (line.startsWith(DOWNLOAD_COMMAND))
				downloadHandler(line);
			else if (line.startsWith(UPLOAD_COMMAND))
				uploadHandler(line);
			else
				Client.logger.info("[" + this.client.getName() + "] Invalid command received: " + line);
			Client.logger.info("[" + this.client.getName() + "] Sent to the server the message: " + line);
		}
	}
	
	private void quitHandler()
	{
		sendCommandToServer(QUIT_COMMAND);
	}
	
	private void listHandler()
	{
		sendCommandToServer(LIST_COMMAND);
	}
	
	private void uploadHandler(String cmd)
	{
		sendCommandToServer(cmd);
		uploadFile(this, cmd.substring(cmd.indexOf(' ') + 1));
	}
	
	private void downloadHandler(String cmd)
	{
		sendCommandToServer(cmd);
	}
	
	private void sendCommandToServer(String command)
	{
		try {
			OutputStream out = this.client.getOutputStream();
			BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(out));
			writer.write(command);
			writer.newLine();
			writer.flush();
		} catch (IOException e) {
			Client.logger.info("[" + this.client.getName() + "] Failed to send the message to the server");
			e.printStackTrace();	
		}
	}
	
	public Client getClient()
	{
		return client;
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
	
	public static void uploadFile(MessageReader reader, String filename)
	{
		System.out.println("[Client] uploading file " + filename);
		try {
			OutputStream out = reader.getClient().getOutputStream();
			BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(out));
			
			// Scriu numele fisierului
			writer.write(filename);
			writer.newLine();
			
			char []cbuf = loadFile(filename);
			writer.write(cbuf.length);
			writer.write(cbuf, 0, cbuf.length);
			writer.flush();
		} catch (IOException e)
		{
			e.printStackTrace();	
		}
	}
	
	public static void downloadFile(Client client)
	{
		try {
			String filename = client.getReader().readLine();
			System.out.println("[Client] downloading file " + filename);
			int len = client.getReader().read();
			char []c = new char[len];
			
			client.getReader().read(c, 0, len);
			
			RandomAccessFile fout = new RandomAccessFile("downloads/" + filename, "rw");
			fout.writeBytes(new String(c));
			fout.close();
		} catch (Exception e)
		{
			e.printStackTrace();
		}
	}
	
	public static void downloadFileList(Client client)
	{
		try {
			int size = client.getReader().read();
			
			System.out.println("There are available %" + size + "% files for download");
			for (int i = 0 ; i < size ; i++)
			{
				System.out.println("File #" + i);
				System.out.println("\tNume Fisier: " +
						client.getReader().readLine()); 
				System.out.println("\tProprietar: " +
						client.getReader().readLine()); 
				System.out.println("\tDepartament: " +
						client.getReader().readLine()); 
			}
		} catch (Exception e)
		{
			e.printStackTrace();
		}
	}
}

public class Client implements Runnable {
	
	static Logger logger = Logger.getLogger(Client.class.getName());
	
	// the name of this client
	private String name;
	
	// the department this client is part of
	private String department;
	
	// the hostname where the server we're trying to connect to resides
	private String hostname;
	
	// the port on which the server we're trying to connect to listens for incoming connections
	private int port;
	
	// the name of this client's keystore
	private String keyStoreName;
	
	// the password to access this server's keystore
	private String password;
	
	private KeyStore clientKeyStore = null;
	private KeyManagerFactory keyManagerFactory = null;
	private TrustManagerFactory trustManagerFactory = null;
	private SSLContext sslContext = null;
	private SSLSocket socket = null;
	private Certificate CACertificate = null;
	
	private InputStream in;
	private OutputStream out;
	private BufferedReader reader;
	
	public BufferedReader getReader()
	{
		return reader;
	}
	
	public Client (String name, String department, String hostname, int port) {
		this.name = name;
		this.department = department;
		this.hostname = hostname;
		this.port = port;
		this.keyStoreName = "security/" + name + "/" + name + ".ks";
		this.password = name + "_password";
	}
	
	public String getName () {
		return this.name;
	}
	
	public OutputStream getOutputStream () {
		return this.out;
	}
	
	private int initialize () {
		try {
			// get a reference to this client's keystore
			this.clientKeyStore = KeyStore.getInstance("JKS");
			this.clientKeyStore.load(new FileInputStream(this.keyStoreName), password.toCharArray());
		} catch (Exception e) {
			logger.severe("[" + this.name + "] Failed to get a reference to this client's keystore: keyStoreName = " + this.keyStoreName + ", password = " + this.password);
			e.printStackTrace();
			return 1;
		}
		
		logger.info("[" + this.name + "] Successfully obtained a reference to the client's keystore");
		
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
			this.keyManagerFactory.init(this.clientKeyStore, this.password.toCharArray());
		} catch (Exception e) {
			logger.severe("[" + this.name + "] Failed to initialize the key manager factory with the key material coming from the client's keystore");
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
			this.trustManagerFactory.init(this.clientKeyStore);
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
			// get the certificate of this client's certification authority
			this.CACertificate = this.clientKeyStore.getCertificate("certification_authority");
		} catch (Exception e) {
			logger.severe("[" + this.name + "] Failed to retrive the certificate of this client's certificate authority");
			e.printStackTrace();
			return 1;
		}
		
		logger.info("[" + this.name + "] Create a separate thread to read from the standard input");
		(new Thread(new MessageReader(this))).start();
		
		return 0;
	}
	
	public void run () {
		if (initialize() != 0) {
			return;
		}
		try {
			this.socket = (SSLSocket) sslContext.getSocketFactory().createSocket(hostname, port);
		} catch (Exception e) {
			logger.severe("[" + this.name + "] Failed to create a SSL socket");
			e.printStackTrace();
			return;	
		}
		logger.info("[" + this.name + "] Successfully created a SSL socket");
		try {
			this.socket.startHandshake();	
		} catch (IOException e) {
			logger.severe("[" + this.name + "] Failed to complete the handshake");
			e.printStackTrace();
			return;
		}
		logger.info("[" + this.name + "] Successful handshake");
		X509Certificate[] peerCertificates = null;
		try {
			peerCertificates = (X509Certificate[])((this.socket).getSession()).getPeerCertificates();
			if (peerCertificates.length < 2) {
				logger.severe("'peerCertificates' should contain at least two certificates");
				return;
			}
		} catch (Exception e) {
			logger.severe("[" + this.name + "] Failed to get peer certificates");
			e.printStackTrace();
			return;
		}
		if (CACertificate.equals(peerCertificates[1])) {
			logger.info("[" + this.name + "] The peer's CA certificate matches this client's CA certificate");
		}
		else {
			logger.severe("[" + this.name + "] The peer's CA certificate doesn't match this client's CA certificate");
			return;
		}
		
		
		try {
			in = socket.getInputStream();
			reader = new BufferedReader(new InputStreamReader(in));
			out = socket.getOutputStream();
		} catch (Exception e) {
			logger.severe("Failed to properly handle the communication with the server");
			e.printStackTrace();
			return;
		}

		logger.info("[" + this.name + "] " + MessageReader.CLIENT_STOP_HEADER);
		String line = null;
		while (true) {
			try {
				line = reader.readLine();
				System.out.println("Server response:\n\t" + line);
				if (line == null) {
					logger.severe("The end of the input stream has been reached?!");
					try {
						socket.close();	
					} catch (IOException e) {
						logger.severe("Failed to properly close the socket");	
					}
					return;	
				}
				else if (line.compareTo("DENIED") == 0) {
					logger.info("This client is not allowed to connect to the chosen server");
					System.out.println("This client is not allowed to use the chosen server. Exiting...");
					try {
						socket.close();	
					} catch (IOException e) {
						logger.severe("Failed to properly close the socket");	
					}
					return;	
				}
				else if (line.equals(MessageReader.QUIT_COMMAND))
				{
					System.out.println("Client is going down....");
					break;
				} else if (line.startsWith(MessageReader.DOWNLOAD_COMMAND))
					MessageReader.downloadFile(this);
				else if (line.equals(MessageReader.LIST_COMMAND))
					MessageReader.downloadFileList(this);
			} catch (IOException e) {
				logger.info("[" + this.name + "] An exception was caught while trying to read a message from the server");
				e.printStackTrace();
				try {
					socket.close();
				} catch (IOException ioe) {
					logger.severe("Failed to close the socket");
					ioe.printStackTrace();
				}
			}
		}
	}
	
	public static void main(String args[]) throws Exception {
		if (args.length != 4) {
			System.err.println("Usage: java Client client_name client_department hostname port");
			System.exit(1);
		}
		
		Runtime.getRuntime().exec("mkdir -p downloads/").waitFor();
		(new Thread(new Client(args[0], args[1], args[2], Integer.parseInt(args[3])))).start();
	}
}