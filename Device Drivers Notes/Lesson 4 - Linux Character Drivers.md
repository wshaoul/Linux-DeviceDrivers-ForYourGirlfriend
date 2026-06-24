# Lesson 4: Linux Character Drivers
If we write drivers for byte-oriented operations (or, in C lingo, character-oriented operations), then we refer to them as **character drivers.** 
- Since the majority of devices are byte-oriented, the majority of device drivers are character device drivers.

Examples include serial, audio, video, camera, and basic I/O drivers
- In fact, *all drivers that aren't storage or network device drivers are some type of character driver*
***
## The Connection
For any user-space application to operate on a byte-oriented device, it must use the corresponding char device driver in kernel space.
Using these drivers is done through char device files that are linked through the VFS. Thus,
	--> An application does usual file  operations on a device
	--> These operations are translated to the functions in the linked char device driver by the VFS
	--> These functions do the final low-level access to the actual device

### The four major entities involved in the connection
1. Application in user-space
2. Character Device File
3. Character Device Driver
4. Character Device

**These four entities are only connected if done so explicitly**:
- The *Application* connects to a *device file* via invoking an `open` syscall on that device file.
- The *Device File* is linked to the *Device Driver* by specific registrations by the Driver
- *Device Driver* is linked to the *Device* by its device-specific low-level operations

  ## Major & Minor \#
Although the connection between the application and the device file is based on the device file's name, **the device file and device driver's connection is based on the *number* of the device file.**
