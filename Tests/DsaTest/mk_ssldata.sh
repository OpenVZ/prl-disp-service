#!/bin/sh

d=ssldata

rm -rf $d
mkdir $d

f_msg=$d/file.txt

echo -ne "123456" > $f_msg

for h in sha1 sha256; do
for b in 1024 3072; do
suffix="${b}_${h}"
f_param=$d/dsaparam_$suffix.pem
f_pub=$d/pub_$suffix.key
f_priv=$d/priv_$suffix.key
f_sig=$d/sig_$suffix.dat
openssl dsaparam -out $f_param $b
openssl gendsa -out $f_priv $f_param
openssl dsa -in $f_priv -pubout -out $f_pub
openssl dgst -sign $f_priv -$h -out $f_sig $f_msg
done
done