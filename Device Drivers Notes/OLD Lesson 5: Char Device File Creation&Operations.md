# Lesson 5: Character Device Files - Creation & Operations
## Automatic creation of device files
In Linux Kernel version <2.4, Device File creation was done by the kernel itself. As the kernel evolved, however, developers realized device files should moreso be meant for users to deal with in user space.

In most Linux desktops, the `udev` daemon* takes two pieces of info to properly creates device files called the **Device Class** and **Device Info**.
- ****Daemon***: a program that runs as a background process, rather than being under the direct control of an interactive user 

The APIs in the header file  `<linux/device.h>` populate the `/sys` directory with device file entries' device class and info before letting the `udev` dameon handle it from there.

The APIs to use are:
```C
// 1. Creating the Device Class
struct class *cl = class_create(THIS_MODULE, "<device class name>");
// 2. Populating the Device Info
device_create(cl, NULL, first, NULL, "<device name format>", ...);
```
- `first` is the `dev_t` type with the corresponding `<major, minor>`

The reverse of the previous instructions (also in order) are:
```C
device_destroy(cl, first);
class_destroy(cl);
```
## File Operations
Any syscalls used on a regular file are applicable to device files as well. In Linux, a file is a file. The difference lies in the virtual file system (VFS), which decodes the file type & transfers file operations to the appropriate channels. In our case, it links a device file to a corresponding device driver.
The user must register file operations for a driver with the VFS,which is done in two steps:
1. Filling in a file operations structure (`struct file_operations wills_fops`) with the desired file operations (`my_open, my_close, my_write, ...`) and intialize the char device struct (`struct
cdev c_dev`) with these fops using `cdev_init()`.
2. Hand this struct to the VFS via `cdev_add()`.

Both `cdev_init()/add()` are declared in `<linux/cdev.h>`.


