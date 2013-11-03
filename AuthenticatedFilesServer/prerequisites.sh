#!/bin/sh

CA_certificate_request="certification_authority/ca.csr"
# the certificate of the certification authority
CA_certificate="certification_authority/ca.pem"
# the file containing the certification authority's key
CA_key="certification_authority/ca.key"
# the file containing the certification authority's serial numbers
CA_serial="certification_authority/ca.srl"
# the identifier of the authorization server's key pair in the keystore
AS_alias="authorization_server_private"
# the authorization server's keystore
AS_ks="authorization_server/security/authorization_server.ks"
# the password for both the authorization server's keystore and key pair
AS_pass="authorization_server_password"
# the certificate request of the authorization server
AS_certificate_request="authorization_server/security/authorization_server.csr"
# the signed certificate of the authorization server
AS_certificate="authorization_server/security/authorization_server.crt"
# the identifier of the authorization server's key pair in the authorization server's keystore
CA_alias="certification_authority"

rm -fr certification_authority/*

rm -fr authorization_server/security/*
rmdir authorization_server/security
mkdir authorization_server/security

rm -fr server/security/*
rmdir server/security
mkdir server/security

rm -fr client/security/*
rmdir client/security
mkdir client/security



# create a private key and certificate request for the certification authority
# (X.509 Certificate Signing Request (CSR) Management); generate a new rsa key on 1024 bits; don't encrypt the output key; specify the output file and the file to send the key to; do not ask anyting during request generation
echo "----------------------------------------------------------------------------"
echo "Create a private key and certificate request for the CERTIFICATION AUTHORITY"
echo "----------------------------------------------------------------------------"
openssl req -new -newkey rsa:1024 -nodes -out ${CA_certificate_request} -keyout ${CA_key} -batch

# create the certification authority's self-signed certificate
# (X.509 Certificate Data Management); output a "trusted" certificate; self sign certificate with the certification authority's key; specify how long till expiry of a signed certificate; input is a certificate request, sign and output; input file; output file
echo "-----------------------------------------------------------------"
echo "Create the self signed certificate of the CERTIFICATION AUTHORITY"
echo "-----------------------------------------------------------------"
openssl x509 -trustout -signkey ${CA_key} -days 365 -req -in ${CA_certificate_request} -out ${CA_certificate}

# create a file to hold the certification authority's serial numbers; this file starts with the number '2'
echo "-------------------------------------------------------------------------------"
echo "Create the file that will hold the serial number of the CERTIFICATION AUTHORITY"
echo "-------------------------------------------------------------------------------"
echo "02" > ${CA_serial}


# create a keystore for the authorization server
# tell keytool to generate a key pair; identifies the new key pair within the keystore; uses the file 'authorization_server.private' as the keystore; declares the type of the keystore (JKS is the default); the algorithm to be used; information about the entity owning the key pair; the password for the entire keystore; the passowrd for the new key pair
echo "------------------------------------------------"
echo "Create the keystore for the AUTHORIZATION SERVER"
echo "------------------------------------------------"
keytool -genkey -alias ${AS_alias} -keystore ${AS_ks} -storetype JKS -keyalg rsa -dname "CN=authorization_server, OU=security, O=SPRC, L=Bucharest, S=Bucharest, C=RO" -storepass ${AS_pass} -keypass ${AS_pass}

# create a certificate request for the authorization server
# create a certificate sign request; the algorithm to be used; the alias of the targeted key entry; the password used to access the key entry; the name of the keystore where that key entry can be found; the password used to access the keystore; the file where the certificate request will be placed
echo "---------------------------------------------------------"
echo "Create a certificate request for the AUTHORIZATION SERVER"
echo "---------------------------------------------------------"
keytool -certreq -keyalg rsa -alias ${AS_alias} -keypass ${AS_pass} -keystore ${AS_ks} -storepass ${AS_pass} -file ${AS_certificate_request}

# have the certification authority sign the certificate request of the authorization server
# (X.509 Certificate Data Management); specify the certification authority's certificate; specify the certification authority's key; specify the certification authority's serial number; input is a certificate request to be signed; input (the certificate to be signed); output (the signed certificate); how long till expiry of the signed certificate
echo "-----------------------------------------------------------------------------------------"
echo "Have the certification authority SIGN the certificate request of the authorization server"
echo "-----------------------------------------------------------------------------------------"
openssl x509 -CA ${CA_certificate} -CAkey ${CA_key} -CAserial ${CA_serial} -req -in ${AS_certificate_request} -out ${AS_certificate} -days 365

# import the certification authority's certificate into the authorization server's keystore
echo "----------------------------------------------------------------------------------------------"
echo "IMPORT the certificate of the CERTIFICATION AUTHORITY into the AUTHORIZATION SERVER's keystore"
echo "----------------------------------------------------------------------------------------------"
keytool -import -alias ${CA_alias} -keystore ${AS_ks} -storepass ${AS_pass} -trustcacerts -file ${CA_certificate}

# import the signed certificate of the authorization server into the authorization server's keystore
echo "--------------------------------------------------------------------------------------------------"
echo "IMPORT the SIGNED certificate of the AUTHORIZATION SERVER into the AUTHORIZATION SERVER's keystore"
echo "--------------------------------------------------------------------------------------------------"
keytool -import -alias ${AS_alias} -keypass ${AS_pass} -keystore ${AS_ks} -storepass ${AS_pass} -trustcacerts -file ${AS_certificate}
