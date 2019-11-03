<pre>
         ______
        / ____ \
   ____/ /    \ \
  / ____/   x  \ \
 / /     __/   / / SSLCLIENT
/ /  x__/  \  / /  SSL Client
\ \     \__/  \ \  Copyright (C) 2019, Tom Oleson, All Rights Reserved.
 \ \____   \   \ \ Made in the U.S.A.
  \____ \   x  / /
       \ \____/ /
        \______/
</pre>


# sslclient
Connect to sslecho server

<pre>
usage: sslclient [-d millis] [-k seconds] [-v] [destination] port [FILE]...

-d millis     Delay between output lines (milliseconds)
-k seconds    Keep connection open after output has finished
              to allow large response to be fully received (seconds)
-v            Output version/build info to console
</pre>


Examples:

// read from standard input and print remote input on standard output
<pre>$ sslclient localhost 1234 </pre>

// send input file to remote and save remote input to output file
// keep the connection open for 30 seconds after input EOF
<pre>$ sslclient -k30 localhost 1234 request.txt > response.txt</pre>


// send input file to remote and save remote input to output file
// delay 200 milliseconds between output lines
<pre>$ sslclient -d200 localhost 1234 request.txt > response.txt</pre>

// continuous tail input file to remote
<pre>$ tail -f app.log | sslclient -k1 localhost 1234</pre>
