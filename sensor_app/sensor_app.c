#include "../HMC5883L_driver/HMC5883L_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
  int fd;
  struct HMC5883L_UserData data;

  //open char device
  fd = open("/dev/HMC5883L_device", O_RDWR);
  if (fd < 0)
  {
    printf("Cannot open device file\n");
    return 0;
  }

  //fetch readings 10 times
  for(int i = 0; i < 10; ++i)
  {
    sleep(1);
    ioctl(fd, HMC5883L_GET_XYZ, (struct HMC5883L_UserData*) &data);
    printf("%d, %d, %d\n", data.x, data.y, data.z);
  }

  close(fd);

  return 1;
}