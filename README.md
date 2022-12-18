## Project-2-group-9

### Kernal Module Programming

- <b>Course</b> &nbsp;&nbsp;&nbsp; : COP 4610 Operating Systems
- <b>Professor</b>  : Dr. Andy Wang
- <b>Group</b>  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp; : 9
- <b>Group Members</b>  :
    - SM22BT &nbsp;: Sri Sai Ram Kiran Muppana 
    - SG22BX &nbsp;: Shanmukh Vishnuvardhan Rao Gongalla
    - MET19D &nbsp;: Matthew Torres

#### Divison of Labor

| NAME     | PART       |
|------    |----------- |
| Muppana  |    2,3     |
| Gongalla |     3      |
| Torres   |    1,3     |


#### List of Files

```
Part1
---------
empty.c       : A C file that contains a basic, empty main function.
part1.c       : A C file that contains 4 system calls.
empty.trace   : Log file for empty C program
part1.trace   : Log file for C program with 4 system calls
                 

Part2
---------
Makefile
 - make: Compiles the my_timer.c and generates the kernel module (.ko files).
 - make clean: Removes the kernel modules from the directory.
my_timer.c    : A kernel module that stores the time since the UNIX epoch and the elapsed time since the my_timer proc file was last opened.


Part3
---------
Makefile
 - make: Compiles the syscall.c, elevator.c and generates the kernel module (.ko files).
 - make clean: Removes the kernel modules from the directory.
syscalls.c    : Wrapper syscall defintion functions 
elevator.c    : Complete elevator program logic, elevator proc fs implementatin and system call methods.

```
#### Makefile Description

- `obj-m +=` Tells the kbuild what objects in the directory to build
- `KERNELDIR` Specifies the directory of the kernel build
- `default` Builds target external module
- `clean` Removes all generated files in the module directory 

#### Commands to add the kernel modules

- Run `$ make` to build 
- Run `$ make clean` to clean the generated module files.
- To insert the kernel module => run `$ sudo insmod module_name.ko`
- To check if the module is inserted => run `$ sudo lsmod | grep module_name`
- To remove the inserted kernel module -> run `$ sudo rmmod module_name`
- To see kernel logs => run `$ sudo dmesg -w`

#### Commands to execute
- <b> Part1: </b> (For empty and part1 as filename) Run `$ gcc -o filename.x filename.c` to build.
--Run $ `strace -o filename.trace ./filename.x` to return .trace files. part1 should have 4 more syscalls than empty
--Run `strace ./filename.x` to check this as well. (empty.x will have several syscalls by default)
- <b> Part2: </b> After adding kernel module, run `$ cat /proc/my_timer`.
- <b> Part3: </b> After adding kernel module, run. 
-- `$ ./producer no_of_passengers` - To generate passengers.
-- `$ ./consumer --start` - To start the elevator.
-- `$ ./consumer --stop` - To stop the elevator.
-- `$ cat /proc/elevator` - To get the status output of the elevator.

#### Known Bugs and Unfinished Portions
- Elevator sometimes crashes in loading state i.e. elevator cannot goto next floor if the passenger cannot onboard/offboard
- Elevator falls into deadlocks
- cat /proc/elevator sometimes crashes
