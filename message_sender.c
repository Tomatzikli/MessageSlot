/*
 * message_reader.c
 *
 *  Created on: 22 במאי 2020
 *      Author: תום תורג'מן
 */
#include "message_slot.h"

#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[])
{
  int file_desc;
  if(argc != 4)
  {
	  printf("Invalid arguments");
	  exit(1);
  }
  file_desc = open(argv[1], O_RDWR);
  if( file_desc < 0 )
  {
	  printf("Error occurred while opening device: %s\n", strerror(errno));
	  exit(1);
  }
  if(ioctl(file_desc, MSG_SLOT_CHANNEL, atoi(argv[2])) != 0)
  {
	  printf("Error occurred in ioctl: %s\n", strerror(errno));
	  exit(1);
  }

 
  if( write(file_desc, argv[3], strlen(argv[3])) != strlen(argv[3]) )
  {
	  printf("Error occurred while writing: %s\n", strerror(errno));
	  close(file_desc);
	  exit(1);
  }

 close(file_desc);

  return 0;
}
