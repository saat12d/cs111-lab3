# You Spin Me Round Robin

UID - 706162648

A round robin CPU scheduling simulator in C

## Building

```shell
make
```

## Running

cmd for running
```shell
./rr processes.txt <quantum>
```
where processes.txt is an input file. It lists the number of processes in the first line, then has one line per process in the format 
pid, arrival_time, burst_time

quantum is the time slice

results

Given processes.txt from the lab skeleton, and running it with a quantum of 3 with the following command,
```shell
./rr processes.txt 2
```
we get the result - 

Average waiting time: 7.00

Average response time: 2.75

## Cleaning up

```shell
make clean
```

