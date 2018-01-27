/**********************************************************
rfdf.cpp

Description:
  Useful for transferring heading data from
  the Gizmo 2 board to ROS system

*/

#include "rfdf_lib/rfdf.h"


// --------------------------------------------------------
// Serial: Configure serial port and set up listener

void rfdf::configure_serial()
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
}

// send serial data (used only by Gizmo)
void rfdf::send_data_serial(float elevation, float azimuth, int id)
{
  // create serial message
  char msg[BUF_SIZE];
  sprintf(msg, "EAI%08.1f,%08.1f,%010d;\n", elevation, azimuth, id);

  // debug: display message
  std::cout << msg << std::endl;

  // transmit message
  write(tty_fd, msg, strlen(msg));
}

// read serial data main loop
void rfdf::main_loop()
{
  char buf[BUF_SIZE];
  int cr;

  configure_serial();

  while (1)
  {
    // read from serial port
    memset(buf, 0, BUF_SIZE);
    cr = read(tty_fd, &buf, BUF_SIZE);
    // process input from serial data
    if (cr > 0)
      process_serial_data(buf, cr);
    // sleep for 10 ms
    serial_sleep(10);
  }
}

void rfdf::process_serial_data(char *buf, int cr)
{
  char fmt[BUF_SIZE];
  char line[BUF_SIZE];
  FILE *file = fmemopen(buf, strlen(buf), "r");
  float elevation;
  float azimuth;
  int id;

  while (fgets(line, BUF_SIZE, file))
  {
    // look for serial message header
    int r = sscanf(line, "EAI%f,%f,%d;\n", &elevation, &azimuth, &id);
    if (r > 0)
    {
      // found a message
      std::cout << "EAI " << elevation << "," << azimuth << "," << id;
    }
  }
}


// --------------------------------------------------------
// Other: Miscellaneous functions

void rfdf::serial_sleep(int milliseconds)
{
  struct timespec req = {0};
  req.tv_sec = 0;
  req.tv_nsec = milliseconds * 1000000L;
  nanosleep(&req, (struct timespec *)NULL);
}

void rfdf::test_transmit_loop()
{
  configure_serial();

  for (int i = 0; i < 100; i++)
  {
    double elevation = i + 0.1;
    double azimuth = 100 - i + 0.2;
    int id = i;
    // transmit one message
    send_data_serial(elevation, azimuth, id);

    // pause for 10 ms
    serial_sleep(10);
  }
}

void rfdf::parse_options(int argc, char** argv)
{
  int c;

  while (1)
  {
    static struct option lopts[] =
    {
      {"device", required_argument,            0, 'd'},
      {"test",   no_argument,       &run_test_flag, 1},
      {"help",   no_argument,                  0, 'h'}
    };

    int option_index = 0;
    c = getopt_long(argc, argv, "d:th", lopts, &option_index);

    // end of options
    if (c == -1)
      break;

    switch (c)
    {
      case 0:
        break;
      case 'd':
        device_flag = 1;
        strcpy(device, optarg);
        break;
      case 't':
        run_test_flag = 1;
        break;
      case 'h':
        //print_usage();
        exit(EXIT_SUCCESS);
        break;
      case '?':
        printf("Error: Invalid option.\n");
        //print_usage();
        exit(EXIT_FAILURE);
      default:
        printf("Error: Invalid option %c.\n", c);
        //print_usage();
        exit(EXIT_FAILURE);
    }
  }
}


// --------------------------------------------------------
// main: entrance point for the program

int main(int argc, char** argv)
{
    rfdf class_obj;

  // process input arguments
  class_obj.parse_options(argc, argv);



  if (!class_obj.device_flag)
  {
    std::cout << "Device option required." << std::endl;
    return EXIT_FAILURE;
  }

  // run test loop
  if (class_obj.run_test_flag)
  {
    class_obj.test_transmit_loop();
  }

  // run main loop
  if (!class_obj.run_test_flag)
  {
    class_obj.main_loop();
  }

  return EXIT_SUCCESS;
}
