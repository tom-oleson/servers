<pre>
         ______
        / ____ \
   ____/ /    \ \
  / ____/   ___\ \
 / /\    __/   / /
/ /  \__/  \  / /  SSLECHO
\ \     \__/  \ \  Copyright (C) 2019, Tom Oleson, All Rights Reserved.
 \ \____   \   \ \ Made in the U.S.A.
  \____ \  /   / /
       \ \/___/ /
        \______/

</pre>

# sslecho
Demonstrates a simple SSL server

Requires libcm_64.a from project common

<pre>
usage: sslecho [-v] port

-v            Output version/build info to console
</pre>

Example:

First we create a private key and a certificate
<pre>$ openssl req -x509 -nodes -days 365 -newkey rsa:1024 -keyout key.pem -out cert.pem</pre>

1. Start the sslecho server:
<pre>$ sslecho 58000</pre>

2. In another terminal, connect to sslecho server using openssl command line tool. Whatever we type after we connect will be echoed to stdout on the server:

<pre>
openssl s_client -connect localhost:58000 -CAfile ./cert.pem

CONNECTED(00000005)
depth=0 C = US, ST = Florida, L = Tampa, O = OnTask Enterprises LLC, CN = OnTask
verify return:1
---
Certificate chain
 0 s:C = US, ST = Florida, L = Tampa, O = OnTask Enterprises LLC, CN = OnTask
   i:C = US, ST = Florida, L = Tampa, O = OnTask Enterprises LLC, CN = OnTask
---
Server certificate
-----BEGIN CERTIFICATE-----
MIICnjCCAgegAwIBAgIUXifNPZE0GWavyejLA681l6g9czcwDQYJKoZIhvcNAQEL
BQAwYTELMAkGA1UEBhMCVVMxEDAOBgNVBAgMB0Zsb3JpZGExDjAMBgNVBAcMBVRh
bXBhMR8wHQYDVQQKDBZPblRhc2sgRW50ZXJwcmlzZXMgTExDMQ8wDQYDVQQDDAZP
blRhc2swHhcNMTkxMDI3MDEzNzAxWhcNMjAxMDI2MDEzNzAxWjBhMQswCQYDVQQG
EwJVUzEQMA4GA1UECAwHRmxvcmlkYTEOMAwGA1UEBwwFVGFtcGExHzAdBgNVBAoM
Fk9uVGFzayBFbnRlcnByaXNlcyBMTEMxDzANBgNVBAMMBk9uVGFzazCBnzANBgkq
hkiG9w0BAQEFAAOBjQAwgYkCgYEAwFpHXXTgnF3SHaYrJClknxj2dE/pPse+riS+
slbZe6X07h9IFURqcUe4qyqkGhTSPZeMcaQKCtP62o2kcfO6dBz5fD2DUYVIJv9x
iSSwsQkhDfJZX7yYlLPGmw7d4j1zTrDYj0qUAW996T5HHVMLbzLxJQUiaioUQqZz
haUl/DUCAwEAAaNTMFEwHQYDVR0OBBYEFM3Y60Qlm45KkEWvNDDFpk04oussMB8G
A1UdIwQYMBaAFM3Y60Qlm45KkEWvNDDFpk04oussMA8GA1UdEwEB/wQFMAMBAf8w
DQYJKoZIhvcNAQELBQADgYEAiXTG7tOujS/kfYlTrNEgziIMFR/vaCVkkZD1d4K9
yZBeqCNwd0dH6CruxlltQCnm+WTvlf5gBAcPUaFmsNdTqzKP2ppIbpBSQRU33+7E
PmYtP0uCSEZT7kr6fJ1O+VoziUSlLxDGT06MN5wbeHf+3kry2uuseaj9ew5xkACm
UbQ=
-----END CERTIFICATE-----
subject=C = US, ST = Florida, L = Tampa, O = OnTask Enterprises LLC, CN = OnTask

issuer=C = US, ST = Florida, L = Tampa, O = OnTask Enterprises LLC, CN = OnTask

---
No client certificate CA names sent
Peer signing digest: SHA256
Peer signature type: RSA-PSS
Server Temp Key: X25519, 253 bits
---
SSL handshake has read 1102 bytes and written 391 bytes
Verification: OK
---
New, TLSv1.3, Cipher is TLS_AES_256_GCM_SHA384
Server public key is 1024 bit
Secure Renegotiation IS NOT supported
Compression: NONE
Expansion: NONE
No ALPN negotiated
Early data was not sent
Verify return code: 0 (ok)
---
---
Post-Handshake New Session Ticket arrived:
SSL-Session:
    Protocol  : TLSv1.3
    Cipher    : TLS_AES_256_GCM_SHA384
    Session-ID: D73957EECF733742669725DADF94967BC2BFCEC705002599E32675CE2833E4CE
    Session-ID-ctx: 
    Resumption PSK: 1BB2E5C94AEEFF997C001C611DE3F262112A78F1B050659028972A4840CB04E10A9406232424E9221DE1EEEE8F64F82D
    PSK identity: None
    PSK identity hint: None
    SRP username: None
    TLS session ticket lifetime hint: 7200 (seconds)
    TLS session ticket:
    0000 - 7f 18 e5 71 ae 4a dd 2e-3f 7e ad d9 d3 19 a6 14   ...q.J..?~......
    0010 - b0 13 97 ff ca d9 74 24-b6 0f e5 12 7f e4 8d 1e   ......t$........
    0020 - ee 14 8d 82 42 dc 97 e8-69 41 45 6b ba 3f ce c1   ....B...iAEk.?..
    0030 - 35 b3 6a 49 37 a9 9d ca-53 3c 8c e2 50 24 a6 e8   5.jI7...S<..P$..
    0040 - a8 06 5c 0a 3d c6 c5 8e-32 41 db f1 e7 ae e8 45   ..\.=...2A.....E
    0050 - c5 7e 28 1a 2b d9 a4 56-f5 36 b1 f8 35 6b 5a f7   .~(.+..V.6..5kZ.
    0060 - 54 e5 99 be a6 77 d5 6e-8a c4 cd f7 d2 ab 3a 15   T....w.n......:.
    0070 - 45 2a ce d9 3d d4 07 05-8b 1b 2a 34 b2 fe 4a c3   E*..=.....*4..J.
    0080 - 98 09 94 f3 a6 fb 97 3f-dd 22 02 ed 50 bc 87 1d   .......?."..P...
    0090 - a6 5a c6 0d 3b 41 bd 12-ef 82 5b e3 64 e6 34 02   .Z..;A....[.d.4.
    00a0 - 31 b8 e4 b3 5d bd 59 f9-22 64 ee 52 fd e6 59 e2   1...].Y."d.R..Y.
    00b0 - 4b bb de 46 76 fb 33 32-b7 0f 72 68 c0 a2 3d 68   K..Fv.32..rh..=h

    Start Time: 1572147133
    Timeout   : 7200 (sec)
    Verify return code: 0 (ok)
    Extended master secret: no
    Max Early Data: 0
---
read R BLOCK
---
Post-Handshake New Session Ticket arrived:
SSL-Session:
    Protocol  : TLSv1.3
    Cipher    : TLS_AES_256_GCM_SHA384
    Session-ID: CF89C3ACB858D6B0E4CF2141154039B2D1F86C7C62F165F67A56D9D5B74B1DEC
    Session-ID-ctx: 
    Resumption PSK: 638E4A8FE5DB0174CB23D55EF15791B7751BA3C1FB8B1F443AF6149852C86CFC493C9733F7C0402D7D33DD41E3CCFF45
    PSK identity: None
    PSK identity hint: None
    SRP username: None
    TLS session ticket lifetime hint: 7200 (seconds)
    TLS session ticket:
    0000 - 7f 18 e5 71 ae 4a dd 2e-3f 7e ad d9 d3 19 a6 14   ...q.J..?~......
    0010 - d9 32 54 cc c1 33 ca 79-4c 48 f5 83 8f 2a 99 23   .2T..3.yLH...*.#
    0020 - 4c 8d 35 a9 b6 fb a3 52-fa 24 be 0c f4 c6 45 6f   L.5....R.$....Eo
    0030 - 11 11 c3 f8 b6 f3 b1 db-65 d0 7d f1 e4 06 89 79   ........e.}....y
    0040 - ed 6c 05 2a a4 9a 87 94-19 bb 72 dd 91 b7 6d 3c   .l.*......r...m<
    0050 - 1b 00 2d 0a 45 88 65 f4-1e 79 3e 56 53 35 74 a4   ..-.E.e..y>VS5t.
    0060 - 19 0d 13 9d 02 9a b4 1e-9e e0 cd 02 45 ce 26 4c   ............E.&L
    0070 - ae 10 c9 a6 42 46 54 85-4d 05 bf cb a0 6e ad 91   ....BFT.M....n..
    0080 - c9 39 9f 5a 76 2f 08 dd-95 e0 eb a2 88 fc fe 89   .9.Zv/..........
    0090 - d8 f5 7c 8f 50 e6 4b a6-74 b9 af a9 2b c1 53 4f   ..|.P.K.t...+.SO
    00a0 - e0 c1 f5 47 09 b7 a5 4d-7c 08 b6 f7 05 71 6f bf   ...G...M|....qo.
    00b0 - 83 d4 8c b7 65 2c 51 a8-16 0f 51 05 7f 45 90 28   ....e,Q...Q..E.(

    Start Time: 1572147133
    Timeout   : 7200 (sec)
    Verify return code: 0 (ok)
    Extended master secret: no
    Max Early Data: 0
---
read R BLOCK

Hello, world!

</pre>


