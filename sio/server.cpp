/*
 * Copyright (c) 2019, Tom Oleson <tom dot oleson at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * The names of its contributors may NOT be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "server.h"

cm_net::client_thread *client = nullptr;
bool connected = false;
int host_port = -1;

void _sleep(int interval /* ms */) {

    // allow other threads to do some work
    long n = 1000000;
    time_t s = 0;
    if(interval > 0) { 
        n = ((long) interval % 1000L) * 1000000L;
        s = (time_t) interval / 1000;
    }
    timespec delay = {s, n};
    nanosleep(&delay, NULL);    // interruptable
}

cm::mutex rx_mutex;
cm::cond rx_response;
bool rx_received = false;
const char *rx_buffer = nullptr;
size_t rx_len = 0;

void server_receive(int fd, const char *buf, size_t sz) {

    if(nullptr != client) {
        if(client->is_connected()) {
            cm_net::send(client->get_socket(), std::string(buf, sz));

            rx_mutex.lock();

            timespec now;
            clock_gettime(CLOCK_REALTIME, &now);
            time_t timeout = cm_time::millis(now) + 100;   // 100ms

            rx_received = false;

            while(!rx_received) {

                // unlocks mutex, then sleeps
                // on wakeup signal, locks mutex again
                timespec ts = {0, 10000000};   // 10 ms
                rx_response.timed_wait(rx_mutex, ts);

                // see if we've timed out waiting for response                
                clock_gettime(CLOCK_REALTIME, &now);
                if(cm_time::millis(now) > timeout) {
                    rx_mutex.unlock();
                    rx_response.broadcast();
                    return;
                }
            }

            // send received data to serial fd
            if(rx_len > 0) cm_sio::sio_write(fd, rx_buffer, rx_len);

            rx_mutex.unlock();
            rx_response.signal();
        }
        else {
            // data not delivered
            // to-do: add send queue
        }
    }
    else if(-1 == host_port) {
        std::cout << cm_util::format("%s", std::string(buf, sz).c_str());
        std::cout.flush();
    }
}

void client_receive(int socket, const char *buf, size_t sz) {
    
    rx_mutex.lock();

    rx_buffer = buf;
    rx_len = sz;
    rx_received = true;
    
    rx_mutex.unlock();

    rx_response.signal();   // wake up waiting thread
}

void sioecho::run(const std::vector<std::string> &ports, const std::string &host_name, int _host_port) {

    // make stdio work with pipes
    std::ios_base::sync_with_stdio(false); 
    std::cin.tie(NULL);

    if(_host_port != -1) {
	host_port = _host_port;
        cm_log::info(cm_util::format("remote server: %s:%d", host_name.c_str(), host_port));
    }

    bool connected = false;     // client connected to remote server
    time_t next_connect_time = 0;

    cm_sio::sio_server server(ports, server_receive);
    while( !server.is_done() ) {
        _sleep(1000);

        if(host_port != -1) {
            if(nullptr == client) {
                client = new cm_net::client_thread(host_name, host_port, client_receive); 
                next_connect_time = cm_time::clock_seconds() + 60;
            }

            // if client thread is running, we are connected
            if(nullptr != client && client->is_connected()) {
                if(!connected) {
                    connected = true;
                }
            }    

            // if client thread is NOT running, we are NOT connected
            if(nullptr != client && !client->is_connected()) {
                if(connected) {
                    connected = false;
                }

                if(cm_time::clock_seconds() > next_connect_time) {
                    // attempt reconnect
                    client->start();
                    next_connect_time = cm_time::clock_seconds() + 60;
                }
            }
        }                
    }
}
