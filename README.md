# MessageSlot
A character device kernel module through which processes communicate. The device has multiple message channels active concurrently, which can be used by multiple processes. The driver's major number is 240, and can be changed via the message_slot.h file. 

This piece of code was written for the "Operating systems" course at TAU.See msgslot.pdf for the full specifications.

To use first run Makefile, then use sodu insmod on the output in order to load the module into the kernel.
In order to make a new device, use mknod "/dev/X c 240 M" where X is the device name, and M is it's minor number.

# Message sender
Command line arguments:
1. argv[1]: message slot file path.
2. argv[2]: the target message channel id. Assume a non-negative integer.
3. argv[3]: the message to pass.

This program sends messages across a specified channel using the module.


# Message reader
Command line arguments:
1. argv[1]: message slot file path.
2. argv[2]: the target message channel id. Assume a non-negative integer.

This program reads messages from a specified channel using the module.

# Example session
1. As root (e.g., with sudo): Load (insmod) the message_slot.ko module.
2. As root: Create a message slot file using mknod.
3. As root: Change the message slot file’s permissions to make it readable and writable by your
user.
4. Invoke message_sender to send a message on some channel.
5. Invoke message_reader to read the message on the same channel.
