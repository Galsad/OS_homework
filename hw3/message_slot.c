//
// Created by gal on 12/7/17.
//
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <asm/uaccess.h>    /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include "message_slot.h"
#include <linux/slab.h>
#include <linux/errno.h>

//#define NULL 0
#define DEVICE_RANGE_NAME "very_cool_dev"
#define MESSAGE_SIZE 128
#define MAJOR_NUM 244

MODULE_LICENSE("GPL");

// channel struct
struct channel{
    int channel_id;
    char buf[128];
    int minor;
};

struct channel_node{
    struct channel_node* next;
    struct channel* channel_info;
};


// global variables
struct channel_node* my_channel_list = NULL;
struct channel_node* current_channel_list = NULL;

static int num_of_channels = 0;
static int num_of_currents = 0;


static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
    //kmalloc(sizeof(struct channel_node));
    //kmalloc(sizeof(struct channel_node));

    //TODO - take care of exceptions
    // ioctl == channel id
    // command_id replace channels
    int minor = iminor(file->f_path.dentry->d_inode);

    // if 0 - no need to add new channel
    int add_new_node = 0;

    // TODO -- change it
    struct channel_node* new_channel_node = kmalloc(sizeof(struct channel_node), GFP_KERNEL);
    new_channel_node->channel_info->channel_id = ioctl_param;
    new_channel_node->next = NULL;

    struct channel_node* new_current_node = kmalloc(sizeof(struct channel_node), GFP_KERNEL);
    new_current_node->channel_info->channel_id = ioctl_param;
    new_current_node->next = NULL;

    if (my_channel_list == NULL){
        num_of_channels += 1;
        num_of_currents += 1;
        my_channel_list = new_channel_node;
        current_channel_list = new_current_node;
        return SUCCESS;
    }

    struct channel_node* current_channel_list_node = my_channel_list;

    // check if ioctl_param in channel my_channel_list
    while (current_channel_list_node != NULL){
        if (current_channel_list_node->channel_info->minor == minor && current_channel_list_node->channel_info->channel_id == ioctl_param){
            add_new_node = 1;
            break;
        }
        current_channel_list_node = current_channel_list_node->next;
    }

    if (add_new_node == 1){
        current_channel_list_node->next = new_channel_node;
    }

    // for the currents
    struct channel_node* current_node = current_channel_list;

    while (current_node != NULL){
        if (current_node->channel_info->minor == minor){
            // if the file exists - change the channel id
            // TODO -- what should i do with the buffer
            current_node->channel_info = current_channel_list_node->channel_info;
            return SUCCESS;
        }
    }

    // we need to add a new node to currents
    current_node->next = new_channel_node;
    current_node->next = new_current_node;

    return SUCCESS;
}


static int device_open( struct inode* inode, struct file*  file )
    {
        // DO NOTHING!
        return SUCCESS;
    }

static int device_release( struct inode* inode, struct file*  file)
    {
        printk("Invoking device_release(%p,%p)\n", inode, file);
        return SUCCESS;
    }

static ssize_t device_read( struct file* file, char __user* buffer, size_t length, loff_t* offset)
    {

    int minor = iminor(file->f_path.dentry->d_inode);

    // find the current channel of the file
    struct channel_node* current_node = current_channel_list;
    while (current_node != NULL){
        if (current_node->channel_info->minor == minor){
            break;
        }
        current_node = current_node->next;
    }

    if (current_node == NULL){
        printk("didn't find minor number");
        return -1;
    }

    int i;
    printk("Invoking device_write(%p,%d)\n", file, length);

    for( i = 0; i < length && i < MESSAGE_SIZE; ++i)
    {
        put_user(current_node->channel_info->buf[i], &buffer[i]);
    }

    // return the number of input characters used
    return i;
}


static ssize_t device_write( struct file* file, const char __user* buffer, size_t length, loff_t* offset)
    {

        int minor = iminor(file->f_path.dentry->d_inode);

        // find the current channel of the file
        struct channel_node* current_node = current_channel_list;
        while (current_node != NULL){
            if (current_node->channel_info->minor == minor){
                // we have found the current channel
                break;
            }

            current_node = current_node->next;
        }

        if (current_node == -1){
            printk("didn't find minor number");
            return -1;
        }

        printk("Invoking device_write(%p,%d)\n", file, length);
        int i = 0;
        for(i = 0; i < length && i < MESSAGE_SIZE; ++i)
            {
                get_user(current_node->channel_info->buf[i], &buffer[i]);
            }

        // return the number of input characters used
        return i;
    }


struct file_operations Fops =
        {
                .read           = device_read,
                .write          = device_write,
                .open           = device_open,
                .release        = device_release,
                .unlocked_ioctl = device_ioctl,
        };


static int __init simple_init(void)
{
    // Register driver capabilities. Set 42 because Ivgeni said so...
    int major = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

    // Negative values signify an error
    if( major < 0 )
    {
        printk( KERN_ALERT "%s registraion failed for  %d\n", DEVICE_FILE_NAME, major);
        return major;
    }

    printk(KERN_INFO "message_slot: registered major number %d\n", major);

    return 0;
}

static void __exit simple_cleanup(void)
{
    // Unregister the device
    // Should always succeed
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}
