# Linux Drivers for Your Girlfriend

This repo has notes I kept along with some completed sample code from the lessons from [this online course](https://sysplay.github.io/books/LinuxDrivers/book/Content/Part01.html).

## Something to Note
- The course uses RedHat/Fedora-based system, whereas I was using an Ubuntu/Debian-based virtual machine. This causes some slight discrepancies with some of the content. For example:
    * ***In Lesson 2***: `ls /lib/modules/$(uname -r)/kernel/fs/fat` on my VM only returned the compressed `msdos.ko` file. The authors had this along with a compressed `fat.ko` and `vfat.ko` file.
    * ***In Lesson 3***: The author's kernel level messages (those output with `sudo dmesg`) were located in `/var/log/messages`; my messages were located in `/var/log/kern.log` and `/var/log/syslog`
