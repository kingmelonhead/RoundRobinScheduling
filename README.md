John Hackstadt
Project 3 - Operating Systems

------- Description ---------

This program is an operating system simulator that is supposed to simulate process scheduling using 
the round robin scheduling algorithm. This is that each process is given a quantum or time slice that 
they are allowed to operate in. in that time, since this is a simulator, we generate what happens to the process
after it gets spawned.

in my program one of three things can happen to a process 

1. as soon as it gets woken up, it deccides it is going to run for a little and then exit
the time that it decided to run gets logged in ns

2.It can run for a portion of the time slice, then get blocked. if blocked it gets moved to a different queue until
a specified time. After the time passes it is allowed to move back to the ready queue

3.the process, either new, or returning from blocked, can finish its work in the time slice and exit when
it is done

in case 1 and 2, the total amount of time spent in the system as well as the total time in control of cpu 
gets logged to the log file, along with the number of nano seconds used on its last burst



-------- Changes ----------------

in the project description it says to make I/O proceses more prone to interupts than cpu processes. However, in implementing this
using random number generation, the I/O processes werent being blocked mroe often than the CPU processes. To make the
simulator more realistic i just made it so that cpu processes simply cant be interupted. I was much more satisfied with my
log file results after doing this

-------- Options ----------------

-h invoke the help menu

-s limit the number of time that the program can run for

-l allows the user to change the name of the logfile generated by the program