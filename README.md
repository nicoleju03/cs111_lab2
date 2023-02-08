# You Spin Me Round Robin
A simulation of round robin scheduling that calculates average waiting time and average response time given a list of processes with their arrival times and burst times.

## Building
Build the executable by running ```make```.

## Running
Run the executable with ```./rr``, followed by a text file that contains your process data (number of processes, pids, arrival times, and burst times), then the quantum length.

For example, ```./rr processes.txt 3``` will run the executable with process.txt as the text file that represents the list of processes and a quantum length of 3.

## Cleaning up
Clean up by running ```make clean```.