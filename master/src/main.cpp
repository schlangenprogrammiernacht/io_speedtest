#include <iostream>
#include <array>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <errno.h>

#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <sys/mman.h>
#include <fcntl.h>

#include "Stopwatch.h"

#define PIPE_READ 0
#define PIPE_WRITE 1

struct SHMSetupMsg {
	char shm_name[32];
	size_t shm_size;
};


struct JobInfo
{
	size_t num_integers;
};


/*
 * Thanks to https://stackoverflow.com/a/12839498 for the basis of this function
 */
int createChild(const char* szCommand, char* const aArguments[], int *procStdinFd, int *procStdoutFd, int *procStderrFd)
{
	int aStdinPipe[2];
	int aStdoutPipe[2];
	int aStderrPipe[2];
	int nChild;
	int nResult;

	if (pipe(aStdinPipe) < 0) {
		perror("allocating pipe for child input redirect");
		return -1;
	}
	if (pipe(aStdoutPipe) < 0) {
		close(aStdinPipe[PIPE_READ]);
		close(aStdinPipe[PIPE_WRITE]);
		perror("allocating pipe for child output redirect");
		return -1;
	}

	if (pipe(aStderrPipe) < 0) {
		close(aStdinPipe[PIPE_READ]);
		close(aStdinPipe[PIPE_WRITE]);
		close(aStderrPipe[PIPE_READ]);
		close(aStderrPipe[PIPE_WRITE]);
		perror("allocating pipe for child error redirect");
		return -1;
	}

	nChild = fork();
	if (0 == nChild) {
		// child continues here

		// redirect stdin
		if (dup2(aStdinPipe[PIPE_READ], STDIN_FILENO) == -1) {
			exit(errno);
		}

		// redirect stdout
		if (dup2(aStdoutPipe[PIPE_WRITE], STDOUT_FILENO) == -1) {
			exit(errno);
		}

		// redirect stderr
		//if (dup2(aStderrPipe[PIPE_WRITE], STDERR_FILENO) == -1) {
		//	exit(errno);
		//}

		// all these are for use by parent only
		close(aStdinPipe[PIPE_READ]);
		close(aStdinPipe[PIPE_WRITE]);
		close(aStdoutPipe[PIPE_READ]);
		close(aStdoutPipe[PIPE_WRITE]); 
		close(aStderrPipe[PIPE_READ]);
		close(aStderrPipe[PIPE_WRITE]); 

		// run child process image
		nResult = execv(szCommand, aArguments);

		// if we get here at all, an error occurred, but we are in the child
		// process, so just exit
		exit(nResult);
	} else if (nChild > 0) {
		// parent continues here

		// close unused file descriptors, these are for child only
		close(aStdinPipe[PIPE_READ]);
		close(aStdoutPipe[PIPE_WRITE]);
		close(aStderrPipe[PIPE_WRITE]);

		// pass remaining open file descriptors to caller
		*procStdinFd = aStdinPipe[PIPE_WRITE];
		*procStdoutFd = aStdoutPipe[PIPE_READ];
		*procStderrFd = aStderrPipe[PIPE_READ];
	} else {
		// failed to create child
		close(aStdinPipe[PIPE_READ]);
		close(aStdinPipe[PIPE_WRITE]);
		close(aStdoutPipe[PIPE_READ]);
		close(aStdoutPipe[PIPE_WRITE]);
		close(aStderrPipe[PIPE_READ]);
		close(aStderrPipe[PIPE_WRITE]);
	}

	return nChild;
}

int main(void)
{
	std::cerr << "[master] Hello World!" << std::endl;
	std::cerr << "[master] Spawning child process" << std::endl;

	char * const args[] = {"worker", NULL};

	int siPipe, soPipe, sePipe;
	int ret;

	int subProcPID = createChild("../worker/worker", args, &siPipe, &soPipe, &sePipe);

	std::cerr << "[master] Subprocess spawned with PID: " << subProcPID << std::endl;
	std::cerr << "[master] FDs: " << siPipe << ", " << soPipe << ", " << sePipe << std::endl;

	// produce work
	JobInfo ji;
	ji.num_integers = 250000;

	// create setup message
	SHMSetupMsg setupMsg;
	strncpy(setupMsg.shm_name, "/shm_test", sizeof(setupMsg.shm_name));
	setupMsg.shm_size = sizeof(int)*ji.num_integers;

	// create a shared memory block of the given size
	int shm_fd = shm_open(setupMsg.shm_name, O_RDWR | O_CREAT, 0600);
	if(shm_fd == -1) {
		std::cerr << "[master] shm_open() failed: " << strerror(errno) << std::endl;
		return 1;
	}

	ret = ftruncate(shm_fd, setupMsg.shm_size);
	if(ret == -1) {
		std::cerr << "[master] ftruncate() failed: " << strerror(errno) << std::endl;
		return 1;
	}

	void *shared_mem = mmap(NULL, setupMsg.shm_size, PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if(shared_mem == (void*)-1) {
		std::cerr << "[master] mmap() failed: " << strerror(errno) << std::endl;
		return 1;
	}

	int *shared_ints = reinterpret_cast<int*>(shared_mem);

	for(size_t i = 0; i < ji.num_integers; i++) {
		shared_ints[i] = i+1;
	}

	uint64_t total_time = 0;
	size_t i;

	// send setup message
	ret = write(siPipe, &setupMsg, sizeof(setupMsg));
	if(ret == -1) {
		std::cerr << "[master] write() failed: " << strerror(errno) << std::endl;
		return 1;
	} else if(ret != sizeof(setupMsg)) {
		std::cerr << "[master] Could not write setup message to worker’s STDIN (" << ret << " bytes written), shutting down with error" << std::endl;
		return 1;
	}

	// make some rounds
	for(i = 0; i < 10000; i++) {
		Stopwatch stw("test");

		stw.Start();

		// send job info
		ret = write(siPipe, &ji, sizeof(ji));
		if(ret == -1) {
			std::cerr << "[master] write() failed: " << strerror(errno) << std::endl;
			break;
		} else if(ret != sizeof(ji)) {
			std::cerr << "[master] Could not write job info to worker’s STDIN (" << ret << " bytes written), shutting down with error" << std::endl;
			break;
		}

		//std::cerr << "[master] sent data for round " << i << std::endl;

		// receive result
		int sum;
		ret = read(soPipe, &sum, sizeof(sum));
		if(ret == -1) {
			std::cerr << "[master] read() data failed: " << strerror(errno) << std::endl;
			break;
		} else if(ret == 0) {
			std::cerr << "[master] Worker closed STDOUT, shutting down with error" << std::endl;
			break;
		}

		stw.Stop();

		std::cerr << "[master] Worker calculated sum as " << sum << std::endl;

		total_time += stw.GetMonotonicTimeNs();
	}

	if(i > 0) {
		std::cerr << "[master] average round trip time: " << (total_time/i) << " ns wall time" << std::endl;
	}

	// finished sending
	close(siPipe);

	std::cerr << "[master] Waiting for process termination" << std::endl;
	int exitStatus;
	pid_t exitPID = wait(&exitStatus);
	std::cerr << "[master] Process with PID " << exitPID << " terminated with code " << WEXITSTATUS(exitStatus) << std::endl;

	// shared memory teardown
	std::cerr << "[master] Shutting down shared memory" << std::endl;

	ret = munmap(shared_mem, setupMsg.shm_size);
	if(ret == -1) {
		std::cerr << "[master] munmap() failed: " << strerror(errno) << std::endl;
		return 1;
	}

	ret = shm_unlink(setupMsg.shm_name);
	if(ret == -1) {
		std::cerr << "[master] shm_unlink() failed: " << strerror(errno) << std::endl;
		return 1;
	}
}
