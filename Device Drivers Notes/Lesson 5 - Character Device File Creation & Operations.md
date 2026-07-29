# Lesson 5: Character Device File Creation & Operations

## Core Shift: Policy Moved to Userspace

**The architectural change**: Early kernels (2.4) had the kernel automatically create `/dev` files via devfs APIs. Modern kernels (~2.6+) export device metadata to `/sys` instead, and **userspace daemons interpret it**. This separation of concerns means:
- Kernel writes `<major>:<minor>` info to `/sys/devices/.../dev`
- The `udev` daemon reads it and creates the appropriate `/dev/xyz` device file
- Drivers never directly create `/dev` entries—they just populate sysfs

This is why your driver needs explicit device registration calls now.

---

## Automatic Device File Creation via udev

### The Three-Step Registration Dance

**1. Register the major/minor range** (you likely did this in Part 4):
```c
alloc_chrdev_region(&first, 0, 1, "Will")
```
Allocates dynamic major (and minor 0, count 1). Stores in `dev_t first`.

**2. Populate the device class** (exposes to `/sys`):
```c
struct class *cl = class_create("chardrv");
```
Creates `/sys/class/chardrv/`. The kernel driver model organizes devices by class (think `tty`, `video`, `usb`, etc.). This is part of the **Linux device model**—a hierarchical registration system that lets the kernel track hardware relationships.

**3. Create the device under that class** (tells udev what to create):
```c
device_create(cl, NULL, first, NULL, "mynull");
```
This creates `/sys/class/chardrv/mynull/dev` containing the major:minor tuple. The udev daemon watches for these sysfs changes and creates `/dev/mynull` with appropriate permissions.

### Why This Matters

The **separation** lets you:
- Change device file naming/permissions via udev rules (no recompile)
- Handle dynamic device hotplug without kernel involvement
- Keep the kernel policy-agnostic

For **multiple minors**, use a loop:
```c
for (int i = 0; i < 4; i++) {
    device_create(cl, NULL, MKDEV(MAJOR(first), MINOR(first) + i), 
                  NULL, "mynull%d", i);
}
```
Creates `/dev/mynull0`, `/dev/mynull1`, etc.

### Cleanup (reverse order matters):
```c
device_destroy(cl, first);    // Remove sysfs entries
class_destroy(cl);             // Remove class
unregister_chrdev_region(first, 1);  // Release major/minor
```

---

## File Operations: Connecting Driver Functions to VFS

### The Bridge Pattern

When userspace calls `open()`, `read()`, `write()` on `/dev/mynull`, here's what happens:

1. **VFS** (Virtual FileSystem layer) intercepts the syscall
2. VFS checks `/dev/mynull`'s inode and sees it's a **character device** with major:minor
3. VFS looks up the **file_operations structure** registered for that major/minor
4. VFS dispatches the syscall to your driver's function (`my_read`, `my_write`, etc.)

### Registering File Operations

**Step 1: Fill the structure**
```c
static struct file_operations wills_fops = {
    .owner = THIS_MODULE,      // Prevent module unload while in use
    .open = my_open,
    .release = my_close,       // "release" not "close"—confusing but standard
    .read = my_read,
    .write = my_write
};
```

**Step 2: Initialize the character device structure**
```c
cdev_init(&c_dev, &wills_fops);
```
Links the cdev to your fops. The `cdev` is the kernel's internal structure for character devices.

**Step 3: Add it to the kernel's character device table**
```c
cdev_add(&c_dev, first, 1);  // Register 1 device starting at 'first'
```
Now VFS knows about your driver. When a syscall comes in for major/minor in range `[first, first+1)`, VFS uses your fops.

**Critical**: You **must** call `cdev_del()` before unloading, or stale entries hang around.

---

## The Null Driver: Practical Example

Here's the complete driver with annotations:

```c
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
    if (IS_ERR(cl = class_create(THIS_MODULE, "chardrv"))) {
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
```

---

## Build & Test (Ubuntu/Debian)

### Makefile (same as Part 4, but included for reference):
```makefile
obj-m := ofcd.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
```

### Test Commands (adjust for Ubuntu):
```bash
# Build
make

# Load module
sudo insmod ofcd.ko

# Check major number
# */proc/devices is a virtual file that lists all currently
# registered device drivers in the kernel, organized by device type.
cat /proc/devices | grep Will

# Verify udev created the device file
ls -la /dev/mynull

# Test it
# `sudo tee` runs as root so it has access to write /dev/null
echo "hello" | sudo tee /dev/mynull > /dev/null
sudo cat /dev/mynull

# Watch kernel logs
sudo dmesg | tail -10

# Unload
sudo rmmod ofcd

# Cleanup
make clean
```

### Expected Output:
```bash
$ cat /proc/devices | grep Will
...
256 chardrv
...

$ ls -la /dev/mynull
crw------- 1 root root 256, 0 <timestamp> /dev/mynull

$ echo "hello" | sudo tee /dev/mynull > /dev/null
hello

$ dmesg | tail -10
# ...unrelated output...
Hello! ofcd registered
Driver: open()
Driver: write()
Driver: close()
Driver: open()
Driver: read()
Driver: close()
```

The file appears created by udev automatically (wait ~1 second after `insmod` if it doesn't show immediately—udev is async).

---

## Critical Patterns & Assumptions

### 1. **Error Handling Cascade**
Notice the cleanup order in `ofcd_init()`: if any step fails, you unwind in **reverse order**. This prevents resource leaks. The pattern:
```
Allocate A → Create B (needs A) → Create C (needs B)
If C fails: destroy C → destroy B → free A
```
This is **mandatory** for drivers; resource leaks in kernel code are catastrophic.

**Assumption flagged**: You might think `device_destroy()` is optional—it's not. Without it, sysfs entries persist and udev won't recreate them on reload.

### 2. **char __user \*buf Pattern**
The `__user` annotation is sparse metadata telling kernel static checkers: "This pointer came from userspace; don't dereference it directly." You saw this in USB HID likely.

In later lessons (Part 6), you'll need `copy_from_user()` and `copy_to_user()` to safely cross the user/kernel boundary. This driver doesn't actually touch `buf`, so it's safe—but real drivers must.

### 3. **VFS Routing via cdev**
The cdev is the **bridge between registration and operations**. When you do:
- `cdev_init(cdev, fops)` — cdev points to your functions
- `cdev_add(cdev, dev_t, count)` — kernel's major/minor lookup table now has an entry

VFS uses the lookup to dispatch. This is why removing a module without `cdev_del()` is dangerous—VFS still points to deallocated function pointers.

### 4. **Why `/dev/null` Behavior is Unusual**
Your `my_write()` returns `len` (bytes consumed), but you never stored anything. Your `my_read()` returns 0 (EOF). So:
- `echo "x" > /dev/mynull` → write succeeds, data discarded
- `cat /dev/mynull` → immediate EOF

This mimics `/dev/null` but is **not** how regular files work. In Part 6, you'll implement buffering and data transfer.

---

## Real-World Connections

- **USB HID**: The HID driver you modified probably called `device_create()` to expose `/dev/hidraw*` entries. The mechanics are identical.
- **Character Device Verticals**: `tty`, `input`, `framebuffer` drivers all use this same pattern. Study `drivers/tty/serial/` or `drivers/input/` to see it at scale.
- **Hotplug**: When you plug in a USB stick, udev watches for device class creation and runs scripts. Your driver just populates sysfs; udev is the orchestrator.

---

## Flags for Part 6 & Beyond

- **`copy_from_user()` / `copy_to_user()`**: You'll need these to actually exchange data. The `__user *buf` parameters aren't touched here.
- **File position (`loff_t *off`)**: In `my_read()`, you ignore it. Real drivers track read position; Part 6 covers this.
- **Locking**: Multiple processes can open `/dev/mynull` concurrently. This driver doesn't protect state—you'll add mutexes/spinlocks in later parts.
