#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>     // MKDEV, MAJOR, MINOR
#include <linux/fs.h>         // file_operations, inode
#include <linux/device.h>     // class_create, device_create
#include <linux/cdev.h>       // cdev_init, cdev_add

static dev_t first;
static struct cdev c_dev;
static struct class *cl;

/* File operation handlers */
static int my_open(struct inode *i, struct file *f)
{
    printk(KERN_INFO "Driver: open()\n");
    return 0;  // Success
}

static int my_close(struct inode *i, struct file *f)
{
    printk(KERN_INFO "Driver: close()\n");
    return 0;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    printk(KERN_INFO "Driver: read()\n");
    return 0;  // Return 0 bytes (like /dev/null)
}

static ssize_t my_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    printk(KERN_INFO "Driver: write()\n");
    return len;  // Pretend we consumed all bytes
}

static struct file_operations wills_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_close,
    .read = my_read,
    .write = my_write
};

static int __init ofcd_init(void)
{
    int ret;
    struct device *dev_ret;

    printk(KERN_INFO "Hello! ofcd registered\n");
    
    /* Step 1: Allocate major/minor */
    if ((ret = alloc_chrdev_region(&first, 0, 1, "Will")) < 0) {
        return ret;
    }
    
    /* Step 2: Create device class */
    if (IS_ERR(cl = class_create("chardrv"))) {
        unregister_chrdev_region(first, 1);
        return PTR_ERR(cl);
    }
    
    /* Step 3: Create device under class (udev will pick this up) */
    if (IS_ERR(dev_ret = device_create(cl, NULL, first, NULL, "mynull"))) {
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        return PTR_ERR(dev_ret);
    }

    /* Step 4: Link file operations to character device */
    cdev_init(&c_dev, &wills_fops);
    if ((ret = cdev_add(&c_dev, first, 1)) < 0) {
        device_destroy(cl, first);
        class_destroy(cl);
        unregister_chrdev_region(first, 1);
        return ret;
    }
    
    return 0;
}

static void __exit ofcd_exit(void)
{
    cdev_del(&c_dev);
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    printk(KERN_INFO "Goodbye! ofcd unregistered\n");
}

module_init(ofcd_init);
module_exit(ofcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Will Shaoul <williamsh4011@gmail.com>");
MODULE_DESCRIPTION("Our First Character Driver");