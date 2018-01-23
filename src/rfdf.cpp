/**********************************************************
rfdf.cpp

Description:
  Useful for transferring heading data from
  the Gizmo 2 board to ROS system

*/

// --------------------------------------------------------
// libraries

#include <iostream>
#include <time.h>
#include <stdlib.h>
// #include <ros.h>

using namespace std;


// --------------------------------------------------------
// ROS: message configuration and transmission




// --------------------------------------------------------
// test_loop: generate heading data

void send_data(float elevation, float azimuth)
{
  // sending data
  cout << "Sending: " << elevation << "," << azimuth << endl;
}

void test_sleep(int milliseconds)
{
  struct timespec req = {0};
  req.tv_sec = 0;
  req.tv_nsec = milliseconds * 1000000L;
  nanosleep(&req, (struct timespec *)NULL);
}

void test_loop()
{
  for (int i = -180; i <= 180; i++)
  {
    float elevation = abs(i % 91);
    float azimuth = i % 181;
    // send data
    send_data(elevation, azimuth);
    // sleep for 10 ms
    test_sleep(10);
  }
}

// --------------------------------------------------------
// main: entrance point for the program

int main(int argc, char** argv)
{
  
  // run test loop with simulated data
  test_loop();
  
  return EXIT_SUCCESS;
}

