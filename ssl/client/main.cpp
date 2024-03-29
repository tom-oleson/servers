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

#include <unistd.h>     // for getopt()
#include "sslclient.h"

const char *__banner__ =
R"(         ______)""\n"
R"(        / ____ \)""\n"
R"(   ____/ /    \ \)""\n"
R"(  / ____/   x  \ \)""\n"  
R"( / /     __/   / / SSLCLIENT)""\n"
R"(/ /  x__/  \  / /  SSL Client)""\n"
R"(\ \     \__/  \ \  Copyright (C) 2019, Tom Oleson, All Rights Reserved.)""\n"
R"( \ \____   \   \ \ Made in the U.S.A.)""\n"
R"(  \____ \   x  / /)""\n"
R"(       \ \____/ /)""\n"
R"(        \______/)""\n";

void usage(int argc, char *argv[]) {
    puts("usage: sslclient [-d millis] [-k seconds] [-v] [destination] port [FILE]...");
    puts("");
    puts("-d millis     Delay between output lines (milliseconds)");
    puts("-k seconds    Keep connection open after output has finished");
    puts("              to allow large response to be fully received (seconds)");
    puts("-v            Output version/build info to console");
    puts("");
}

bool is_port_number(const std::string& s) {
    return( strspn( s.c_str(), "0123456789" ) == s.size() );
}

int main(int argc, char *argv[]) {

    int opt;
    bool version = false;
    int interval = 1;
    int keep = 0;
    std::string host = "localhost";
    int port = -1;

    while((opt = getopt(argc, argv, "hk:d:R:S:v")) != -1) {
        switch(opt) {

            case 'v':
                version = true;
                break;

            case 'd':
                interval = atoi(optarg);
                break;

            case 'k':
                keep = atoi(optarg);
                break;

            case 'h':
            default:
                usage(argc, argv);
                exit(0);
        }
    }

    sslclient::init_logs();

    if(version) {
        puts(__banner__);
        cm_log::always(cm_util::format("SSLCLIENT %s build: %s %s", VERSION ,__DATE__,__TIME__));
        usage(argc, argv);
        exit(0);
    }

    if(optind >= argc) {
        fputs("Expected argument after options\n", stderr);
        exit(1);
    }

    if(optind < argc && !is_port_number(argv[optind])) {
        host = argv[optind++];
    }

    if(optind < argc && is_port_number(argv[optind])) {
        port = atoi(argv[optind++]);
    }
    
    if(port == -1) {
        fputs("Missing argument port\n", stderr);
        exit(1);
    }

    std::vector<std::string> files;
    while(optind < argc) {
        files.push_back(argv[optind++]);
    }

    sslclient::run(keep, interval, host, port, files);

    return 0;
}
