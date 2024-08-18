#ifndef HMC5883L_DRIVER
#define HMC5883L_DRIVER

#include <linux/ioctl.h>

struct HMC5883L_UserData
{
  int x;
  int y;
  int z;
};

#define CHAR_DEVICE_FILENAME "HMC5883L_device"

#define IOCTL_BASE 100

#define HMC5883L_TESTING _IO(IOCTL_BASE,0)
#define HMC5883L_GET_XYZ _IOR(IOCTL_BASE,1,struct HMC5883L_UserData*)

#endif