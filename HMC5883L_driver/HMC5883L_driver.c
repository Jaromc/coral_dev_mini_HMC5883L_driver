#include "HMC5883L_driver.h"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define IOCTL_BASE 100

struct MyDeviceData
{
    struct cdev cdev;

    //...any other needed items
};

// /dev/i2c-5
#define I2C_BUS_DEVICE_NUMBER 5
#define SECONDARY_DEVICE_NAME "HMC5883L"
//the sensor will appear at this address 
#define HMC5883L_SECONDARY_ADDR 0x1E

dev_t dev = 0;
static struct class *dev_class;
static struct cdev char_cdev;

static struct i2c_adapter *i2c_adapter = NULL;
//Magnetometer structure
static struct i2c_client  *i2c_client_mag = NULL;

//////////////////////////////////////////////////////////////////////////
// Below: i2c coms between this driver and the HMC5883L sensor
//////////////////////////////////////////////////////////////////////////
 
//send data to the HMC5883L
static int I2C_Write(unsigned char *buf, unsigned int len)
{
  //send data to the bus driver via its master_xfer()
  int ret = i2c_master_send(i2c_client_mag, buf, len); 
  return ret;
}
 
 //read from HMC5883L.
 //out: buffer to store data
 //len: amount of data to read
static int I2C_Read(unsigned char *out, unsigned int len)
{
  int ret = i2c_master_recv(i2c_client_mag, out, len); 
  return ret;
}

static int HMC5883L_init_device(void)
{
  unsigned char buf[2] = {0};
  int ret;

  //Initialises the sensor as per the datasheet
  buf[0] = 0x00; buf[1] = 0x70;
  I2C_Write(buf, 2);
  //set gain to 1.3Ga
  buf[0] = 0x01; buf[1] = 0x20;
  I2C_Write(buf, 2);
  //continuous measurement mode
  buf[0] = 0x02; buf[1] = 0x00;
  I2C_Write(buf, 2);
  
  return 0;
}

//gets called onmce when the HMC5883L is connected 
static int HMC5883L_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
  HMC5883L_init_device();
  
  pr_info("HMC5883L initialised\n");
  
  return 0;
}

static int HMC5883L_remove(struct i2c_client *client)
{     
  pr_info("HMC5883L Removed\n");
  return 0;
}
 
static const struct i2c_device_id HMC5883L_id[] = {
  { SECONDARY_DEVICE_NAME, 0 },
  { }
};
MODULE_DEVICE_TABLE(i2c, HMC5883L_id);

static struct i2c_driver HMC5883L_driver = {
  .driver = {
      .name   = SECONDARY_DEVICE_NAME,
      .owner  = THIS_MODULE,
  },
  .probe = HMC5883L_probe,
  .remove = HMC5883L_remove,
  .id_table = HMC5883L_id,
};

static struct i2c_board_info HMC5883L_i2c_board_info = {
  I2C_BOARD_INFO(SECONDARY_DEVICE_NAME, HMC5883L_SECONDARY_ADDR)
};

int HMC5883L_driver_i2c_init(void)
{
  int ret = -1;
  i2c_adapter = i2c_get_adapter(I2C_BUS_DEVICE_NUMBER);
  
  if(i2c_adapter != NULL)
  {
      i2c_client_mag = i2c_new_device(i2c_adapter, &HMC5883L_i2c_board_info);
      
      if(i2c_client_mag != NULL)
      {
          i2c_add_driver(&HMC5883L_driver);
          ret = 0;
      }
      
      i2c_put_adapter(i2c_adapter);
  }

  return ret;
}

void HMC5883L_driver_i2c_exit(void)
{
  i2c_unregister_device(i2c_client_mag);
  i2c_del_driver(&HMC5883L_driver);
}

//////////////////////////////////////////////////////////////////////////
// Below: character device code for userspace ioctl access
//////////////////////////////////////////////////////////////////////////
static int char_dev_release(struct inode *inode, struct file *file)
{
  return 0;
}

static int char_dev_open(struct inode *inode, struct file *file)
{
  struct MyDeviceData *my_data =
             container_of(inode->i_cdev, struct MyDeviceData, cdev);

  file->private_data = my_data;

  return 0;
}

static ssize_t char_dev_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
  //for userspace read() operations 
  return 0;
}

static ssize_t char_dev_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
  //for userspace write() operations
  return len;
}

// Expects the start X, Y or Z address for the HMC5883L registers
int read_HMC5883L_addr(int addr)
{
  unsigned char buf[1] = {0};
  int outVal = 0;
  unsigned char data[2];
  int readRet = 0; 

  buf[0] = addr;
  data[0] = 0;
  data[1] = 0;
  I2C_Write(buf, 1);
  readRet = I2C_Read(data, 2);

  //combine to get a 16bit value
  outVal =  (int16_t)(data[1] | ((int16_t)(data[0] << 8)));

  pr_info("addr = %d, readRet = %d, data[0] = %d, data[1] = %d, outVal = %d\n", addr, readRet, data[0], data[1], outVal);

  return outVal;
}

void read_HMC5883L_values(int *x, int *y, int *z)
{
#ifdef 0
//for single value reading
  *x = read_HMC5883L_addr(0x03);
  *y = read_HMC5883L_addr(0x05);
  *z = read_HMC5883L_addr(0x07);
#else
  unsigned char buf[1] = {0};
  int outVal = 0;
  unsigned char data[6];
  int readRet = 0; 

  //set start location
  buf[0] = 0x03;
  I2C_Write(buf, 1);

  data[0] = 0;
  data[1] = 0;
  //read all 6 bytes
  readRet = I2C_Read(data, 6);

  //combine to form 16bit 2's compliment values 
  *x = (int16_t)(data[1] | ((int16_t)(data[0] << 8)));
  *y = (int16_t)(data[3] | ((int16_t)(data[2] << 8)));
  *z = (int16_t)(data[5] | ((int16_t)(data[4] << 8)));

  //pr_info("readRet = %d, 0 = %d, 1 = %d, 2 = %d, 3 = %d, 4 = %d, 5 = %d, 6 = %d\n", readRet, data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
  #endif
}

static long char_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
  struct MyDeviceData *my_data =
         (struct MyDeviceData*) file->private_data;
     
  struct HMC5883L_UserData outData;

  switch(cmd) 
  {
    case HMC5883L_TESTING:
      {
        // copy_from_user()
        // TODO : put the driver into a testing state
        pr_info("Value = %d\n", cmd);
      }
      break;
    case HMC5883L_GET_XYZ:
      { 
        //the user should call this no more often than 100ms.
        //We could instead be checking the ready state register and
        //blocking until it has data.

        read_HMC5883L_values(&outData.x, &outData.y, &outData.z);

        if(copy_to_user((struct HMC5883L_UserData*) arg, &outData, sizeof(struct HMC5883L_UserData)) )
        {
          pr_err("Data Read\n");
        }
      }
      break;
    default:
      break;
  }

  return 0;
}

static struct file_operations fops =
{
  .owner = THIS_MODULE,
  .unlocked_ioctl = char_dev_ioctl,
  .read = char_dev_read,
  .write = char_dev_write,
  .open = char_dev_open,
  .release = char_dev_release,
};

void char_dev_exit(void)
{
  device_destroy(dev_class,dev);
  class_destroy(dev_class);
  cdev_del(&char_cdev);
  unregister_chrdev_region(dev, 1);
}

int char_dev_init(void)
{
  int ret = -1;

  if((alloc_chrdev_region(&dev, 0, 1, "HMC5883L_Dev")) <0)
  {
    pr_err("Cannot allocate major number\n");
    return -1;
  }

  pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));

  cdev_init(&char_cdev,&fops);

  // Adding character device to the system
  if((cdev_add(&char_cdev,dev,1)) < 0)
  {
    pr_err("Cannot add the device to the system\n");
    goto r_class;
  }

  // "HMC5883L_class" will appear in /dev/
  if(IS_ERR(dev_class = class_create(THIS_MODULE,"HMC5883L_class")))
  {
    pr_err("Cannot create the struct class\n");
    goto r_class;
  }

  if(IS_ERR(device_create(dev_class,NULL,dev,NULL, CHAR_DEVICE_FILENAME)))
  {
    pr_err("Cannot create the Device 1\n");
    goto r_device;
  }
  
  pr_info("HMC5883L Char Device Driver Initialised\n");
  return 0;

  r_device:
    class_destroy(dev_class);
  r_class:
    unregister_chrdev_region(dev,1);
  
  return -1;
}

//called once to load the driver
static int __init HMC5883L_driver_init(void)
{
  // initialise the i2c communication between the HMC5883L sensor
  // and the i2c bus.
  if (HMC5883L_driver_i2c_init() < 0)
  {
    pr_info("HMC5883L Driver - Failed to initialise i2c\n");
    return -1;  
  }

  // intialises the char device for communication between a 
  // userspace application and this driver via ioctl calls
  if (char_dev_init() < 0)
  {
    pr_info("HMC5883L Driver - Failed to initialise ioctl\n");
    return -1;  
  }
  
  pr_info("HMC5883L Driver Added\n");
  return 0;
}

static void __exit HMC5883L_driver_exit(void)
{
  HMC5883L_driver_i2c_exit();
  char_dev_exit();
  
  pr_info("HMC5883L Driver Removed\n");
}
 
module_init(HMC5883L_driver_init);
module_exit(HMC5883L_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C driver for the HMC5883L magnetometer");
MODULE_VERSION("1.00");
