#!/bin/sh

clients=$(ls client/security)
servers=$(ls server/security)
for c in $clients; do
	for s in $servers; do
		C_alias=$c"_private"
		S_alias=$s"_private"
		C_certificate=client/security/$c/$c".crt"
		S_certificate=server/security/$s/$s".crt"
		C_ks=client/security/$c/$c".ks"
		S_ks=server/security/$s/$s".ks"
		C_pass=$c"_password"
		S_pass=$s"_password"
		# import the signed certificate of the client into the department server's keystore
		echo "------------------------------------------------------------------------------------------------------"
		echo "IMPORT the SIGNED certificate of the client '$c' into the '$s' department server's keystore"
		echo "------------------------------------------------------------------------------------------------------"
		keytool -import -alias ${C_alias} -keypass ${S_pass} -keystore ${S_ks} -storepass ${S_pass} -trustcacerts -file ${C_certificate}
		
		# import the signed certificate of the department server into the client's keystore
		echo "------------------------------------------------------------------------------------------------------"
		echo "IMPORT the SIGNED certificate of the '$s' department server '$c' client's keystore"
		echo "------------------------------------------------------------------------------------------------------"
		keytool -import -alias ${S_alias} -keypass ${C_pass} -keystore ${C_ks} -storepass ${C_pass} -trustcacerts -file ${S_certificate}
	done
done
