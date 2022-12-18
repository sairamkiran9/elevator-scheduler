#include <unistd.h>
#include <sys/syscall.h>

int main() {
//exactly four write system calls
	syscall(SYS_write, 1, "test.\n", 7);
	syscall(SYS_write, 1, "test.\n", 7);
	syscall(SYS_write, 1, "test.\n", 7);
	syscall(SYS_write, 1, "test.\n", 7);

	return 0;
}
