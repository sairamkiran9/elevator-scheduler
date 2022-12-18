#include <linux/init.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/moduleparam.h>
// #include <asm/uaccess.h>

MODULE_LICENSE("Dual BSD/GPL");

#define BUF_SIZE 256
#define MODULE_PARENT NULL
#define MODULE_NAME "my_timer"
#define MODULE_PERMISSIONS 0644

static int count = 0;
static int readFlag;
static char *string1;
static char *string2;

long elapsedSec;
long elapsedNS;

static struct timespec64 currentTime;
static struct proc_dir_entry *proc_entry;

static struct Time
{
    long sec;
    long nsec;
} time;

int open_timer(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Executing open_timer()\n");

    count++;
    readFlag = 1;

    string1 = kmalloc(sizeof(char) * BUF_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
    string2 = kmalloc(sizeof(char) * BUF_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
    if (string1 == NULL || string2 == NULL)
    {
        printk(KERN_WARNING "Error: could not allocate memory for a string.");
        return -ENOMEM;
    }
    ktime_get_ts64(&currentTime);
    sprintf(string1, "Current time: %lld.%ld\n", currentTime.tv_sec, currentTime.tv_nsec);
    printk(KERN_NOTICE "Current time: %lld.%ld\n", currentTime.tv_sec, currentTime.tv_nsec);
    if (currentTime.tv_nsec - time.nsec < 0)
    {
        elapsedSec = currentTime.tv_sec - time.sec - 1;
        elapsedNS = currentTime.tv_nsec - time.nsec + 1000000000;
    }
    else
    {
        elapsedSec = currentTime.tv_sec - time.sec;
        elapsedNS = currentTime.tv_nsec - time.nsec;
    }

    if (count > 1)
    {
        sprintf(string2, "Elapsed time: %ld.%ld\n", elapsedSec, elapsedNS);
        strcat(string1, string2);
    }

    time.sec = currentTime.tv_sec;
    time.nsec = currentTime.tv_nsec;

    return 0;
}

static ssize_t read_timer(struct file *sp_file, char __user *buffer, size_t size, loff_t *offset)
{
    int msg_size = strlen(string1);
    readFlag = !readFlag;
    if (!readFlag)
    {
        printk(KERN_INFO "Executing read_timer()\n");
        if (copy_to_user(buffer, string1, msg_size))
            return -EFAULT;
        return msg_size;
    }
    return 0;
}

int release_timer(struct inode *sp_inode, struct file *sp_file)
{
    printk(KERN_INFO "Executing release_timer()\n");
    kfree(string1); // Deallocates item
    kfree(string2); // Deallocates item
    return 0;
}

static struct proc_ops procfile_ops =
    {
        .proc_open = open_timer,
        .proc_read = read_timer,
        .proc_release = release_timer,
};

static int my_timer_init(void)
{
    proc_entry = proc_create(MODULE_NAME, MODULE_PERMISSIONS, MODULE_PARENT, &procfile_ops);
    if (proc_entry == NULL)
        return -ENOMEM;

    return 0;
}

static void my_timer_cleanup(void)
{
    printk(KERN_INFO "Removing /proc/%s.\n", MODULE_NAME);
    remove_proc_entry(MODULE_NAME, MODULE_PARENT);
}

module_init(my_timer_init);
module_exit(my_timer_cleanup);
