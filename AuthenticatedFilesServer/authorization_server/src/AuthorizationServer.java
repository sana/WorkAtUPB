/**
 * Dascalu Laurentiu, 342C3
 * Tema 3 SPRC
 * 
 * Server de autorizare a accesului clientilor
 * la resursele serverului
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
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.security.KeyStore;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
/**
 * Copyright (C) 2008 by Politehnica University of Bucharest
 * All rights reserved.
 * Refer to LICENSE for terms and conditions of use.
 */
import java.util.Date;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.StringTokenizer;
import java.util.Timer;
import java.util.TimerTask;
import java.util.logging.Logger;

import javax.crypto.Cipher;
import javax.crypto.CipherInputStream;
import javax.crypto.CipherOutputStream;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLServerSocket;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.TrustManagerFactory;

// helper class for crypting and decrypting a file
class DESEncrypter {
	private Cipher encryptionCipher;
	private Cipher decryptionCipher;
	
	public DESEncrypter (SecretKey key) throws Exception {
		encryptionCipher = Cipher.getInstance("DES");
		encryptionCipher.init(Cipher.ENCRYPT_MODE, key);
		decryptionCipher = Cipher.getInstance("DES");
		decryptionCipher.init(Cipher.DECRYPT_MODE, key);
	}
	
	// write to 'out' the encryption of the information read from 'in'
	public boolean encrypt (InputStream in, OutputStream out) {
		byte[] buffer = new byte[1024];
		try {
			OutputStream cout = new CipherOutputStream(out, encryptionCipher);
			int bytesRead = 0;
			while ((bytesRead = in.read(buffer)) >= 0) {
				cout.write(buffer, 0, bytesRead);
			}
			cout.close();
			in.close();
			out.close();
		} catch (IOException e) {
			System.err.println("Failed to encrypt the information");
			e.printStackTrace();
			return false;
		}
		return true;
	}
	
	// write to 'out' the information obtained by decrypting the information read from 'in'
	public boolean decrypt (InputStream in, OutputStream out) {
		byte[] buffer = new byte[1024];
		try {
			CipherInputStream cin = new CipherInputStream(in, decryptionCipher);
			int bytesRead = 0;
			while ((bytesRead = cin.read(buffer)) >= 0) {
				out.write(buffer, 0, bytesRead);
			}
			cin.close();
			in.close();
			out.close();
		} catch (IOException e) {
			System.err.println("Failed to decrypt the information");
			e.printStackTrace();
			return false;
		}
		return true;
	}
}

class BannedTimerTask extends TimerTask {
	private AuthorizationServer server = null;
	private String clientName = null;
	
	public BannedTimerTask (AuthorizationServer server, String clientName) {
		this.server = server;
		this.clientName = clientName;	
	}
	
	public void run () {
		byte[] buffer = new byte[1024];
		this.server.logger.info("REMOVING THE BAN FOR CLIENT '" + this.clientName);
		try {
			if (!this.server.crypt.decrypt(new FileInputStream("banned_encrypted"), new FileOutputStream("banned_decrypted"))) {
				return;
			}
			FileInputStream in = new FileInputStream("banned_decrypted");
			in.read(buffer);
			in.close();
			String info = new String(buffer);
			// delete 'clientName' from the list of the banned clients
			info.replace(clientName, "");
			// delete the old encryption
			File file = new File("banned_encrypted");
			file.delete();
			// encrypt the new information
			File newEncryption = new File("banned_encrypted");
			if (newEncryption.createNewFile() == false) {
				this.server.logger.severe("Failed to re-create the 'banned_encrypted' file when trying to remove the banning of the client '" + clientName + "'");
				return;	
			}
			FileOutputStream out = new FileOutputStream("banned_encrypted");
			out.write(info.getBytes());
			out.close();
			
			if (!this.server.crypt.encrypt(new FileInputStream("banned_decrypted"), new FileOutputStream(newEncryption))) {
				this.server.logger.severe("Failed to ban the client '" + clientName + "'");
				return;
			}
		} catch (Exception e) {
			this.server.logger.warning("An exception was caught while trying to remove '" + clientName + "' from the banned list");
			e.printStackTrace();
			return;
		}
		// delete the decryption
		File temp = new File("banned_decrypted");
		temp.delete();
		return;
	}
}


public class AuthorizationServer implements Runnable {
	static Logger logger = Logger.getLogger(AuthorizationServer.class.getName());
	
	// the name of this server's keystore
	private String keyStoreName;
	// the password to access this server's keystore
	private String password;
	private Map<String, Integer> priorities;
	
	private KeyStore serverKeyStore = null;
	private KeyManagerFactory keyManagerFactory = null;
	private TrustManagerFactory trustManagerFactory = null;
	private SSLContext sslContext = null;
	private SSLServerSocket serverSocket = null;
	private Certificate CACertificate = null;
	// key used for encrypting / decrypting the file containing information about the banned clients
	private SecretKey key = null;
	DESEncrypter crypt = null;
	
	public AuthorizationServer () {
		this.keyStoreName = "security/authorization_server.ks";
		this.password = "authorization_server_password";
		this.priorities = new LinkedHashMap<String, Integer>();
		this.priorities.put("HUMAN_RESOURCES", 1);
		this.priorities.put("ACCOUNTING", 1);
		this.priorities.put("IT", 2);
		this.priorities.put("MANAGEMENT", 3);
	}
	
	private int initialize () {
		try {
			// get a reference to the authorization server's keystore
			this.serverKeyStore = KeyStore.getInstance("JKS");
			this.serverKeyStore.load(new FileInputStream(this.keyStoreName), password.toCharArray());
		} catch (Exception e) {
			logger.severe("Failed to get a reference to the authorization server's keystore: keyStoreName = " + this.keyStoreName + ", password = " + this.password);
			e.printStackTrace();
			return 1;
		}
		logger.info("Successfully obtained a reference to the authorization server's keystore");
		
		try {
			// get a key manager factory
			this.keyManagerFactory = KeyManagerFactory.getInstance("SunX509");
		} catch (Exception e) {
			logger.severe("Failed to obtain a key manager factory");
			e.printStackTrace();
			return 1;
		}
		logger.info("Successfully obtained a key manager factory");
		
		try {
			// initialize the key manager factory with a source of key material
			this.keyManagerFactory.init(this.serverKeyStore, this.password.toCharArray());
		} catch (Exception e) {
			logger.severe("Failed to initialize the key manager factory with the key material coming from the server's keystore");
			e.printStackTrace();
			return 1;
		}
		logger.info("Successfully initialized the key manager factory");
		
		try {
			// get a trust manager factory
			this.trustManagerFactory = TrustManagerFactory.getInstance("SunX509");
		} catch (Exception e) {
			logger.severe("Failed to obtain a trust manager factory");
			e.printStackTrace();
			return 1;
		}
		logger.info("Successfully obtained a trust manager factory");
		
		try {
			// initialize the trust manager factory with a source of key material
			this.trustManagerFactory.init(this.serverKeyStore);
		} catch (Exception e) {
			logger.severe("Failed to initialize the trust manager factory with the key material coming from the server's keystore");
			e.printStackTrace();
			return 1;
		}
		logger.info("Successfully initialized the trust manager factory");
		
		try {
			// set the SSL context
			this.sslContext = SSLContext.getInstance("TLS");
			this.sslContext.init(this.keyManagerFactory.getKeyManagers(), this.trustManagerFactory.getTrustManagers(), null);
		} catch (Exception e) {
			logger.severe("Failed to initialize the SSl context");
			e.printStackTrace();
			return 1;
		}
		logger.info("Successfully initialized a SSL context");
		
		try {
			// get the certificate of this server's certification authority
			this.CACertificate = this.serverKeyStore.getCertificate("certification_authority");
		} catch (Exception e) {
			logger.severe("Failed to retrive the certificate of this server's certificate authority");
			e.printStackTrace();
			return 1;
		}
		try {
			// generate a cryptographic key
			key = KeyGenerator.getInstance("DES").generateKey();
		} catch (Exception e) {
			logger.severe("Failed to generate the cryptographic key");
			e.printStackTrace();
			return 1;
		}
		try {
			// obtain a DES encrypter
			crypt = new DESEncrypter(key);
		} catch (Exception e) {
			logger.severe("Failed to create a DESEncrypter");
			e.printStackTrace();
			return 1;
		}
		
		return 0;
	}
	
	// returns 'true' if a client is in the 'banned list'
	private boolean isBanned (String clientName) {
		byte[] buffer = new byte[1024];
		boolean result;
		try {
			if (!crypt.decrypt(new FileInputStream("banned_encrypted"), new FileOutputStream("banned_decrypted"))) {
				return false;	
			}
			FileInputStream in = new FileInputStream("banned_decrypted");
			in.read(buffer);
			in.close();
			String info = new String(buffer);
			if (info.indexOf(clientName) == -1) {
				result = false;	
			}
			else {
				result = true;	
			}
		} catch (Exception e) {
			logger.severe("Failed to find out if client '" + clientName + "' is banned or not");
			e.printStackTrace();
			return false;	
		}
		// delete the decryption
		File temp = new File("banned_decrypted");
		temp.delete();
		return result;
	}
	
	// adds a client into the 'banned list'
	private void ban (String clientName) {
		try {
			if (!crypt.decrypt(new FileInputStream("banned_encrypted"), new FileOutputStream("banned_decrypted"))) {
				return;	
			}
			// add this client to the list of those banned
			FileOutputStream out = new FileOutputStream("banned_decrypted", true);
			String line = clientName + " " + System.currentTimeMillis() + "\n";
			out.write(line.getBytes());
			out.close();
			// delete the old encryption
			File file = new File("banned_encrypted");
			file.delete();
			// encrypt the new information
			File newEncryption = new File("banned_encrypted");
			if (newEncryption.createNewFile() == false) {
				logger.severe("Failed to re-create the 'banned_encrypted' file when banning the client '" + clientName + "'");
				return;	
			}
			if (!crypt.encrypt(new FileInputStream("banned_decrypted"), new FileOutputStream("banned_encrypted"))) {
				logger.severe("Failed to ban the client '" + clientName + "'");
				return;
			}
			// set the timer so that the client is allowed back in 15s
			int after = 15000; // 15s
			Date timeToRun = new Date(System.currentTimeMillis() + after);
			Timer timer = new Timer();
			timer.schedule(new BannedTimerTask(this, clientName), timeToRun);
			
		} catch (Exception e) {
			logger.severe("Failed to ban the client '" + clientName + "'");
			e.printStackTrace();
			return;
		}
		// delete the decryption
		File temp = new File("banned_decrypted");
		temp.delete();
		return;
	}
	
	// process a request received from a department server
	private String processRequest (String request) {
		StringTokenizer st = new StringTokenizer(request, " ");
		if (st.countTokens() != 3) {
			logger.severe("The request doesn't respect the agreed format: <" + request + ">");
			return "ERROR";
		}
		int count = 0;
		String serverDepartment = null, clientName = null, clientDepartment = null;
		while (st.hasMoreTokens()) {
			String token = st.nextToken();
			count++;
			if (count == 1) {
				serverDepartment = token;
			}
			if (count == 2) {
				clientName = token;	
			}
			if (count == 3) {
				clientDepartment = token;	
			}
		}
		if (serverDepartment.compareTo("BAN") == 0) {
			logger.info("A BAN request has been received");
			ban(clientName);
			return "OK";	
		}
		Integer serverPriority = this.priorities.get(serverDepartment);
		Integer clientPriority = this.priorities.get(clientDepartment);
		if (serverPriority == null || clientPriority == null) {
			logger.severe("The priorities list doesn't contain the priorities for the server or client's department");
			return "ERROR";
		}
		if ((clientPriority.intValue() >= serverPriority.intValue()) && (isBanned(clientName) == false)) {
			return "ALLOWED";	
		}
		else {
			return "DENIED";
		}
	}
	
	public void run () {
		if (initialize() != 0) {
			return;
		}
		try {
			this.serverSocket = (SSLServerSocket) sslContext.getServerSocketFactory().createServerSocket(7000);
		} catch (Exception e) {
			logger.severe("Failed to create a SSL server socket");
			e.printStackTrace();
			return;
		}
		logger.info("Successfully created a SSL server socket");
		
		serverSocket.setNeedClientAuth(true);
		logger.info("Listening for incoming connections... ");
		
		while (true) {
			SSLSocket socket = null;
			try {
				socket = (SSLSocket) this.serverSocket.accept();
			} catch (Exception e) {
				logger.severe("Failed to accept an incoming connection");
				e.printStackTrace();
				return;	
			}
			logger.info("Just accepted a connection");
			try {
				socket.startHandshake();
			} catch (IOException e) {
				logger.severe("Failed to complete a handshake");
				e.printStackTrace();
				continue;	
			}
			logger.info("Successful handshake");
			X509Certificate[] peerCertificates = null;
			try {
				peerCertificates = (X509Certificate[])(socket.getSession()).getPeerCertificates();
				if (peerCertificates.length < 2) {
					logger.severe("'peerCertificates' should contain at least two certificates");
					continue;
				}
			} catch (Exception e) {
				logger.severe("Failed to get peer certificates");
				e.printStackTrace();
				continue;
			}
				
			if (CACertificate.equals(peerCertificates[1])) {
				logger.info("The peer's CA certificate matches the authorization server's CA certificate");
			}
			else {
				logger.severe("The peer's CA certificate doesn't match the authorization server's CA certificate");
				continue;
			}
			BufferedReader reader = null;
			BufferedWriter writer = null;
			try {
				reader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
				writer = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));	
			} catch (Exception e) {
				logger.severe("Failed to properly handle the communication with a peer");
				e.printStackTrace();
				continue;
			}
			String request = null;
			try {
				request = reader.readLine();	
			} catch (IOException e) {
				logger.severe("Failed to read a message from the socket");
				e.printStackTrace();
				try {
					socket.close();
				} catch (IOException ioe) {
					logger.severe("Failed to close the socket");
					ioe.printStackTrace();	
				}
				continue;
			}
			if (request == null) {
				logger.severe("The end of the input stream has been reached");
				try {
					socket.close();
				} catch (IOException ioe) {
					logger.severe("Failed to close the socket");
					ioe.printStackTrace();	
				}
				continue;
			}
			System.out.println("Received request: <" + request + ">");
			String response = processRequest(request);
			try {
				writer.write(response);
				writer.newLine();
				writer.flush();	
			} catch (IOException e) {
				logger.severe("Failed to write a message to the socket");
				e.printStackTrace();
				try {
					socket.close();
				} catch (IOException ioe) {
					logger.severe("Failed to close the socket");
					ioe.printStackTrace();	
				}
				continue;
			}
		}
	}
	
	public static void main (String args[]) {
		(new Thread(new AuthorizationServer())).start();
	}
}
