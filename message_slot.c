/*
 * message_slot.c
 *
 *  Created on: 22 במאי 2020
 *      Author: תום תורג'מן
 */


// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include "message_slot.h"
#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>

MODULE_LICENSE("GPL");


// The message the device will give when asked
typedef struct
{
	int minor;
	int total_channels;
	char** buffers;
	int* channel;
	int* buff_size;

}Minor_slot;

static Minor_slot* message_slot = NULL;
static int total_slots = 0;

//================== DEVICE FUNCTIONS ===========================
int search_minor(int minor_num)
{
	int i;
	for(i = 0; i < total_slots; i++)
	{
		if( message_slot[i].minor == minor_num)
			return i;
	}
	return -1;
}
int search_channel(int minor_pos, int channel_num)
{
	int i;
	if(minor_pos >= 0)
	{
		for(i = 0; i < message_slot[minor_pos].total_channels; i++)
		{
			if( message_slot[minor_pos].channel[i] == channel_num)
				return i;
		}
	}
	return -1;
}
static int device_open( struct inode* inode,
                        struct file*  file )
{

  int minor_num = iminor(inode);
  file->private_data = (void*)0;//set channel to 0
  if(message_slot == NULL)
  {
	//create message_slot
	  message_slot = kmalloc(sizeof(Minor_slot), GFP_KERNEL);
	  if(!message_slot)//allocation failed
	  {
		  return -ENOMEM;
	  }
	  message_slot[0].minor = minor_num;
	  message_slot[0].buffers = NULL;
	  message_slot[0].channel = NULL;
	  message_slot[0].buff_size = NULL;
	  message_slot[0].total_channels = 0;
	  total_slots++;
  }
  else
  {
	  //Search if the minor has already been added.
	  if(search_minor(minor_num) == -1)//minor number is new
	  {
		  message_slot = krealloc(message_slot, (total_slots+1)*sizeof(Minor_slot), GFP_KERNEL); // reallocating data
		  if(!message_slot)//allocation failed
		  {
			  return -ENOMEM;
		  }
		  message_slot[total_slots].minor = minor_num;
		  message_slot[total_slots].buffers = NULL;
		  message_slot[total_slots].channel = NULL;
		  message_slot[total_slots].buff_size = NULL;
		  message_slot[total_slots].total_channels = 0;
		  total_slots++;
	  }
  }

  return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
  return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{
  int i;
  int buffersize;
  int minor = iminor(file->f_inode);
  int channel = (int) (uintptr_t)file->private_data;
  int minor_pos = search_minor(minor);
  int channel_pos = search_channel(minor_pos, channel);
  if(channel == 0)//if no channel has been set
  	  return -EINVAL;
  buffersize = message_slot[minor_pos].buff_size[channel_pos];
  if(buffersize == 0)// no msg exists
	  return -EWOULDBLOCK;
  if(buffersize > length)// buffer too small
	  return -ENOSPC;
  for( i = 0;i < buffersize ; ++i)
  {
  	if(put_user(message_slot[minor_pos].buffers[channel_pos][i], &buffer[i])!= 0) // storing the i'th bit
  		return -EFAULT;//if the value was not stored correctly
  }
  return buffersize; // return the size of bytes read, the buffer's size
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)

{
  int i;
  int minor = iminor(file->f_inode);
  int channel = (int) (uintptr_t)file->private_data;
  int minor_pos = search_minor(minor);
  int channel_pos = search_channel(minor_pos, channel);
  if(channel == 0)//if no channel has been set
	  return -EINVAL;
  if(length <= 0 || length > BUF_LEN)
	  return -EMSGSIZE;
  message_slot[minor_pos].buff_size[channel_pos] = length;//changing buffer size
  for( i = 0;i < length; ++i)
  {
	if(get_user(message_slot[minor_pos].buffers[channel_pos][i], &buffer[i])!= 0) // storing the i'th bit
		return -EFAULT;//if the value was not stored correctly
  }

  return length;
}

//----------------------------------------------------------------

static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
  int minor_num = iminor(file->f_inode);
  int minor_loc; // location of minor in the array
printk("0");
  if(message_slot == NULL)
	  return -EBADF; // returning bad file number since there were no calls to open
printk("1");

  if((MSG_SLOT_CHANNEL == ioctl_command_id) && (ioctl_param > 0)) // checking for valid command
  {
	  minor_loc = search_minor(minor_num);
	  if(minor_loc != -1)
	  {
		  if(message_slot[minor_loc].total_channels ==  0)// have to init the arrays
		  {
			  message_slot[minor_loc].buffers = kmalloc(sizeof(char*), GFP_KERNEL);
			  if(!message_slot[minor_loc].buffers)//allocation failed
			  	  {
			  		  return -ENOMEM;
			  	  }
			  message_slot[minor_loc].buffers[0] = kmalloc(BUF_LEN*sizeof(char), GFP_KERNEL);//allocate a buffer
			  if(!message_slot[minor_loc].buffers[0])//allocation failed
				  {
					  return -ENOMEM;
				  }
			  message_slot[minor_loc].channel = kmalloc(sizeof(int), GFP_KERNEL);
			  if(!message_slot[minor_loc].channel)//allocation failed
				  {
					  return -ENOMEM;
				  }
			  message_slot[minor_loc].channel[0] = ioctl_param;
			  message_slot[minor_loc].buff_size = kmalloc(sizeof(int), GFP_KERNEL);
			  if(!message_slot[minor_loc].buff_size)//allocation failed
				  {
					  return -ENOMEM;
				  }
			  message_slot[minor_loc].buff_size[0] = 0;
			  message_slot[minor_loc].total_channels++;
		  }
		  else // search if channel exists
		  {
			  if(search_channel(minor_loc, ioctl_param) == -1)//channel not found.  reallocate arrays
			  {
				  message_slot[minor_loc].buffers = krealloc(message_slot[minor_loc].buffers, (message_slot[minor_loc].total_channels +1)*sizeof(char*), GFP_KERNEL);
				  if(!message_slot[minor_loc].buffers)//allocation failed
					  {
						  return -ENOMEM;
					  }
				  message_slot[minor_loc].buffers[message_slot[minor_loc].total_channels] = kmalloc(BUF_LEN*sizeof(char), GFP_KERNEL);//allocate a buffer
				  if(!message_slot[minor_loc].buffers[message_slot[minor_loc].total_channels])//allocation failed
					  {
						  return -ENOMEM;
					  }
				  message_slot[minor_loc].channel = krealloc(message_slot[minor_loc].channel, (message_slot[minor_loc].total_channels +1)*sizeof(int), GFP_KERNEL);
				  if(!message_slot[minor_loc].channel)//allocation failed
					  {
						  return -ENOMEM;
					  }
				  message_slot[minor_loc].channel[message_slot[minor_loc].total_channels] = ioctl_param;

				  message_slot[minor_loc].buff_size = krealloc(message_slot[minor_loc].buff_size, (message_slot[minor_loc].total_channels +1)*sizeof(int), GFP_KERNEL);;
				  if(!message_slot[minor_loc].buff_size)//allocation failed
					  {
						  return -ENOMEM;
					  }
				  message_slot[minor_loc].buff_size[message_slot[minor_loc].total_channels] = 0;
				  message_slot[minor_loc].total_channels++;
			  }
		  }
	  }
	  else
	  {
		  return -EBADF; // minor not found means there was no call to open with the minor number
	  }

  }
  else
	  return -EINVAL;
  file->private_data = (void*)ioctl_param;//set channel
  return SUCCESS;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
{
  .owner	  = THIS_MODULE,
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  .release        = device_release,
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init init(void)
{
  int rc = -1;

  // Register driver capabilities. Obtain major num
  rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

  // Negative values signify an error
  if( rc < 0 )
  {
    printk( KERN_ALERT "%s registraion failed for  %d\n",
                       DEVICE_FILE_NAME, MAJOR_NUM );
    return rc;
  }

// device created

  return 0;
}
//---------------------------------------------------------------
void free_data(void)
{
	int i , j;
	for(i = 0; i < total_slots ; i++)
	{
		for(j = 0; j < message_slot[i].total_channels; j++)
		{
			kfree(message_slot[i].buffers[j]);//release buffers
		}
		if(message_slot[i].total_channels > 0)//release minor slot allocated memory
		{
			kfree(message_slot[i].buffers);
			kfree(message_slot[i].buff_size);
			kfree(message_slot[i].channel);
		}
	}
	kfree(message_slot);
}
//---------------------------------------------------------------
static void __exit cleanup(void)
{
  // Unregister the device
  // free data
  free_data();
  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(init);
module_exit(cleanup);

//========================= END OF FILE =========================
