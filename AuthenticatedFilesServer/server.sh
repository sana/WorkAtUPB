#!/bin/sh

if test $# -ne 1; then
	echo "Usage: $0 department_name"
	exit 1
fi
rm -rf server/security/$1
mkdir server/security/$1

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

# the identifier of this department server's key pair in the keystore
DS_alias=$1"_private"
# this department server's keystore
DS_ks=server/security/$1/$1".ks"
# the password for both this department server's keystore and key pair
DS_pass=$1"_password"
# the certificate request of this department server
DS_certificate_request=server/security/$1/$1".csr"
# the signed certificate of this department server
DS_certificate=server/security/$1/$1".crt"

# create a keystore for this department server
echo "--------------------------------------------------"
echo "Create the keystore for the '$1' department server"
echo "--------------------------------------------------"
keytool -genkey -alias ${DS_alias} -keystore ${DS_ks} -storetype JKS -keyalg rsa -dname "CN=$1, OU=department, O=SPRC, L=Bucharest, S=Bucharest, C=RO" -storepass ${DS_pass} -keypass ${DS_pass}

# create a certificate request for this department server
echo "-----------------------------------------------------------"
echo "Create a certificate request for the '$1' department server"
echo "-----------------------------------------------------------"
keytool -certreq -keyalg rsa -alias ${DS_alias} -keypass ${DS_pass} -keystore ${DS_ks} -storepass ${DS_pass} -file ${DS_certificate_request}

# have the certification authority sign the certificate request for this department server
echo "-------------------------------------------------------------------------------------------"
echo "Have the certification authority SIGN the certificate request of the '$1' department server"
echo "-------------------------------------------------------------------------------------------"
openssl x509 -CA ${CA_certificate} -CAkey ${CA_key} -CAserial ${CA_serial} -req -in ${DS_certificate_request} -out ${DS_certificate} -days 365

# import the certification authority's certificate into this department server's keystore
echo "------------------------------------------------------------------------------------------------"
echo "IMPORT the certificate of the CERTIFICATION AUTHORITY into the '$1' department server's keystore"
echo "------------------------------------------------------------------------------------------------"
keytool -import -alias ${CA_alias} -keystore ${DS_ks} -storepass ${DS_pass} -trustcacerts -file ${CA_certificate}

# import the signed certificate of this department server into this department server's keystore
echo "------------------------------------------------------------------------------------------------------"
echo "IMPORT the SIGNED certificate of the '$1' department server into the '$1' department server's keystore"
echo "------------------------------------------------------------------------------------------------------"
keytool -import -alias ${DS_alias} -keypass ${DS_pass} -keystore ${DS_ks} -storepass ${DS_pass} -trustcacerts -file ${DS_certificate}

# import the signed certificate of the authorization server into this department server's keystore
echo "--------------------------------------------------------------------------------------------------"
echo "IMPORT the SIGNED certificate of the AUTHORIZATION SERVER into the '$1' department server's keystore"
echo "--------------------------------------------------------------------------------------------------"
keytool -import -alias ${AS_alias} -keypass ${DS_pass} -keystore ${DS_ks} -storepass ${DS_pass} -trustcacerts -file ${AS_certificate}
