#!/bin/sh

if test $# -ne 2; then
	echo "Usage: $0 client_name department_name"
	exit 1
fi
rm -rf client/security/$1
mkdir client/security/$1

# the certificate of the certification authority
CA_certificate="certification_authority/ca.pem"
# the file containing the certification authority's key
CA_key="certification_authority/ca.key"
# the file containing the certification authority's serial numbers
CA_serial="certification_authority/ca.srl"
# the identifier of the certification authority's key pair in this department server's keystore
CA_alias="certification_authority"

# the certificate of the authorization server
AS_certificate="authorization_server/security/authorization_server.crt"
# the identifier of the authorization server's key pair in this department server's keystore
AS_alias="authorization_server"

# the identifier of this client's key pair in the keystore
C_alias=$1"_private"
# this client's keystore
C_ks=client/security/$1/$1".ks"
# the password for both this client's keystore and key pair
C_pass=$1"_password"
# the certificate request of this client
C_certificate_request=client/security/$1/$1".csr"
# the signed certificate of this client
C_certificate=client/security/$1/$1".crt"

# create a keystore for this client
echo "---------------------------------------"
echo "Create the keystore for the client '$1'"
echo "---------------------------------------"
keytool -genkey -alias ${C_alias} -keystore ${C_ks} -storetype JKS -keyalg rsa -dname "CN=$1, OU=$2, O=SPRC, L=Bucharest, S=Bucharest, C=RO" -storepass ${C_pass} -keypass ${C_pass}

# create a certificate request for this client
echo "------------------------------------------------"
echo "Create a certificate request for the client '$1'"
echo "------------------------------------------------"
keytool -certreq -keyalg rsa -alias ${C_alias} -keypass ${C_pass} -keystore ${C_ks} -storepass ${C_pass} -file ${C_certificate_request}

# have the certification authority sign the certificate request for this client
echo "--------------------------------------------------------------------------------"
echo "Have the certification authority SIGN the certificate request of the client '$1'"
echo "--------------------------------------------------------------------------------"
openssl x509 -CA ${CA_certificate} -CAkey ${CA_key} -CAserial ${CA_serial} -req -in ${C_certificate_request} -out ${C_certificate} -days 365

# import the certification authority's certificate into this client's keystore
echo "------------------------------------------------------------------------------------------------"
echo "IMPORT the certificate of the CERTIFICATION AUTHORITY into the '$1' client's keystore"
echo "------------------------------------------------------------------------------------------------"
keytool -import -alias ${CA_alias} -keystore ${C_ks} -storepass ${C_pass} -trustcacerts -file ${CA_certificate}

# import the signed certificate of this client into this client's keystore
echo "------------------------------------------------------------------------------------------------------"
echo "IMPORT the SIGNED certificate of the '$1' client into the '$1' client's keystore"
echo "------------------------------------------------------------------------------------------------------"
keytool -import -alias ${C_alias} -keypass ${C_pass} -keystore ${C_ks} -storepass ${C_pass} -trustcacerts -file ${C_certificate}

# import the signed certificate of the authorization server into this client's keystore
echo "--------------------------------------------------------------------------------------------------"
echo "IMPORT the SIGNED certificate of the AUTHORIZATION SERVER into the '$1' client's keystore"
echo "--------------------------------------------------------------------------------------------------"
keytool -import -alias ${AS_alias} -keypass ${C_pass} -keystore ${C_ks} -storepass ${C_pass} -trustcacerts -file ${AS_certificate}
