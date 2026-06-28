// our first driver's code

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h> // analogous to stdio.h

// constructor of our module called with __init
static int __init ofd_init(void) {
    // analogous to printf
    printk(KERN_INFO "Namaste: ofd registered\n");

    /*  NOTE: without "\n" in the printk statement, the 
        string is not printed until an internal buffer is
        flushed -- that is, when the module is removed when
        it is currently loaded or when it is loaded AFTER it's
        been removed (if it hasn't been removed, the below 
        statement would not have been stored in the buffer in 
        the first place)
    */
    return 0;
}

// Destructor, called with __exit
static void __exit ofd_exit(void) {
    printk(KERN_INFO "Gbye: ofd unregistered\n");
}

// macros included by module.h that specific our functions as
// constructors or destructors (called with insmod and rmmod)
module_init(ofd_init);
module_exit(ofd_exit);

// our module's signature
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Will Shaoul wshaoul@gmail.com");
MODULE_DESCRIPTION("Our First Driver");