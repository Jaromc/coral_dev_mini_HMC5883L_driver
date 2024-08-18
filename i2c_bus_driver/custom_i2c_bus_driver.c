#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/i2c-algo-bit.h>

#define ADAPTER_NAME "CUSTOMER_I2C_ADAPTER"
// GPIO pin to use for SCL
// This is the device path number (sys/class/gpio/gpio424)
#define SCL_GPIO 424
//GPIO pin to use for SDA
#define SDA_GPIO 400

static int I2C_Read_SCL(void *data)
{
  gpio_direction_input(SCL_GPIO);
  return gpio_get_value(SCL_GPIO);
}

static int I2C_Read_SDA(void *data)
{
  gpio_direction_input(SDA_GPIO);
  return gpio_get_value(SDA_GPIO);
}

static void I2C_Set_SCL(void *data, int state)
{
  gpio_direction_output(SCL_GPIO, state);
  gpio_set_value(SCL_GPIO, state);
}

static void I2C_Set_SDA(void *data, int state)
{
  gpio_direction_output(SDA_GPIO, state);
  gpio_set_value(SDA_GPIO, state);
}

static void I2C_Post_transfer(struct i2c_adapter *adapt)
{
  pr_info("Transfered data");
}

static int I2C_Init(void)
{
  int ret = 0;
  pr_info("Initialising I2C bus driver\n");

  do //single run loop to break on error
  {
    if(gpio_is_valid(SCL_GPIO) == false)
    {
      pr_err("SCL GPIO %d is not valid\n", SCL_GPIO);
      ret = -1;
      break;
    }
   
    if(gpio_is_valid(SDA_GPIO) == false)
    {
      pr_err("SDA GPIO %d is not valid\n", SDA_GPIO);
      ret = -1;
      break;
    }

    ret = gpio_request(SCL_GPIO, "SCL_GPIO");
    if(ret < 0)
    {
      pr_err("Error requesting SCL GPIO %d, %d\n", SCL_GPIO, ret);
      ret = -1;
      break;
    }

    ret = gpio_request(SDA_GPIO, "SDA_GPIO");
    if(ret < 0)
    {
      pr_err("Error requesting SDA GPIO %d, $d\n", SDA_GPIO, ret);
      gpio_free(SCL_GPIO);
      ret = -1;
      break;
    }
    
    gpio_direction_output(SCL_GPIO, 1);
    gpio_direction_output(SDA_GPIO, 1);
  } while(false);

  if (ret == 0)
  {
    pr_info("I2C bus driver Initialised\n");
  }

  return ret;  
}

struct i2c_algo_bit_data algo_bit_data = {
  .setsda = I2C_Set_SDA,
  .setscl = I2C_Set_SCL,
  .getscl = I2C_Read_SCL,
  .getsda = I2C_Read_SDA,
  .post_xfer = I2C_Post_transfer,
  .udelay = 5,
  .timeout = 100,
};

static struct i2c_adapter i2c_adapter = {
  .owner      = THIS_MODULE,
  .class      = I2C_CLASS_HWMON | I2C_CLASS_SPD,
  .name       = ADAPTER_NAME,
  .algo_data  = &algo_bit_data,
  .nr         = 5, // /dev/i2c-5
};

static int __init custom_i2c_driver_init(void)
{
  int ret = -1;
  
  ret = I2C_Init();
  if (ret < 0)
  {
    pr_info("Failed to add Custom Bus Driver");
    return ret;
  }
  
  ret = i2c_bit_add_numbered_bus(&i2c_adapter);
  pr_info("Custom Bus Driver Added\n");
  return ret;
}

static void __exit custom_i2c_driver_exit(void)
{
  gpio_free(SCL_GPIO);
  gpio_free(SDA_GPIO);

  i2c_del_adapter(&i2c_adapter);
  pr_info("Custom Bus Driver Removed\n");
}

module_init(custom_i2c_driver_init);
module_exit(custom_i2c_driver_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Custom I2C Bus driver taking advantage of the i2c-gpio driver");
MODULE_VERSION("1.00");
