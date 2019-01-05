#include <iostream>
#include <vector>

#include <cstring>

#include <unistd.h>

#include <sys/types.h>
#include <sys/shm.h>

struct JobInfo
{
	int shm_id;
	size_t num_integers;
};


int main(void)
{
	int ret;

	std::cerr << "[worker] Hello World!" << std::endl;

	while(true) {
		// read the length
		JobInfo ji;
		ret = read(STDIN_FILENO, &ji, sizeof(ji));
		if(ret == -1) {
			std::cerr << "[worker] read(JobInfo) failed: " << strerror(errno) << std::endl;
			return 1;
		} else if(ret == 0) {
			std::cerr << "[worker] STDIN closed from other end, shutting down" << std::endl;
			return 0;
		}

		// attach shared memory
		void *shared_mem = shmat(ji.shm_id, NULL, 0);
		if(shared_mem == (void*)-1) {
			std::cerr << "[master] shmat() failed: " << strerror(errno) << std::endl;
			return 1;
		}

		int *shared_ints = reinterpret_cast<int*>(shared_mem);

		// calculate
		int sum = 0;
		for(size_t i = 0; i < ji.num_integers; i++) {
			sum += shared_ints[i];
		}

		// detach shared memory
		ret = shmdt(shared_ints);
		if(ret == -1) {
			std::cerr << "[master] shmdt() failed: " << strerror(errno) << std::endl;
			return 1;
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
}
