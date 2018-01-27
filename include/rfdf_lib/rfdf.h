#ifndef RFDF_H
#define RFDF_H

#include <iostream>
#include <time.h>

#include <string.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUF_SIZE 100

using namespace std;

class rfdf
{
public:
    void process_serial_data(char *buf, int cr);
    void serial_sleep(int milliseconds);
    void main_loop();
    void test_transmit_loop();
    void configure_serial();
    void send_data_serial(float elevation, float azimuth, int id);
    void parse_options(int argc, char** argv);

    int buf_size_ = 100;
    char device[BUF_SIZE];
    int run_test_flag = 0;
    int device_flag = 0;
    int tty_fd;

};

#endif // RFDF_H
