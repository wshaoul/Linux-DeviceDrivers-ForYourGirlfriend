# Lesson 3: Message Logging & GCC Extensions
## printk and dmesg
### printk
`printk` puts its contents in the kernel log ring bufer. They can be sent to a variety of different places based on which macro is used
```#define KERN_EMERG    "<0>" /* system is unusable            */
#define KERN_ALERT    "<1>" /* action must be taken immediately    */
#define KERN_CRIT    "<2>" /* critical conditions            */
#define KERN_ERR    "<3>" /* error conditions            */
#define KERN_WARNING    "<4>" /* warning conditions            */
#define KERN_NOTICE    "<5>" /* normal but significant condition    */
#define KERN_INFO    "<6>" /* informational                */
#define KERN_DEBUG    "<7>" /* debug-level messages
```

### dmesg
However, no matter the log level (the number associated with each macro above) the kernel's syslog daemon redirects messages to the log file `/var/log/kernel.log`
But the messages from this file are only passed to the user-space by the `dmesg` command,which dumps the kernel ring buffer into standard output (the terminal)

***
## GCC Extensions (shoutout Claude)
When you builda driver directly into the kernel (*not as a dynamically loadable `.ko` modue*) the kernel performs some optimizations:
 `__init`: 
- These only ever run once — at boot time to initialize things.
- After boot, they'll never be called again. So once the kernel finishes booting, there's no reason to keep that code sitting in RAM forever.
- GCC places all __init functions into a special "init section" of the kernel image, and after boot completes the kernel just frees that entire section. Free memory!

`__exit`:
- These are cleanup functions meant to run at shutdown.
- But think about it — if the system is shutting down anyway, everything is getting wiped out regardless. There's no process to clean up for, no users to protect.
- So the kernel goes one step further and doesn't even include the exit section in the built-in kernel image at all.
- Why waste space storing code that's only useful during a shutdown that already handles cleanup implicitly?

However, for our purposes of dynamically loading a modulee with `insmod or rmmod` on a system already running, __init and __exit are ignored markers.
- This is especially true for the `__exit` function -- we actually need to run rmmod to clean uo properly