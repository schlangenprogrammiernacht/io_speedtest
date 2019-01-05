#include <iostream>
#include <vector>

#include <cstring>

#include <unistd.h>

#include <sys/types.h>
#include <sys/shm.h>

#include <sys/mman.h>
#include <fcntl.h>

struct SHMSetupMsg {
	char shm_name[32];
	size_t shm_size;
};


struct JobInfo
{
	char shm_name[32];
	size_t num_integers;
};


int main(void)
{
	int ret;

	std::cerr << "[worker] Hello World!" << std::endl;

	// read the setup message
	SHMSetupMsg setupMsg;
	ret = read(STDIN_FILENO, &setupMsg, sizeof(setupMsg));
	if(ret == -1) {
		std::cerr << "[worker] read(setupMsg) failed: " << strerror(errno) << std::endl;
		return 1;
	} else if(ret == 0) {
		std::cerr << "[worker] STDIN closed from other end, shutting down" << std::endl;
		return 0;
	}

	// create a shared memory block of the given size
	int shm_fd = shm_open(setupMsg.shm_name, O_RDWR, 0600);
	if(shm_fd == -1) {
		std::cerr << "[worker] shm_open() failed: " << strerror(errno) << std::endl;
		return 1;
	}

	ret = ftruncate(shm_fd, setupMsg.shm_size);
	if(ret == -1) {
		std::cerr << "[worker] ftruncate() failed: " << strerror(errno) << std::endl;
		return 1;
	}

	void *shared_mem = mmap(NULL, setupMsg.shm_size, PROT_READ, MAP_SHARED, shm_fd, 0);
	if(shared_mem == (void*)-1) {
		std::cerr << "[worker] mmap() failed: " << strerror(errno) << std::endl;
		return 1;
	}

	int *shared_ints = reinterpret_cast<int*>(shared_mem);

	while(true) {
		// read the job information
		JobInfo ji;
		ret = read(STDIN_FILENO, &ji, sizeof(ji));
		if(ret == -1) {
			std::cerr << "[worker] read(JobInfo) failed: " << strerror(errno) << std::endl;
			return 1;
		} else if(ret == 0) {
			std::cerr << "[worker] STDIN closed from other end, shutting down" << std::endl;
			return 0;
		}

		// calculate
		int sum = 0;
		for(size_t i = 0; i < 5 /*ji.num_integers*/; i++) {
			sum += shared_ints[i];
		}

		// write result
		ret = write(STDOUT_FILENO, &sum, sizeof(sum));
		if(ret == -1) {
			std::cerr << "[worker] write() failed: " << strerror(errno) << std::endl;
			return 1;
		} else if(ret != sizeof(sum)) {
			std::cerr << "[worker] Could not write all the data to STDOUT, shutting down with error" << std::endl;
			return 2;
		}
	}

	// shared memory teardown

	ret = munmap(shared_mem, setupMsg.shm_size);
	if(ret == -1) {
		std::cerr << "[worker] munmap() failed: " << strerror(errno) << std::endl;
		return 1;
	}

	close(shm_fd);
}
