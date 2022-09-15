/******************************************************************************/
/*									
/*	这是一个标准字符设备驱动框架
/*		不包含platform
/*		可以编译模块 可以加载内核
/*
/*****************************************************************************/

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/*       物理地址       */
#define PERIPH_BASE     		     	(0x40000000)
/*	映射虚拟地址	*/
static void __iomem *DEMO_PLATFORM_ADDRESS;

struct test_dev{
	dev_t devid;		 /* 设备号 */
	struct cdev cdev; 	 /* cdev */
	struct class *class; 	 /* 类 */
	struct device *device;   /* 设备 */
	int    major; 		 /* 主设备号 */
	int    minor; 		 /* 次设备号 */	
	int    devices_munber	 /* 设备数量 */
	char   *devices_name	 /* 设备名字 */
}newchrled;

/*
*@description : 虚拟映射
*@param - address_len : 需要映射的长度
*@return : 无
*/
void devices_remap(void)
{
	/* 虚拟地址映射 */
	DEMO_PLATFORM_ADDRESS = ioremap(PERIPH_BASE, int address_len);
}

/*
* @description : 取消映射
* @return : 无
*/
void devices_unmap(void)
{
	/* 取消映射 */	
	iounmap(void __iomem *address);
}


/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int led_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &newchrled; /* 设置私有数据 */
	return 0;
}

/*
 * @description		: 从设备读取数据 
 * @param - filp 	: 要打开的设备文件(文件描述符)
 * @param - buf 	: 返回给用户空间的数据缓冲区
 * @param - cnt 	: 要读取的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 读取的字节数，如果为负值，表示读取失败
 */
static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	return 0;
}

/*
 * @description		: 向设备写数据 
 * @param - filp 	: 设备文件，表示打开的文件描述符
 * @param - buf 	: 要写给设备写入的数据
 * @param - cnt 	: 要写入的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue;
	unsigned char databuf[1];

	/*从用户态中获取消息 即：用户态发往内核的消息*/
	/************copy_from_user()***********/
	retvalue = copy_from_user(char *databuf, char *buf, size_t cnt);
	if(retvalue < 0) {
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}

	return 0;
}

/*
 * @description		: 关闭/释放设备
 * @param - filp 	: 要关闭的设备文件(文件描述符)
 * @return 			: 0 成功;其他 失败
 */
static int led_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/* 设备操作函数 */
static struct file_operations newchrled_fops = {
	.owner   = 	THIS_MODULE,
	.open    = 	led_open,
	.read    = 	led_read,
	.write   = 	led_write,
	.release = 	led_release,
};


/*
 * @description	: 驱动入口函数
 * @param 		: 无
 * @return 		: 无
 */
static int __init led_init(void)
{
	u32 val = 0;
	int ret;
	
	/* 1、寄存器地址映射 */
	devices_remap();   

	/* 注册字符设备驱动 */
	/* 1、创建设备号 */
	if (newchrled.major) 
	{		
		/*  定义了设备号 */
		newchrled.devid = MKDEV(newchrled.major, 0);	/* 设置次设备号为0 */
		ret = register_chrdev_region(newchrled.devid, newchrled.devices_munber,newchrled.devices_name);
		if(ret < 0) 
		{
			pr_err("cannot register %s char driver [ret=%d]\n",newchrled.devices_name, newchrled.devices_munber);
			goto fail_map;
		}
	} 
	else 
	{	/* 没有定义设备号 */
		ret = alloc_chrdev_region(&newchrled.devid, 0, newchrled.devices_munber, newchrled.devices_name);	/* 申请设备号 */
		if(ret < 0) 
		{
			pr_err("%s Couldn't alloc_chrdev_region, ret=%d\r\n", newchrled.devices_name, ret);
			goto fail_map;
		}
		newchrled.major = MAJOR(newchrled.devid);	/* 获取分配号的主设备号 */
		newchrled.minor = MINOR(newchrled.devid);	/* 获取分配号的次设备号 */
	}
	printk("newcheled major=%d,minor=%d\r\n",newchrled.major, newchrled.minor);	
	
	/* 2、初始化cdev */
	newchrled.cdev.owner = THIS_MODULE;
	cdev_init(&newchrled.cdev, &newchrled_fops);
	
	/* 3、添加一个cdev */
	ret = cdev_add(&newchrled.cdev, newchrled.devid, newchrled.devices_munber);
	if(ret < 0)
		goto del_unregister;
		
	/* 4、创建类 */
	newchrled.class = class_create(THIS_MODULE, newchrled.devices_name);
	if (IS_ERR(newchrled.class)) {
		goto del_cdev;
	}

	/* 5、创建设备 */
	newchrled.device = device_create(newchrled.class, NULL, newchrled.devid, NULL, newchrled.devices_name);
	if (IS_ERR(newchrled.device)) {
		goto destroy_class;
	}
	
	return 0;

destroy_class:
	class_destroy(newchrled.class);
del_cdev:
	cdev_del(&newchrled.cdev);
del_unregister:
	unregister_chrdev_region(newchrled.devid, newchrled.devices_munber);
fail_map:
	led_unmap();
	return -EIO;

}

/*
 * @description	: 驱动出口函数
 * @param 		: 无
 * @return 		: 无
 */
static void __exit led_exit(void)
{
	/* 取消映射 */
   	led_unmap();
   
	/* 注销字符设备驱动 */
	cdev_del(&newchrled.cdev);/*  删除cdev */
	unregister_chrdev_region(newchrled.devid, newchrled.devices_munber); /* 注销设备号 */

	device_destroy(newchrled.class, newchrled.devid);
	class_destroy(newchrled.class);/* 删除类 */
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DL");
MODULE_INFO(intree, "Y");
