<pre>
         ______
        / ____ \
   ____/ /    \ \
  / ____/   x  \ \
 / /     __/   / / RCAT
/ /  x__/  \  / /  Remote File Concatenator
\ \     \__/  \ \  Copyright (C) 2019, Tom Oleson, All Rights Reserved.
 \ \____   \   \ \ Made in the U.S.A.
  \____ \   x  / /
       \ \____/ /
        \______/
</pre>


# rcat
Concatenate files to remote and print remote input on standard output

<pre>
usage: rcat [-d millis] [-k seconds] [-R size] [-S size] [-v] [destination] port [FILE]...

-d millis     Delay between output lines (milliseconds)
-k seconds    Keep connection open after output has finished
              to allow large response to be fully received (seconds)
-R size       Set socket receive buffer size
-S size       Set socket send buffer size
-v            Output version/build info to console
</pre>


Examples:

// read from standard input and print remote input on standard output
<pre>$ rcat localhost 1234 </pre>


// pipe message with single quotes and print remote input on standard output
<pre>$ echo $"+name 'Tom Oleson'" | rcat localhost 1234 </pre>
<pre>$ echo $"\$name" | rcat localhost 1234 </pre>

// send input file to remote and save remote input to output file
// keep the connection open for 30 seconds after input EOF
<pre>$ rcat -k30 localhost 1234 request.txt > response.txt</pre>


// send input file to remote and save remote input to output file
// delay 200 milliseconds between output lines
<pre>$ rcat -d200 localhost 1234 request.txt > response.txt</pre>

// continuous tail input file to remote
<pre>$ tail -f app.log | rcat -k1 localhost 1234</pre>



