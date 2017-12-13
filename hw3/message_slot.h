#ifndef HW3_MESSAGE_SLOT_H_H
#define HW3_MESSAGE_SLOT_H_H

#define SUCCESS 0
#define FAIL -1
#define BUF_LEN 128
#define DEVICE_FILE_NAME "my_very_ultra_mega_cool_device"
#define MESSAGE_SIZE 128
#define MAJOR_NUM 244
#define DEVICE_RANGE_NAME "very_cool_dev"
#define IOCTL_COMMAND_ID _IOW(MAJOR_NUM, 0, unsigned long)


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/ioctl.h>

// destroy the file list and the channel list
//static int device_release( struct inode* inode, struct file*  file);

// iterate over file list and get the buffer content from the relevant file
//static ssize_t device_read( struct file* file, char __user* buffer, size_t length, loff_t* offset);

// iterate over file list and write to the buffer of the channel of the relevant file
//static ssize_t device_write( struct file* file, const char __user* buffer, size_t length, loff_t* offset);

// create a new file and add it to the file list
//static int device_open( struct inode* inode, struct file*  file );

// connect channel and file (update both lists)
//static long device_ioctl( struct   file* file, unsigned int   ioctl_command_id, unsigned long  ioctl_param );
#endif //HW3_MESSAGE_SLOT_H_H
