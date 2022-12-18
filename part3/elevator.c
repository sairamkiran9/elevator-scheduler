#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/unistd.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>

MODULE_LICENSE("Dual BSD/GPL");

#define BUF_SIZE 4096
#define MODULE_PARENT NULL
#define MODULE_NAME "elevator"
#define MODULE_PERMISSIONS 0644

#define IDLE 0
#define UP 1
#define DOWN 2
#define LOADING 3
#define OFFLINE 4
#define MAX_PASSENGERS 10
#define MAX_LOAD 100
#define MAX_FLOORS 10

struct thread_parameter
{
    int stop;
    int current_state;
    int next_state;
    int current_floor;
    int next_floor;
    int wait_passengers;
    int passengers_serviced;
    int passenger_service_floor[MAX_FLOORS];
};

struct passengers
{
    struct list_head list;
    int type;
    int start_floor;
    int dest_floor;
};

int i;
int read_flag;
int elevator_start;
char *message;

struct thread_parameter elevator;
struct list_head passenger_queue[MAX_FLOORS];
struct list_head elevator_list;

// struct mutex proc_mutex;
struct mutex elevator_mutex;
struct mutex passenger_mutex;

struct task_struct *elevator_thread;
static struct proc_dir_entry *proc_entry;

int get_elevator_load(void);

char *state(int current_state)
{
    /**
     * @brief Returns state of the elevator
     *
     */
    static char str[8];

    switch (current_state)
    {
    case OFFLINE:
        sprintf(str, "OFFLINE");
        break;
    case IDLE:
        sprintf(str, "IDLE");
        break;
    case UP:
        sprintf(str, "UP");
        break;
    case DOWN:
        sprintf(str, "DOWN");
        break;
    case LOADING:
        sprintf(str, "LOADING");
        break;
    default:
        sprintf(str, "ERROR");
        break;
    }
    return str;
}

void init_passenger(void)
{
    /**
     * @brief init method to intialize values for elevator.
     *
     */
    elevator.stop = 0;
    elevator.current_state = OFFLINE;
    elevator.next_state = UP;
    elevator.current_floor = 1;
    elevator.next_floor = 1;
    elevator.wait_passengers = 0;
    elevator.passengers_serviced = 0;
    for (i = 0; i < MAX_FLOORS; i++)
    {
        elevator.passenger_service_floor[i] = 0;
    }

    int j = 0;
    while (j < MAX_FLOORS)
    {
        INIT_LIST_HEAD(&passenger_queue[j]);
        j++;
    }
    INIT_LIST_HEAD(&elevator_list);
}

void add_passenger(int type, int start, int dest)
{
    /**
     * @brief method to add passengers when syscall method issue_request is called.
     *
     */
    struct passengers *new_passenger;
    new_passenger = kmalloc(sizeof(struct passengers), __GFP_RECLAIM | __GFP_IO | __GFP_FS);
    new_passenger->type = type;
    new_passenger->start_floor = start;
    new_passenger->dest_floor = dest;
    mutex_lock_interruptible(&passenger_mutex);
    list_add_tail(&new_passenger->list, &passenger_queue[start - 1]);
    elevator.wait_passengers++;
    mutex_unlock(&passenger_mutex);
}

extern int (*STUB_start_elevator)(void);
int start_elevator(void)
{
    /**
     * @brief Syscall method to start the elevator
     *
     */
    if (elevator.current_state == OFFLINE)
    {
        printk("Starting elevator\n");
        elevator.current_state = IDLE;
        elevator.current_floor = 1;
        elevator_start = 1;
        elevator.stop = 0;
        elevator.next_state = UP;
        elevator.next_floor = 1;
        return 0;
    }
    else if (elevator.current_state != OFFLINE)
    {
        return 1;
    }
    else
    {
        return -ENOMEM;
    }
}

extern int (*STUB_stop_elevator)(void);
int stop_elevator(void)
{
    /**
     * @brief Syscall method to stop the elevator
     *
     */
    // if (elevator_start == 1)
    // {
    //     printk("Stopping elevator \n");
    //     elevator.current_state = OFFLINE;
    //     elevator.stop = 1;
    //     elevator_start = 0;
    //     return 0;
    // }
    // else
    // {
    //     return 1;
    // }
    if (elevator_start == 1)
    {
        while (get_elevator_load() != 0)
        {
            printk(KERN_NOTICE "Waiting for the elevator to upload passengers.\n");
        }
        printk("Stopping elevator \n");
        elevator.current_state = OFFLINE;
        elevator.stop = 1;
        elevator_start = 0;
        return 0;
    }
    else
    {
        return 1;
    }
}

extern int (*STUB_issue_request)(int, int, int);
int issue_request(int start_floor, int destination_floor, int type)
{
    /**
     * @brief Syscall method that adds passengers to wait list.
     *
     */
    printk(KERN_NOTICE "%s: Your int is %d %d %d\n", __FUNCTION__,
           start_floor, destination_floor, type);

    if (type > 3)
    {
        return 1;
    }
    else if (start_floor < 1 || start_floor > MAX_FLOORS)
    {
        return 1;
    }
    else if (destination_floor < 1 || destination_floor > MAX_FLOORS)
    {
        return 1;
    }
    else
    {
        add_passenger(type, start_floor, destination_floor);
        return 0;
    }
}

int goto_floor(int dest)
{
    /**
     * @brief Moves the elevator to the next floor (UP/DOWN)
     *
     */
    if (elevator.current_floor == dest)
    {
        printk("Elevator is on the floor %d\n", dest);
        return 0;
    }
    else
    {
        printk("Elevator going to floor %d\n", dest);
        ssleep(2);
        elevator.current_floor = dest;
        return 1;
    }
    return 0;
}

int get_elevator_capacity(void)
{
    /**
     * @brief Returns the total number of passengers of the elevator.
     *
     */
    struct passengers *passenger;
    struct list_head *pos;
    int size = 0;
    mutex_lock_interruptible(&elevator_mutex);
    list_for_each(pos, &elevator_list)
    {
        passenger = list_entry(pos, struct passengers, list);
        size++;
    }
    mutex_unlock(&elevator_mutex);
    return size;
}

int get_passenger_weights(int type)
{
    /**
     * @brief Returns the weight of the pet based on type.
     *
     */
    if (type == 0)
        return 15;
    else if (type == 1)
        return 45;
    else if (type == 2)
        return 5;
    else
        return 0;
}

int get_elevator_load(void)
{
    /**
     * @brief Returns the total weight of the elevator.
     *
     */
    struct passengers *passenger;
    struct list_head *pos;
    int weight = 0;
    mutex_lock_interruptible(&elevator_mutex);
    list_for_each(pos, &elevator_list)
    {
        passenger = list_entry(pos, struct passengers, list);
        weight += get_passenger_weights(passenger->type);
    }
    mutex_unlock(&elevator_mutex);
    return weight;
}

int load_elevator(void)
{
    /**
     * @brief Check if a new passenger can get onboarded to into the elevator
     * @return 1 if passenger can get loaded else 0
     */

    struct passengers *passenger;
    struct list_head *pos;
    if (MAX_LOAD == get_elevator_load())
        return 0;
    if (MAX_PASSENGERS == get_elevator_capacity())
        return 0;
    mutex_lock_interruptible(&passenger_mutex);
    list_for_each(pos, &passenger_queue[elevator.current_floor - 1])
    {
        passenger = list_entry(pos, struct passengers, list);
        if ((get_passenger_weights(passenger->type) + get_elevator_load() <= 100) &&
                (passenger->start_floor == elevator.current_floor) &&
                ((passenger->dest_floor > elevator.current_floor) &&
                 (elevator.next_state == UP)) ||
            ((passenger->dest_floor < elevator.current_floor) &&
             (elevator.next_state == DOWN)))
        {
            mutex_unlock(&passenger_mutex);
            return 1;
        }
    }
    mutex_unlock(&passenger_mutex);
    return 0;
}

int unload_elevator(void)
{
    /**
     * @brief Check if there is a passenger to get unloaded from elevator
     * @return 1 if passenger reached destination, else 0,
     */

    struct passengers *passenger;
    struct list_head *pos;
    mutex_lock_interruptible(&elevator_mutex);
    list_for_each(pos, &elevator_list)
    {
        passenger = list_entry(pos, struct passengers, list);
        if (passenger->dest_floor == elevator.current_floor)
        {
            mutex_unlock(&elevator_mutex);
            return 1;
        }
    }
    mutex_unlock(&elevator_mutex);
    return 0;
}

void unload_passengers(void)
{
    /**
     * @brief Unloads the passenger if the destination floor matches current floor.
     *
     */
    struct passengers *passenger;
    struct list_head *pos, *q;
    mutex_lock_interruptible(&elevator_mutex);
    list_for_each_safe(pos, q, &elevator_list)
    {
        passenger = list_entry(pos, struct passengers, list);
        if (passenger->dest_floor == elevator.current_floor)
        {
            elevator.passengers_serviced++;
            elevator.passenger_service_floor[passenger->start_floor - 1]++;
            list_del(pos);
            kfree(passenger);
        }
    }
    mutex_unlock(&elevator_mutex);
}

void load_passengers(int floor)
{
    /**
     * @brief Loads the passenger if the start floor is equal to the current floor
     * and load, size criterias are met.
     *
     */
    if (floor > MAX_FLOORS || floor < 1)
    {
        printk(KERN_NOTICE "Error: Invalid Floor!");
        return;
    }

    int weight = get_elevator_load();
    struct passengers *passenger;
    struct list_head *pos, *q;
    int i = floor - 1;
    mutex_lock_interruptible(&passenger_mutex);
    list_for_each_safe(pos, q, &passenger_queue[i])
    {
        passenger = list_entry(pos, struct passengers, list);
        if ((passenger->start_floor == floor) &&
            ((get_passenger_weights(passenger->type) + weight) <= 100))
        {
            struct passengers *new_passenger;
            new_passenger = kmalloc(sizeof(struct passengers),
                                    __GFP_RECLAIM | __GFP_IO | __GFP_FS);
            new_passenger->type = passenger->type;
            new_passenger->start_floor = passenger->start_floor;
            new_passenger->dest_floor = passenger->dest_floor;
            mutex_lock_interruptible(&elevator_mutex);
            list_add_tail(&new_passenger->list, &elevator_list);
            elevator.wait_passengers--;
            list_del(pos);
            kfree(passenger);
            mutex_unlock(&elevator_mutex);
            mutex_unlock(&passenger_mutex);
            return;
        }
    }
    mutex_unlock(&passenger_mutex);
}

char *get_elevator_status(void)
{
    /**
     * @brief Returns a string with current number of pets in the elevator
     *
     */
    static char str[24];
    int c = 0, d = 0, l = 0;
    struct passengers *passenger;
    struct list_head *pos, *q;
    mutex_lock_interruptible(&elevator_mutex);
    list_for_each_safe(pos, q, &elevator_list)
    {
        passenger = list_entry(pos, struct passengers, list);
        if (passenger->type == 0)
        {
            c++;
        }
        else if (passenger->type == 1)
        {
            d++;
        }
        else
        {
            l++;
        }
    }
    mutex_unlock(&elevator_mutex);
    sprintf(str, "%d C, %d D, %d L", c, d, l);
    return str;
}

char *get_floor_status(int floor)
{
    /**
     * @brief Prints passengers on each floor.
     *
     */
    static char *str1;
    static char *str2;
    struct passengers *passenger;
    struct list_head *pos;
    // mutex_lock_interruptible(&passenger_mutex);
    str1 = kmalloc(sizeof(char) * BUF_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
    str2 = kmalloc(sizeof(char) * BUF_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
    int i = MAX_FLOORS - 1;
    while (i >= 0)
    {
        int count = 0, c = 0, d = 0, l = 0;
        list_for_each(pos, &passenger_queue[i])
        {
            passenger = list_entry(pos, struct passengers, list);
            count++;
            if (passenger->type == 0)
            {
                c++;
            }
            else if (passenger->type == 1)
            {
                d++;
            }
            else
            {
                l++;
            }
        }
        if (floor == i + 1)
        {
            sprintf(str1, "[*] Floor %d: %d ", i + 1, count);
        }
        else
        {
            sprintf(str1, "[ ] Floor %d: %d ", i + 1, count);
        }
        strcat(str2, str1);
        while (c)
        {
            sprintf(str1, "%s ", "C");
            strcat(str2, str1);
            c--;
        }
        while (d)
        {
            sprintf(str1, "%s ", "D");
            strcat(str2, str1);
            d--;
        }
        while (l)
        {
            sprintf(str1, "%s ", "L");
            strcat(str2, str1);
            l--;
        }
        sprintf(str1, "\n");
        strcat(str2, str1);
        i--;
    }
    // mutex_unlock(&passenger_mutex);
    return str2;
}

int run_elevator(void *data)
{
    /**
     * @brief Main elevator method that has logic to traverse across the floors
     * and onboard/offboard the pets from the elevator.
     */
    int flag = 0;
    while (!kthread_should_stop())
    {
        switch (elevator.current_state)
        {
        case OFFLINE:
            break;
        case IDLE:
            elevator.next_state = UP;
            if (load_elevator() && !elevator.stop)
            {
                elevator.current_state = LOADING;
            }
            else
            {
                elevator.current_state = UP;
                elevator.next_floor = elevator.current_floor + 1;
            }
            break;
        case UP:
            goto_floor(elevator.next_floor);
            if (elevator.current_floor == MAX_FLOORS)
            {
                elevator.next_state = DOWN;
                elevator.current_state = DOWN;
            }
            if ((load_elevator() && !elevator.stop) || unload_elevator())
            {
                elevator.current_state = LOADING;
            }
            else if (elevator.current_floor == MAX_FLOORS)
            {
                elevator.next_floor = elevator.current_floor - 1;
            }
            // else if (elevator.stop && !get_elevator_capacity())
            // {
            //     elevator.current_state = OFFLINE;
            //     elevator.stop = 0;
            //     elevator.next_state = UP;
            // }
            // else if (!get_elevator_capacity())
            // {
            //     elevator.current_state = IDLE;
            //     elevator.stop = 0;
            //     elevator.next_state = UP;
            // }
            else
            {
                elevator.next_floor = elevator.current_floor + 1;
            }
            break;
        case DOWN:
            goto_floor(elevator.next_floor);
            if (elevator.current_floor == 1)
            {
                elevator.next_state = UP;
                elevator.current_state = UP;
            }
            if (elevator.stop && !get_elevator_capacity() && elevator.current_floor == 1)
            {
                elevator.current_state = OFFLINE;
                elevator.stop = 0;
                elevator.next_state = UP;
            }
            // else if (!get_elevator_capacity())
            // {
            //     elevator.current_state = IDLE;
            //     elevator.stop = 1;
            //     elevator.next_state = UP;
            // }
            else if ((load_elevator() && !elevator.stop) || unload_elevator())
            {
                elevator.current_state = LOADING;
            }
            else if (elevator.current_floor == 1)
            {
                elevator.next_floor = elevator.current_floor + 1;
            }
            else
            {
                elevator.next_floor = elevator.current_floor - 1;
            }
            break;
        case LOADING:
            ssleep(1);
            unload_passengers();
            if (load_elevator() && !elevator.stop)
            {
                load_passengers(elevator.current_floor);
                // flag++;
                // if (flag == 1)
                // {
                //     if (elevator.current_floor == MAX_FLOORS)
                //     {
                //         elevator.current_floor--;
                //     }
                //     else
                //     {
                //         elevator.current_floor++;
                //     }
                //     break;
                // }
            }
            elevator.current_state = elevator.next_state;
            if (elevator.current_state == DOWN)
            {
                if (elevator.current_floor == 1)
                {
                    elevator.next_state = UP;
                    elevator.current_state = UP;
                    elevator.next_floor = elevator.current_floor + 1;
                }
                else
                {
                    elevator.next_floor = elevator.current_floor - 1;
                }
            }
            else
            {
                if (elevator.current_floor == MAX_FLOORS)
                {
                    elevator.next_state = DOWN;
                    elevator.current_state = DOWN;
                    elevator.next_floor = elevator.current_floor - 1;
                }
                else
                {
                    elevator.next_state = UP;
                    elevator.current_state = UP;
                    elevator.next_floor = elevator.current_floor + 1;
                }
            }
            break;
        }
    }
    return 0;
}

int open_elevator(struct inode *sp_inode, struct file *sp_file)
{
    /**
     * @brief Method that creates procfs file
     *
     */
    printk(KERN_NOTICE "Executing open_elevator() \n");
    read_flag = 1;
    message = kmalloc(sizeof(char) * BUF_SIZE, __GFP_RECLAIM | __GFP_IO | __GFP_FS);
    if (message == NULL)
    {
        printk(KERN_ERR "Error: open_elevator");
        return -ENOMEM;
    }
    return 0;
}

static ssize_t read_elevator(struct file *sp_file, char __user *buff, size_t size, loff_t *offset)
{
    /**
     * @brief Method that updates procfs file of kernel module
     *
     */
    int msg_size;
    sprintf(message, "Elevator state: %s \nCurrent floor: %d \nCurrent weight: %d \
            \nElevator status: %s\nNumber of passengers: %d \
            \nNumber of passengers waiting: %d \
            \nNumber of passengers serviced: %d \n\n\n%s",
            state(elevator.current_state), elevator.current_floor,
            get_elevator_load(), get_elevator_status(), get_elevator_capacity(),
            elevator.wait_passengers, elevator.passengers_serviced,
            get_floor_status(elevator.current_floor));
    msg_size += strlen(message);
    read_flag = !read_flag;
    if (!read_flag)
    {
        printk(KERN_NOTICE "read_elevator() called.\n");
        if (copy_to_user(buff, message, msg_size))
        {
            return -EFAULT;
        }
        return msg_size;
    }
    return 0;
}

int release_elevator(struct inode *sp_inode, struct file *sp_file)
{
    /**
     * @brief Method that cleans the procfs
     *
     */
    printk(KERN_INFO "Executing release_elevator()\n");
    kfree(message); // Deallocates item
    return 0;
}

static struct proc_ops procfile_ops =
    {
        /**
         * @brief Procfs intialization
         *
         */
        .proc_open = open_elevator,
        .proc_read = read_elevator,
        .proc_release = release_elevator,
};

static int create_elevator(void)
{
    /**
     * @brief Method to intialize syscall methods
     *
     */
    STUB_start_elevator = start_elevator;
    STUB_issue_request = issue_request;
    STUB_stop_elevator = stop_elevator;
    return 0;
}

static void remove_elevator(void)
{
    /**
     * @brief Method to remove syscall methods
     *
     */
    STUB_start_elevator = NULL;
    STUB_issue_request = NULL;
    STUB_stop_elevator = NULL;
}

static int init_elevator(void)
{
    /**
     * @brief Create a elevator thread and procfs
     *
     */
    create_elevator();
    init_passenger();
    // create and wake the thread
    elevator_thread = kthread_run(run_elevator, NULL, "Elevator Thread");
    if (IS_ERR(elevator_thread))
    {
        printk(KERN_ERR "Error: run_elevator %ld \n", PTR_ERR(elevator_thread));
        return PTR_ERR(elevator_thread);
    }
    proc_entry = proc_create(MODULE_NAME, MODULE_PERMISSIONS, MODULE_PARENT, &procfile_ops);
    if (proc_entry == NULL)
    {
        printk(KERN_ERR "Error: proc_create\n");
        remove_proc_entry(MODULE_NAME, MODULE_PARENT);
        return -ENOMEM;
    }
    return 0;
}

static void cleanup_elevator(void)
{
    /**
     * @brief Stop elevator thread and procfs file
     *
     */
    int r;
    printk(KERN_NOTICE "Removing /proc/%s.\n", MODULE_NAME);
    remove_proc_entry(MODULE_NAME, MODULE_PARENT);
    remove_elevator();
    r = kthread_stop(elevator_thread);
    // mutex_destroy(&proc_mutex);
    mutex_destroy(&passenger_mutex);
    mutex_destroy(&elevator_mutex);
    // printk(KERN_NOTICE "Elevator thread stopped and elevator is offline now.\n");
    if (r != -EINTR)
    {
        printk(KERN_NOTICE "Elevator thread stopped and elevator is offline now.\n");
    }
    else
    {
        printk(KERN_ERR "Error: cleaning elevator");
    }
}

module_init(init_elevator);
module_exit(cleanup_elevator);