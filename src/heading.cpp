/**********************************************************
Heading.c

Description:
  Useful for transferring heading data from
  the Gizmo 2 board to the Tegra X1

*/
char *use_msg =
"Usage:\n"
"  heading [OPTIONS] [-d /dev/path]\n\n"

"  -d, --device\n"
"    begins a service and opens the serial device on the\n"
"    specified path\n"
"  -r, --read\n"
"    an example code reading from the FIFO to obtain\n"
"    heading data\n"
"  -e, --elevation=angle\n"
"    send elevation data supplied to the service for\n"
"    transmission\n"
"  -a, --azimuth=angle\n"
"    send azimuth data supplied to the service for\n"
"    transmission\n"
"  -k, --kill\n"
"    kill the service\n"
"  -v, --verbose\n"
"    print lots of useful comments for debugging\n"
"  -h, --help\n"
"    print this usage message\n\n"

"note:\n"
"  1. before using any options the service must be started\n"
"     using the 'd' or 'device' option\n"
"  2. data received by the service can be accessed by\n"
"     reading from the FIFO located at /tmp/headingfifo\n\n"

"Example (transmit)\n"
"  heading --device=/dev/ttyUSB0\n"
"  heading --azimuth=170.3 --elevation=45.3\n"
"Example (receive)\n"
"  heading --device=/dev/ttyUSB0\n"
"  heading -r\n"
"  [see read_data_fifo() in source code for c implementation]\n";


// --------------------------------------------------------
// #include's

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
//#include <ros/ros.h>



// --------------------------------------------------------
// function declarations

void close_connection(void);


// --------------------------------------------------------
// Global variables

#define BUF_SIZE 100

// file descriptor for serial port
int tty_fd;

// fifo
char heading_fifo[] = "/tmp/headingfifo";
int fifo_fd;

// static flags used by command line options
static int initialize_flag = 0;
static int read_flag = 0;
static int send_flag = 0;
static int kill_flag = 0;
static int verbose_flag = 0;

// variable storage for command line options
static char elevation_data[BUF_SIZE];
static char azimuth_data[BUF_SIZE];
static char device[BUF_SIZE];

// heading data structure
typedef struct
{
  char azimuth[BUF_SIZE];
  char elevation[BUF_SIZE];
} heading_struct;

// data received is stored and overwritten here
heading_struct angles;

// headers for data transmission
char azimuth_header[] = "AZIMUTH";
char elevation_header[] = "ELEVATION";


// --------------------------------------------------------
// Display information

// print usage
void print_usage()
{
  printf(use_msg);
}

void print_options()
{
  printf("Command line arguments\n");
  printf("----------------------\n");
  if (strlen(device) > 0)
    printf("Device: %s\n", device);
  if (strlen(elevation_data) > 0)
    printf("Elevation: %s\n", elevation_data);
  if (strlen(azimuth_data) > 0)
    printf("Azimuth: %s\n", azimuth_data);
  if (initialize_flag)
    printf("Opening serial device.\n");
  if (read_flag)
    printf("Reading last data received.\n");
  if (kill_flag)
    printf("Sending kill signal.\n");
}


// --------------------------------------------------------
// Initialization functions

// create fifo for inter-process communication
void create_fifo()
{
  // first check if file exists
  if (access(heading_fifo, F_OK))
    mkfifo(heading_fifo, 0666);
  if (fifo_fd < 1)
    fifo_fd = open(heading_fifo, O_RDWR | O_NONBLOCK);
  if (fifo_fd < 1)
  {
    printf("Error: Could not open FIFO.\n");
    exit(EXIT_FAILURE);
  }
}

// configure and open the serial port
void configure_serial()
{
  struct termios tio;
  char c = 0x00;
  
  memset(&tio, 0, sizeof(tio));
  tio.c_iflag = 0;
  tio.c_oflag = 0;
  tio.c_cflag = CS8 | CREAD | CLOCAL;
  tio.c_lflag = 0;
  tio.c_cc[VMIN] = 1;
  tio.c_cc[VTIME] = 5;
  cfsetospeed(&tio, B115200);
  cfsetispeed(&tio, B115200);
  
  tty_fd = open(device, O_RDWR | O_NONBLOCK);
  if (tty_fd < 0)
  {
    printf("Error: Failed to open serial port - %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  tcsetattr(tty_fd, TCSANOW, &tio);
  
  memset(&angles, 0, sizeof(angles));
}


// --------------------------------------------------------
// Data transfer TO the serial port or the fifo

void send_data_serial()
{
  int azimuth_len = strlen(angles.azimuth);
  int elevation_len = strlen(angles.elevation);
  if (azimuth_len < 1 || elevation_len < 1)
    return;
  char data[BUF_SIZE];
  // transmit azimuth data
  sprintf(data, "%s%s\n", azimuth_header, angles.azimuth);
  write(tty_fd, data, strlen(data));
  if (verbose_flag)
    printf("Sending %s over serial port.\n", data);
  // transmit elevation data
  sprintf(data, "%s%s\n", elevation_header, angles.elevation);
  write(tty_fd, data, strlen(data));
  if (verbose_flag)
    printf("Sending %s over serial port.\n", data);
}

void send_data_fifo(int send)
{
  char data[BUF_SIZE];
  
  if (verbose_flag)
    printf("Sending data to FIFO.\n");
  create_fifo();

  sprintf(data, "%s%s\n", elevation_header, elevation_data);
  write(fifo_fd, data, strlen(data));

  sprintf(data, "%s%s\n", azimuth_header, azimuth_data);
  write(fifo_fd, data, strlen(data));
  
  if (send)
  {
    sprintf(data, "SEND\n");
    write(fifo_fd, data, strlen(data));
  }
}

void send_fifo_kill()
{
  if (verbose_flag)
    printf("Sending kill signal to FIFO.\n");
  create_fifo();
  write(fifo_fd, "K\n", 2);
}


// --------------------------------------------------------
// Data transfer FROM the serial port or the fifo

void read_data_fifo()
{
  char buf[BUF_SIZE];
  int cr;

  create_fifo();

  while (1) {
    // read from fifo
    memset(buf, 0, BUF_SIZE);
    cr = read(fifo_fd, &buf, BUF_SIZE);
    // output to stdout
    if (cr > 0)
    {
      FILE *file = fmemopen(buf, strlen(buf), "r");
      char line[BUF_SIZE];
      char tmp[BUF_SIZE];
      char data[BUF_SIZE];
      int r;
      while (fgets(line, BUF_SIZE, file))
      {
        // look for Elevation data
        strcpy(tmp, elevation_header);
        strcat(tmp, "%s\n");
        r = sscanf(line, tmp, data);
        if (r > 0)
          printf("Elevation: %s\n", data);

        // look for Azimuth data
        strcpy(tmp, azimuth_header);
        strcat(tmp, "%s\n");
        r = sscanf(line, tmp, data);
        if (r > 0)
          printf("Azimuth: %s\n", data);
      }
    }
    // sleep
    usleep(10000);
  }
}

void process_data(char *buf)
{
  char tmp[BUF_SIZE];
  char msg[BUF_SIZE];
  int r;
  int send = 0;
  char line[BUF_SIZE];
  FILE *file = fmemopen(buf, strlen(buf), "r");
  while(fgets(line, BUF_SIZE, file))
  {
    if (verbose_flag)
      printf("Serial Message: %s", line);
    
    // Look for Elevation data
    strcpy(tmp, elevation_header);
    strcat(tmp, "%s\n");
    r = sscanf(line, tmp, msg);
    if (r > 0)
    {
      if (verbose_flag)
        printf("Elevation: %s\n", msg);
      strcpy(elevation_data, msg);
      send = 1;
    }
    
    // Look for Azimuth data
    strcpy(tmp, azimuth_header);
    strcat(tmp, "%s\n");
    r = sscanf(line, tmp, msg);
    if (r > 0)
    {
      if (verbose_flag)
        printf("Azimuth: %s\n", msg);
      strcpy(azimuth_data, msg);
      send = 1;
    }
  }
  if (send)
    send_data_fifo(0);
}

void process_fifo(char *buf)
{
  char tmp[BUF_SIZE];
  char msg[BUF_SIZE];
  float data;
  int r;
  char *ret;
  char line[BUF_SIZE];
  FILE *file = fmemopen(buf, strlen(buf), "r");
  while (fgets(line, BUF_SIZE, file))
  {
    if (verbose_flag)
      printf("FIFO Message: %s", line);
    
    // Look for Elevation data
    strcpy(tmp, elevation_header);
    strcat(tmp, "%s\n");
    r = sscanf(line, tmp, msg);
    if (r > 0)
    {
      if (verbose_flag)
        printf("Elevation: %s\n", msg);
      strcpy(angles.elevation, msg);
    }
    
    // Look for Azimuth data
    strcpy(tmp, azimuth_header);
    strcat(tmp, "%s\n");
    r = sscanf(line, tmp, msg);
    if (r > 0)
    {
      if (verbose_flag)
        printf("Azimuth: %s\n", msg);
      strcpy(angles.azimuth, msg);
    }
    
    // Look for Send command
    ret = strstr(line, "SEND\n");
    if (ret)
    {
      if (verbose_flag)
        printf("Found SEND\n");
      send_data_serial();
    }

    // Look for Kill command
    ret = strstr(line, "K\n");
    if (ret)
    {
      if (verbose_flag)
        printf("Found Kill\n");
      close_connection();
      exit(EXIT_SUCCESS);
    }
  }
}

void main_loop()
{
  char buf[BUF_SIZE];
  int cr;
//  configure_serial();   ###################### I commented this out for testing.
  create_fifo();

  while (1) {
    // read from fifo
    memset(buf, 0, BUF_SIZE);
    cr = read(fifo_fd, &buf, BUF_SIZE);
    // process input from fifo
    if (cr > 0)
      process_fifo(buf);
    // read from serial in
    memset(buf, 0, BUF_SIZE);
    cr = read(tty_fd, &buf, BUF_SIZE);
    // process input from serial in
    if (cr > 0)
      process_data(buf);
    // sleep for 10 ms
    usleep(10000);
  }
}


// --------------------------------------------------------
// Other functions

void close_connection(void)
{
  close(tty_fd);
  close(fifo_fd);
}

void parse_options(int argc, char** argv)
{
  int c;

  while (1)
  {
    static struct option lopts[] =
    {
      {"device", required_argument,            0, 'd'},
      {"read", no_argument,             &read_flag, 1},
      {"elevation", required_argument,         0, 'e'},
      {"azimuth", required_argument,           0, 'a'},
      {"kill", no_argument,             &kill_flag, 1},
      {"verbose", no_argument,       &verbose_flag, 1},
      {"help", no_argument,                    0, 'h'}
    };

    int option_index = 0;
    c = getopt_long(argc, argv, "d:re:a:kvh", lopts, &option_index);

    // end of options
    if (c == -1)
      break;

    switch (c)
    {
      case 0:
        break;
      case 'd':
        initialize_flag = 1;
        strcpy(device, optarg);
        break;
      case 'r':
        read_flag = 1;
        break;
      case 'e':
        strcpy(elevation_data, optarg);
        send_flag = 1;
        break;
      case 'a':
        strcpy(azimuth_data, optarg);
        send_flag = 1;
        break;
      case 'k':
        kill_flag = 1;
        break;
      case 'v':
        verbose_flag = 1;
        break;
      case 'h':
        print_usage();
        exit(EXIT_SUCCESS);
        break;
      case '?':
        printf("Error: Invalid option.\n");
        print_usage();
        exit(EXIT_FAILURE);
      default:
        printf("Error: Invalid option %c.\n", c);
        print_usage();
        exit(EXIT_FAILURE);
    }
  }
  
  if (verbose_flag)
    print_options();
}

int heading_is_running()
{
  char result[50];
  int count = 0;
  FILE *cmd = popen("pgrep heading", "r");
  if (!cmd)
  {
    printf("Error: (popen) failed to check if heading is already running.\n");
    exit(EXIT_FAILURE);
  }
  while(fgets(result, 50, cmd))
    count++;
  pclose(cmd);
//  return (count > 1);
  return 1;
}

// --------------------------------------------------------
// main

int main(int argc, char** argv)
{
  // process input options
  parse_options(argc, argv);
  
  // verbose syntax
  if (verbose_flag)
  {
    printf("\nMessages\n");
    printf("----------------------\n");
  }

  // initialize serial port
  if (initialize_flag && heading_is_running()) {
    printf("Error: a serial port is already open. If opening a new serial connection use the -k option to close the first connection.\n");
  } else if (initialize_flag) {
    // check for invalid options
    if (send_flag || read_flag || kill_flag) {
      printf("Warning: All options other than -d are ignored.\n");
    }
    // open serial port by launching main_loop process
    main_loop();
  }

  // process should already be running for other options
  if (heading_is_running())
  {
    // check for invalid combination of options
    if (read_flag && kill_flag)
    {
      printf("Error: Cannot read and kill in the same operation.\n");
      exit(EXIT_FAILURE);
    }
    // send options to process
    if (send_flag)
    {
      // send data message
      send_data_fifo(1);
    }
    if (read_flag)
    {
      // read data message
      read_data_fifo();
    }
    if (kill_flag)
    {
      // send kill message
      send_fifo_kill();
    }
  } else {
    printf("Error: Serial connection must already be open to use the given options. Use the -d option to open a serial connection for the given device path.\n");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
