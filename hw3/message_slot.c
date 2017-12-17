//
// Created by gal on 12/7/17.
//
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <asm/uaccess.h>    /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include "message_slot.h"
#include <linux/slab.h>
#include <linux/errno.h>
MODULE_LICENSE("GPL");

//TODO -- in case of errors need to return -1 with errno
//TODO -- need to free all variables
//

struct channel_node{
    struct channel_node* next;
    int channel_id;
    char buf[128];
    int minor;
    int has_data; // 0 is no data, 1 has data
};

struct file_node{
    int active_channel_id;
    int minor;
    struct file_node* next;
};


// global variables
struct channel_node* my_channel_list = NULL;
struct file_node* file_list = NULL;

static int num_of_files = 0;
static int num_of_written_channels = 0;


static long device_ioctl( struct   file* file, unsigned int   ioctl_command_id, unsigned long  ioctl_param )
{

    if (ioctl_command_id != MSG_SLOT_CHANNEL){
        printk("command id is not right!\n");
        return -EINVAL;
    }

    printk("in ioctl!\n");
    //TODO - take care of exceptions
    // ioctl == channel id
    int minor = iminor(file->f_path.dentry->d_inode);

    struct file_node* p = file_list;
    int has_file = 0;

    // there are no files in our list
    if (p == NULL){
        return -EINVAL;
    }

    while (p!=NULL){
        // find the relevant file
        if (p->minor == minor){
            has_file = 1;
            break;
        }
        p = p->next;
    }

    // can't find file
    if (has_file == 0){
        printk("file does not exits! \n");
        return -EINVAL;
    }

    // if 0 - no need to add new channel
    int add_new_node = 0;

    // first time we call this function - initialize channel_list and active_channel_list
    if (my_channel_list == NULL){
        printk("adding channel! \n");
        // create a new channel
        struct channel_node* new_channel_node = kmalloc(sizeof(struct channel_node), GFP_KERNEL);
        new_channel_node->channel_id = ioctl_param;
        new_channel_node->next = NULL;
        new_channel_node->minor = minor;
        new_channel_node->has_data = 0;
        p->active_channel_id = ioctl_param;
        my_channel_list = new_channel_node;
        printk("channel added successfully! \n");
        return SUCCESS;
    }

    // pointer to the beginning of the list
    struct channel_node* current_channel = my_channel_list;

    // check if ioctl_param in channel my_channel_list
    while (current_channel->next != NULL){
        // chane minor of channel to the one belongs to the file
        if (current_channel->channel_id == ioctl_param && current_channel->minor == minor){
            p->active_channel_id = ioctl_param;
            add_new_node = 1;
            return SUCCESS;
        }
        current_channel = current_channel->next;
    }

    // in case the last node has the channel_id
    if (current_channel->channel_id == ioctl_param && current_channel->minor == minor){
        current_channel->minor = minor;
        p->active_channel_id = ioctl_param;
        add_new_node = 1;
        return SUCCESS;
    }


    // the channel wasn't found in the list
    if (add_new_node == 0){
        printk("adding new channel!\n");
        struct channel_node* new_node = kmalloc(sizeof(struct channel_node), GFP_KERNEL);
        new_node->next = NULL;
        new_node->minor = minor;
        new_node->has_data = 0;
        new_node->channel_id = ioctl_param;
        p->active_channel_id = ioctl_param;
        current_channel->next = new_node;
        printk("Done adding channel! \n");
    }
    return SUCCESS;
}


static int device_open( struct inode* inode, struct file*  file )
    {
        printk("file opened!\n");
        int minor = iminor(inode);
        // TODO -- need to check if need to add something else to the struct
        // this is the first file we add to our database
        if (num_of_files == 0){
            num_of_files += 1;
            file_list = kmalloc(sizeof(struct file_node), GFP_KERNEL);
            file_list->next = NULL;
            file_list->minor = minor;
            file_list->active_channel_id = NULL;
            printk("file added to list!\n");
            return SUCCESS;
        }

        // this is not the first node of the list
        struct file_node* p = file_list;
        while (p->next != NULL){
            if(p->minor == minor) {
                printk("file already exists!\n");
                return SUCCESS;
            }
            p = p->next;
        }

        // last node in the list contains the relevant minor
        if (p->minor == minor){
            printk("file already exists!\n");
            return SUCCESS;
        }

        // file does not exist in file list - add it
        num_of_files += 1;
        struct file_node* new_node = kmalloc(sizeof(struct file_node), GFP_KERNEL);
        new_node->minor = minor;
        p->next = new_node;
        p->active_channel_id = NULL;
        return SUCCESS;
    }

static int device_release( struct inode* inode, struct file*  file)
    {
        // freeing all memory from lists

        printk("Invoking device_release(%p,%p)\n", inode, file);
        return SUCCESS;
    }

static ssize_t device_read( struct file* file, char __user* buffer, size_t length, loff_t* offset)
    {

    printk("in read file!\n");
    int minor = iminor(file->f_path.dentry->d_inode);
    int has_error = 0;

    // find the current channel of the file
    struct file_node* current_file_node = file_list;

    while (current_file_node != NULL){
        if (current_file_node->minor == minor){
            break;
        }
        current_file_node = current_file_node->next;
    }

    if (current_file_node == NULL){
        printk("didn't find minor number\n");
        return -EINVAL;
    }

    int active_channel = current_file_node->active_channel_id;

    printk("READ: active channel is: %d", active_channel);
    struct channel_node* current_channel_node = my_channel_list;

    while (current_channel_node != NULL){
        if (current_channel_node->minor == minor && current_channel_node->channel_id == active_channel){
            has_error = 1;
            break;
        }
        current_channel_node = current_channel_node->next;
    }

    if (has_error == 0){
        printk("problem in list of channels!\n");
        return -EINVAL;
    }

    if (num_of_written_channels == 0 || current_channel_node->has_data == 0){
        printk("no message is written!\n");
        return -EINVAL;
    }

    int i;
    for(i = 0; i < length && i < MESSAGE_SIZE; ++i)
    {
        if (i == MESSAGE_SIZE){
            printk("message is too long!\n");
            return -ENOSPC;
        }
        put_user(current_channel_node->buf[i], &buffer[i]);
    }

    // channel contains no message
    if ( i == 0) {
        return -EWOULDBLOCK;
    }

    // return the number of input characters used
    return i;
}


static ssize_t device_write( struct file* file, const char __user* buffer, size_t length, loff_t* offset)
{

    printk("in write file!\n");
    int minor = iminor(file->f_path.dentry->d_inode);
    int has_error = 0;

    // find the current channel of the file
    struct file_node* current_file_node = file_list;
    while (current_file_node != NULL){
        if (current_file_node->minor == minor){
           break;
        }
        current_file_node = current_file_node->next;
    }

    if (current_file_node == NULL){
        printk("didn't find minor number\n");
        return -EINVAL;
    }

    int active_channel = current_file_node->active_channel_id;
    printk("WRITE: active channel is: %d", active_channel);
    struct channel_node* current_channel_node = my_channel_list;


    while (current_channel_node != NULL){
        if (current_channel_node->minor == minor && current_channel_node->channel_id == active_channel){
            has_error = 1;
            break;
        }
        current_channel_node = current_channel_node->next;
    }

    if (has_error == 0){
        printk("problem in list of channels!\n");
        return -EINVAL;
    }

    // set data flag to 1
    current_channel_node->has_data = 1;
    int i;
    for(i = 0; i < length && i < MESSAGE_SIZE; ++i)
    {
        if (i == MESSAGE_SIZE){
            printk("message is too long!\n");
            return -EINVAL;
        }
        get_user(current_channel_node->buf[i], &buffer[i]);
    }
    num_of_written_channels += 1;
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
    // Register writedriver capabilities. Set 42 because Ivgeni said so...
    printk(KERN_INFO "initialize... \n");
    int rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

    // Negative values signify an error
    if( rc < 0 )
    {
        printk( KERN_ALERT "%s registraion failed for  %d\n", DEVICE_FILE_NAME, MAJOR_NUM);
        return rc;
    }

    printk( "Registeration is successful. \n");
    printk( "If you want to talk to the device driver,\n" );
    printk( "you have to create a device file:\n" );
    printk( "mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM );
    printk( "You can echo/cat to/from the device file.\n" );
    printk( "Dont forget to rm the device file and "
                    "rmmod when you're done\n" );


    return 0;
}

static void __exit simple_cleanup(void)
{
    // free both lists

    printk("freeing all memory!\n");

    struct channel_node* p1 = my_channel_list;
    struct file_node* p2 = file_list;

    struct channel_node* tmp_channel_node;
    struct file_node* tmp_file_node;

    int skip_channel = 0;
    int skip_file = 0;

    if (p1 == NULL){
        skip_channel = 1;
    }

    if (p2 == NULL){
        skip_file = 1;
    }

    while (skip_channel == 0 || p1->next == NULL){
        tmp_channel_node = p1;
        p1 = tmp_channel_node->next;
        kfree(tmp_channel_node);
    }

    if (skip_channel == 0){
        kfree(p1);
    }

    while (skip_file == 0 || p2->next == NULL){
        tmp_file_node = p2;
        p2 = tmp_file_node->next;
        kfree(tmp_file_node);
    }

    if (skip_file == 0){
        kfree(p2);
    }

    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

module_init(simple_init);
module_exit(simple_cleanup);
