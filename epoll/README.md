<pre>
                  _ _ 
  ___ _ __   ___ | | |
 / _ \ '_ \ / _ \| | |
|  __/ |_) | (_) | | |
 \___| .__/ \___/|_|_|
     |_|              
</pre>

Simple event-driven (epoll/edge-triggered) tcp server

## Build server

make -f linux.mk

## Start server
<pre>
$ ./epoll
</pre>

## netcat (nc) to server

<pre>
$ nc localhost 54000
</pre>

