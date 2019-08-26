#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/kobject.h>
#include<linux/sysfs.h>
#include<linux/init.h>
#include<linux/string.h>
#include<linux/uaccess.h>
#include<linux/slab.h>
#include<linux/kthread.h>
#include<linux/delay.h>

#define SUCCESS 0

static struct kobject *proc_kobj;

static struct task_struct *tsk;

typedef struct {
char name[30];		//name of the process
int prio;		//0 default		1-5	(highest prio first)
long time;		//in milisec
int sched;		//0-normal process	1-FIFO		2-RR
}proc_struct;

static proc_struct proc_data = {0};
static int flag=0;

struct list {
	proc_struct data;
	struct list *next;
}*head[6]={0};

int isListEmpty(struct list **last){

	if (*last == NULL)
		return -1;

	return SUCCESS;
}
//queue a process in a queue according to priority
void enqueue(struct list **last, proc_struct data){
        struct list *node;

        node = (struct list *)kmalloc(sizeof(struct list), GFP_KERNEL);
        if (!node){
                pr_err("Failed to allocate memory for node\n");
                return ;
        }

        node->data=data;

        if (*last == NULL){
                *last=node;
                (*last)->next = *last;
        }
        else{
        node->next = (*last)->next;
        (*last)->next = node;
        *last=node;
        }

        pr_info("Enqueued the Item\n");
}
//dequeue logic
void dequeue(struct list **last){
        struct list *first;
	long time;

        if (*last == NULL)
		return ;

	//if time is less then 100ms or a FIFO process then run and dequeue that process
	else if (((*last)->next->data.time <= 100) || ((*last)->next->data.sched == 1)) {

	pr_info("Serving on behalf of '%s' process (type=%d) for time=%ldms\n",(*last)->next->data.name, (*last)->next->data.sched, (*last)->next->data.time);
	//If fifo Process Sleep for a given TIme
	if ((*last)->next->data.sched == 1)
		msleep((*last)->next->data.time);
	else //If not a fifo process sleep for 100ms
		msleep(100);
	//if its last node the put last==NULL
	if ((*last)->next == *last){
                kfree(*last);
                *last=NULL;
        }//if not last node then remove the first node
        else {
                first=(*last)->next;

                (*last)->next = first->next;
                kfree(first);

        }
	return ;
	}

	else {//If process is a RR or SCHED_OTHER process and Time Greater Than 100ms
	pr_info("Serving on behalf of '%s' process (type=%d) ", (*last)->next->data.name, (*last)->next->data.sched);
	//if last process sleep for a given time
	if ((*last)->next == *last){
		pr_info("for time=%ldms\n", (*last)->next->data.time);
		msleep((*last)->next->data.time);
		kfree(*last);
		*last = NULL;
		return ;
	}//if not sleep for 100ms and update the time
	else {
	pr_info("Time=100ms\n");
	msleep(100);
	time = (*last)->next->data.time;
	time = time - 100;
	(*last)->next->data.time = time;
	}

                first = (*last)->next;
                *last = first;
	}

}
//print every queue with data 
void viewqueue(void){
int i=0;
struct list *temp, *last;

for (i=0; i<6;i++){
        pr_info("Queue[%d]:\n\t", i);
        last = head[i];

        if (last==NULL)
                pr_info("No item in linked list\n");
        else{
        temp=last->next;
        while (temp!=last){
		pr_info("Name:%s\tprio=%d\tSched=%d\tTime=%ld\n", temp->data.name, temp->data.prio, temp->data.sched, temp->data.time);
                temp=temp->next;
        }
		pr_info("Name:%s\tprio=%d\tSched=%d\tTime=%ld\n", temp->data.name, temp->data.prio, temp->data.sched, temp->data.time);
        }
        pr_info("\n\n");
}

}
//main thread which is managing the schedule of each process
static int kthread(void *data){
int i, j, ret;
	while (1){

		for (i=5; i>=0; i--){

	serve:	//dequeue that node
		dequeue(&head[i]);
		//check if new process of high priority is available or not
		for (j=5; j>=i; j--){
			ret = isListEmpty(&head[i]);
			if (ret == 0)
				goto serve;	//if it does first serve that
		}

		//if no process in the queue then get out of the loop
		if ((head[0] == NULL) && (head[1] == NULL) && (head[2] == NULL) && (head[3] == NULL) && (head[4] == NULL) && (head[5] == NULL))
			break;
		
                if (kthread_should_stop())
			break;

	 	}

		break;
	
	}
pr_info("\nOut of loop\n");
	return 0;
}


static ssize_t read_name(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
	pr_info("%s: invoked\n", __func__);
	
	return 0;
}
//Assign name to process
static ssize_t write_name(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){

	pr_info("%s: invoked\n", __func__);

	if (count > 30)
		return -EFAULT;

	sscanf(buf, "%s\n", &proc_data.name);

	pr_info("proc=%s\n", buf);
	return count;
}

static ssize_t read_prio(struct kobject *kobj, struct kobj_attribute *attr, char *buf){

	pr_info("%s: invoked\n", __func__);

	return 0;
}
//assign priority to process
static ssize_t write_prio(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){

	pr_info("%s: invoked\n", __func__);

	sscanf(buf, "%d", &proc_data.prio);

	pr_info("prio=%d\n", proc_data.prio);
	return count;
}

static ssize_t read_time(struct kobject *kobj, struct kobj_attribute *attr, char *buf){

	pr_info("%s: invoked\n", __func__);

	return 0;
}

//assign execution time to process
static ssize_t write_time(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){

	pr_info("%s: invoked\n", __func__);
	sscanf(buf, "%ld", &proc_data.time);

	pr_info("Time=%ld\n", proc_data.time);

	return count;
}

static ssize_t read_sched(struct kobject *kobj, struct kobj_attribute *attr, char *buf){

	pr_info("%s: invoked\n", __func__);
	return 0;
}

//assign Scheduler to process
static ssize_t write_sched(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){

	pr_info("%s: invoked\n", __func__);
	sscanf(buf, "%d", &proc_data.sched);

	pr_info("sched=%d\n", proc_data.sched);

	return count;
}
//read data from structue and print in dmesg
static ssize_t read_queue(struct kobject *kobj, struct kobj_attribute *attr, char *buf){

	pr_info("%s: invoked\n", __func__);
	pr_info("Name=%s\n", proc_data.name);
	pr_info("prio=%d\n", proc_data.prio);
	pr_info("time=%ld\n", proc_data.time);
	pr_info("sched=%d\n", proc_data.sched);

	return 0;
}

//queue the process to the queue
static ssize_t write_queue(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){

	pr_info("%s: invoked\n", __func__);

	if (proc_data.prio >= 6 || proc_data.prio < 0)
		proc_data.prio = 0;
			
	if (proc_data.sched > 2 || proc_data.sched < 0)
		proc_data.sched = 0;

	if (proc_data.time >= 1000 || proc_data.time <=0)
		proc_data.time = 1000;

	enqueue(&head[proc_data.prio],  proc_data);
	pr_info("Queued %s process in %d queue\n", proc_data.name, proc_data.prio);

	return count;
}

//print all the queue and their nodes in dmesg
static ssize_t read_proc(struct kobject *kobj, struct kobj_attribute *attr, char *buf){

	pr_info("%s: invoked\n", __func__);

	viewqueue();

	return 0;
}

//run the scheduler by creating a kernel thread
static ssize_t run_proc(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){

	pr_info("%s: invoked\n", __func__);
	
	if (flag == 0){
	tsk = kthread_create(kthread, NULL, "mykthread");
        if (IS_ERR(tsk)){
                pr_err("%s: unable to start kernel thread\n",__func__);
                return PTR_ERR(tsk);
        }

	pr_info("%s:Kthread created with id %d\n", __func__, tsk->pid);
	        wake_up_process(tsk);
		flag = 1;
	}

	return count;
}

//Run Process
//(Name		Priority	Time of exec		Scheduler assossiated with proc)
//Creating kobject for representation in Sysfs
static struct kobj_attribute procname = __ATTR(Name, 0660, read_name, write_name);
static struct kobj_attribute priority = __ATTR(priority, 0660, read_prio, write_prio);
static struct kobj_attribute time     = __ATTR(execution_time, 0660, read_time, write_time);
static struct kobj_attribute sched    = __ATTR(scheduler, 0660, read_sched, write_sched);

static struct kobj_attribute queue    = __ATTR(queue_proc, 0660, read_queue, write_queue);
static struct kobj_attribute execute  = __ATTR(run_process, 0660, read_proc, run_proc);

static struct attribute *attrs[] = {
	&procname.attr,
	&queue.attr,
	&priority.attr,
	&time.attr,
	&sched.attr,
	&execute.attr,
	NULL,
};
//assigning echo operation in a group
static struct attribute_group attr_group = {
	.attrs = attrs,
};

//Initaialization of module
static int __init mysched_init(void){
	int ret;

	pr_info("Dummy Scheduler Module INserted\n");
	//creating a kobject in sysfs
	proc_kobj = kobject_create_and_add("create_proc", NULL);
	if (!proc_kobj){
		pr_err("Failed to create a sysfs object");
		return -ENOMEM;
	}
	//create group of object in the kobject directory
	ret = sysfs_create_group(proc_kobj, &attr_group);
	if (ret)
		kobject_put(proc_kobj);



	pr_info("Process Scheduler Initialized\n");

return SUCCESS;
}

static void __exit mysched_exit(void){

	kobject_del(proc_kobj);
/*
	if (flag == 1)
		kthread_stop(tsk);
*/
	pr_info("Dummy Scheduler Module Removed\n");
}

module_init(mysched_init);
module_exit(mysched_exit);

MODULE_AUTHOR("jitenderanga@gmail.com");
MODULE_DESCRIPTION("Dummy Scheduler Module");
MODULE_LICENSE("GPL");
