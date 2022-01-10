British University in Egypt

Computer Engineering Department

Operating Systems Course (21COMP02I)

### group members
1. Mahmoud Ashraf Mahmoud
2. Ahmed mohamed fady
3. malek sayed zaki
4. Rehand Ashraf
5. Reem Amr Emara

# project coding Requirments

## Requirement 2
In this part of the assessment, you need to modify xv6 to include the following scheduling
algorithms:

• Round Robin

• Shortest Job First (SJF)

• Multi-Level Feedback Queue

The user should be allowed to edit a configuration file in the operating system to provide any required
parameters or assumptions for these scheduling algorithms. Real processes must be executed, and
each algorithm must be tested by computing the average turnaround time and waiting time for each
algorithm for a set of processes that start, execute, and end in specific times. A detailed comparatives
analysis and explanation of the results must be included in the report that will be delivered by the end
of the assessment.

### code
you can download and run requirement 2 code using this command
```bash
git clone -b scheduling https://github.com/uptotec/xv6 xv6-req2
cd xv6-req2
make
make qemu-nox
```
if you chage any parameters in the os.config file you have to force recompilation using the following command
```bash
make qemu-nox -B
```

## Requirement 3
In this requirement, you need to implement hierarchical paging in xv6, with all the needed
parameters (page size, number of levels, address format … etc.) are user-defined via a configuration
file. Additionally, FIFO and LRU page replacement algorithms should be implemented (configuration
parameters of these algorithms should be stored in configuration file too). The performance parameters
(e.g., number of page faults, number of empty frames … etc.) of the hierarchical paging as well as the
replacement algorithms should be collected as the size of the pages and the number of levels is
changed, with a complete analysis should be provided in the report in addition to the collected results.

### code
you can download and run requirement 3 code using this command
```bash
git clone https://github.com/uptotec/xv6 xv6-req3
cd xv6-req3
make
make qemu-nox
```
if you chage any parameters in the os.config file you have to force recompilation using the following command
```bash
make qemu-nox -B
```