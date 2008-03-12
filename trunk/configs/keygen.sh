#! /bin/sh

while getopts h:e:y:bf c
     do
         case $c in
           b)         BATCH=1;;
           f)         FORCE=1;;
           h)         FQDN=$OPTARG;;
           e)         ENTITYID=$OPTARG;;
           y)         YEARS=$OPTARG;;
           \?)        echo keygen [-h hostname for cert] [-y years to issue cert] [-e entityID to embed in cert]
                      exit 1;;
         esac
     done

if [ -n $FORCE ] ; then
    rm sp-key.pem sp-cert.pem
fi

if  [ -e sp-key.pem ] || [ -e sp-cert.pem ] ; then
    if [ -z $BATCH ] ; then  
        echo The files sp-key.pem and/or sp-cert.pem already exist!
        echo Use -f option to force recreation of keypair.
        exit 2
    fi
    exit 0
fi

if [ -z $FQDN ] ; then
    FQDN=`hostname`
fi

if [ -z $YEARS ] ; then
    YEARS=10
fi

DAYS=$(($YEARS*365))

if [ -z $ENTITYID ] ; then
    ALTNAME=subjectAltName=DNS:$FQDN
else
    ALTNAME=subjectAltName=DNS:$FQDN,URI:$ENTITYID
fi

cat >sp-cert.cnf <<EOF
# OpenSSL configuration file for creating sp-cert.pem
[req]
prompt=no
default_bits=2048
encrypt_key=no
default_md=sha1
distinguished_name=dn
# PrintableStrings only
string_mask=MASK:0002
x509_extensions=ext
[dn]
CN=$FQDN
[ext]
subjectAltName=$ALTNAME
subjectKeyIdentifier=hash
EOF

if [ -z $BATCH ] ; then
    openssl req -config sp-cert.cnf -new -x509 -days $DAYS -keyout sp-key.pem -out sp-cert.pem
else
    openssl req -config sp-cert.cnf -new -x509 -days $DAYS -keyout sp-key.pem -out sp-cert.pem 2> /dev/null
fi
