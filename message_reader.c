
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
  int msg_length;
  char buffer[BUF_LEN+1]; // +1 for null terminating character
  if(argc != 3)
  {
	  printf("Invalid arguments");
	  exit(1);
  }
  file_desc = open(argv[1], O_RDONLY);
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
  msg_length = read(file_desc, buffer, BUF_LEN);
  if(msg_length == -1)
  {
	  printf("Error occurred while reading: %s\n", strerror(errno));
	  close(file_desc);
	  exit(1);
  }
  close(file_desc);
  buffer[msg_length] = '\0';

  printf("%s" , buffer);
  return 0;
}
