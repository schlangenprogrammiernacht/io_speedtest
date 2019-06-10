# A Simple I/O Speedtest

This repo contains the code to test I/O roundtrip times on a POSIX system.
Currently shared memory is implemented as a data exchange method.

## Building

Simply run

```sh
./make.sh
```

## Running

```sh
cd build/master
./master
```

## How it works

The _master_ process sets up a POSIX shared memory (basically a `mmap`ed file
on _tmpfs_) and starts the _worker_ process.

Then the _master_ process fills the shared memory with a number of integers and
notifies the _worker_ through `STDIN` when everything is prepared. The _worker_
then calculates the sum of the numbers and reports it back through `STDOUT`.

This process is repeated a few times. The `master` process measures and shows
the average response time.

## License

MIT. (see `LICENSE` file)
