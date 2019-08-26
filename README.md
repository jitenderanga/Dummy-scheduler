# Dummy-scheduler
For Your info its not even close to a scheduler its just dummy implementaion. It would have been a scheduler if I would have created a process of my own on user request and scheduled that on the task for some interval on the CPU but its not doing any that kind of stuff. 

Dummy Scheduler is a Kernel Module which is interfaced with sysfs. Its Main objective is to run on behalf of the user to serve a process which have been queued by the user. Every Proccess has a Circular Linked Queue assigned to it which is based on the priority. Priority can range from 0 to 5 so their are 6 queues for each priority. Default priority is 0. Process also have a policy assigned to it which decide the how cpu will be utilized by that process.

when kernel module is inserted it create file under sysfs "/sys/create_proc" and contain Following files.
Name:-          Name of The Process
priority:-      Priority to Be Assigned                                 0-5                     (Default = 0)
execution_time:-Execution TIme of Process                               0-1000ms                (Default = 100ms)
scheduler:-     Scheduler to be assigned                                0-2                     (0 = SCHED_OTHER, 1 = FIFO, 2 = RR)(0 = Default)
queue_proc:-    Queue That Process to priority queue according to priority assigned (Queue is assigned based on priority no)
run_process:-   When Processes have been Queued it Run the Scheduler. Prcesses can be Scheduled to queued at run Time.

Working of Module:-
--------------------
        Module create a instance of "create_proc" under sysfs. When a user turn on scheduler by writing 
        into "run_process". It internally create a Kernel Thread, which is responsible for serving the 
        process. It always run the queue with highest priority first i.e 5th queue and so on. RR & 
        SCHED_OTHER process are served in Time Slices on 100ms after 100ms if execution period is over
        they are removed from the queue if execution time period is still remaining it move the process
        in tail of the queue. FIFO process does not have a time slice it always remain on head of queue 
        until execution time. After every 100ms queues are checked if queue with high priority is available 
        it is served first.

Running the Module:-
--------------------
insmod dummy_scheduler.ko

cd /sys/create_proc

(Its writes the data into a kernel structure and that structure is passed queued)

echo "FIrst" > Name; echo 0 > priority; echo 0 > scheduler; echo 100 > execution_time; echo "aaa" > queue_proc;

echo "Second" > Name; echo 1 > priority; echo 1 > scheduler; echo 500 > execution_time; echo "aaa" > queue_proc;

echo "Third" > Name; echo 1 > priority; echo 1 > scheduler; echo 200 > execution_time; echo "aaa" > queue_proc;

echo "Fourth" > Name; echo 4 > priority; echo 2 > scheduler; echo 500 > execution_time; echo "aaa" > queue_proc;

echo "Fifth" > Name; echo 4 > priority; echo 2 > scheduler; echo 300 > execution_time; echo "aaa" > queue_proc;

echo "Sixth" > Name; echo 5 > priority; echo 2 > scheduler; echo 300 > execution_time; echo "aaa" > queue_proc;


cat run_process   (see what has been queued to the kernel)

dmesg | tail -50

echo run_process   (run the scheduler)

dmesg | tail -50

After this when you dmesg you will see that all the process are run one by one according to the priority and time slice assigned to them.
