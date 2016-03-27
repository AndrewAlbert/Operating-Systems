#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/acpi.h>
#include <asm/uaccess.h>
#include <linux/kthread.h>

MODULE_LICENSE("GPL");

//kthread to collect battery info every second
struct task_struct *ts;

//Dynamic battery information
int previousStatus = 0;
int previousCapacity = 0;
bool CapacityWasLow = false;
bool CapacityWasFull = false;
int customRate = 100;
int count = 0;
int battState = 0;	//0 is steady, 1 is discharging, 2 is charging
int ChargeRate = 0;
int remainingCapacity = 0;
int presentVoltage = 0;

//Static battery information
int present = 0;
int designCapacity = 0;
int lastFullCapacity = 0;
int batteryTechnology = 0;
int designVoltage = 0;
int designCapacityWarning = 0;
int designCapacityLow = 0;
int CapacityGranularity1 = 0;
int CapacityGranularity2 = 0;
char* modelNumber;
char* serialNumber;
char* batteryType;
char* OEMinfo;

#define BUFFER_SIZE 512

//battery's info buffer
struct proc_dir_entry *proc_entry_info, *root_dir_info = NULL;
char result_buffer_info[BUFFER_SIZE];
int temp_info = 1; //ensures info is not read twice before writing to buffer

//battery's stat buffer
struct proc_dir_entry *proc_entry_stat, *root_dir_stat = NULL;
char result_buffer_stat[BUFFER_SIZE];
int temp_stat = 1; //ensures stat is not read twice before writing to buffer

void batt_check(int y){
	//battery is charging or discharging, calculate discharge rate (if applicable) and record remaining capacity
	if(previousCapacity != remainingCapacity){
		if(ChargeRate == -1 && count != 0){
			customRate = 60*((previousCapacity - remainingCapacity)/(count));
			customRate = (customRate < 0) ? customRate*-1 : customRate;
			printk(KERN_INFO "Time between change: %d sec\n", count);
			printk(KERN_INFO "Changed by: %d mAh/min\n", customRate);
		}
		count = 0;
		previousCapacity = remainingCapacity;
	}

	//battery changes from charging to discharging and vice versa
	if(previousStatus != battState)
		printk(KERN_INFO "\nChanged: \n\tcharging status: %d\n\tcharge remaining: %d mAh\n\t%d\n", battState, remainingCapacity, y);
	//battery capacity falls below warning level
	else if(remainingCapacity <= designCapacityWarning){
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(HZ * 29);
		printk(KERN_INFO "\nCritical: \n\tcharging status: %d\n\tcharge remaining: %d mAh\n\t%d\n", battState, remainingCapacity, y);
	}
	//battery capacity falls below low level
	else if(remainingCapacity <= designCapacityLow && !CapacityWasLow){
		CapacityWasLow = true;
		printk(KERN_INFO "\nLow: \n\tcharging status: %d\n\tcharge remaining: %d mAh\n\t%d\n", battState, remainingCapacity, y);
	}
	//battery is fully charged
	else if(battState == 0 && !CapacityWasFull){
		CapacityWasFull = true;
		printk(KERN_INFO "\nFull: \n\tcharging status: %d\n\tcharge remaining: %d mAh\n\t%d\n", battState, remainingCapacity, y);
	}
	//battery is charged back above warning level
	else if((remainingCapacity > designCapacityWarning) && CapacityWasLow) CapacityWasLow = false;
	//battery discharges below full level
	else if(CapacityWasFull && battState != 0) CapacityWasFull = false;

	//record battery's charging/discharging status
	previousStatus = battState;
}

int batt_thread(void* data){
	acpi_status status;
	acpi_handle handle;
	union acpi_object *result = NULL;
	struct acpi_buffer buffer = {ACPI_ALLOCATE_BUFFER, NULL};
	
	//grab batter information every second while the kthread is in use
	while(!kthread_should_stop()){
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(HZ);
		count++;

		status = acpi_get_handle(NULL, (acpi_string)"\\_SB_.PCI0.BAT0", &handle);
		status = acpi_evaluate_object(handle, "_BST", NULL, &buffer);
		result = buffer.pointer;

		//get dynamic battery information
		if(result){
			battState = result->package.elements[0].integer.value;
			ChargeRate = result->package.elements[1].integer.value;
			remainingCapacity = result->package.elements[2].integer.value;
			presentVoltage = result->package.elements[3].integer.value;
		previousCapacity = (previousCapacity == 0) ? remainingCapacity : previousCapacity;

			batt_check(remainingCapacity);
		}
	}
	if (result) kfree(result);
	return 0;
}

//read static battery information and write to buffer
ssize_t read_info(struct file *fd, char __user *buf, size_t c, loff_t *off){
	int length;
	if(temp_info == 0){
		temp_info = 1;
		return 0;
	}
	sprintf(result_buffer_info, "%d %d %d %d %d %d %d %d %d %s %s %s %s\n", present, designCapacity, lastFullCapacity, batteryTechnology, designVoltage, designCapacityWarning, designCapacityLow, CapacityGranularity1, CapacityGranularity2, modelNumber, serialNumber, batteryType, OEMinfo);

	length = strlen(result_buffer_info);
	if(copy_to_user(buf, result_buffer_info, length)) return -EFAULT;

	temp_info = 0;
	return length;
}

//read dynamic battery information and write to buffer
ssize_t read_stat(struct file *fd, char __user *buf, size_t c, loff_t *off){
	int length;
	if(temp_stat == 0){
		temp_stat = 1;
		return 0;
	}

	//if discharging, calculate a custom discharge rate
	if(ChargeRate == -1){
		ChargeRate = customRate;
	}

	sprintf(result_buffer_stat, "%d %d %d %d\n", battState, ChargeRate, remainingCapacity, presentVoltage);
	length = strlen(result_buffer_stat);
	if(copy_to_user(buf, result_buffer_stat, length)) return -EFAULT;
	temp_stat = 0;
	return length;
}

static const struct file_operations file_ops_stat = {
	.owner = THIS_MODULE,
	.read = read_stat
};

static const struct file_operations file_ops_info = {
	.owner = THIS_MODULE,
	.read = read_info
};

int init_module( void ){
	acpi_status status;
	acpi_handle handle;
	union acpi_object *result;
	struct acpi_buffer buffer = {ACPI_ALLOCATE_BUFFER, NULL};

	status = acpi_get_handle(NULL, "\\_SB_.PCI0.BAT0", &handle);
	status = acpi_evaluate_object(handle, "_BIF", NULL, &buffer);
	result = buffer.pointer;

	//Get static battery information
	if(result)
	{
		present = result->package.elements[0].integer.value;
		designCapacity = result->package.elements[1].integer.value;
		lastFullCapacity = result->package.elements[2].integer.value;
		batteryTechnology = result->package.elements[3].integer.value;
		designVoltage = result->package.elements[4].integer.value;
		designCapacityWarning = result->package.elements[5].integer.value;
		designCapacityLow = result->package.elements[6].integer.value;
		CapacityGranularity1 = result->package.elements[7].integer.value;
		CapacityGranularity2 = result->package.elements[8].integer.value;
		modelNumber = result->package.elements[9].string.pointer;
		serialNumber = result->package.elements[10].string.pointer;
		batteryType = result->package.elements[11].string.pointer;
		OEMinfo = result->package.elements[12].string.pointer;

		//print static battery information to KERN_INFO
		printk(KERN_INFO "present: %d\n", present);
		printk(KERN_INFO "design capacity: %d mAh\n", designCapacity);
		printk(KERN_INFO "last full capacity: %d mAh\n", lastFullCapacity);
		printk(KERN_INFO "battery technology: %d\n", batteryTechnology);
		printk(KERN_INFO "design voltage: %d mV\n", designVoltage);
		printk(KERN_INFO "design capacity low level warning: %d mAh\n", designCapacityWarning);
		printk(KERN_INFO "design capacity low level (critical) warning: %d mAh\n", designCapacityLow);
		printk(KERN_INFO "Capacity granularity 1: %d mAh\n", CapacityGranularity1);
		printk(KERN_INFO "Capacity granularity 2: %d mAh\n", CapacityGranularity2);
		printk(KERN_INFO "Model number: %s\n", modelNumber);
		printk(KERN_INFO "Serial number: %s\n", serialNumber);
		printk(KERN_INFO "Battery type: %s\n", batteryType);
		printk(KERN_INFO "OEM Info: %s\n", OEMinfo);

		kfree(result);
	}

	//create process entries for battery information
	proc_entry_info = proc_create("battery_info.txt", 438, NULL, &file_ops_info);
	strcpy(result_buffer_info, "initialized\n");
	if(proc_entry_info == NULL){
		printk(KERN_ERR "Couldn't create proc entry 0\n");
		return -ENOMEM;
	}

	proc_entry_stat = proc_create("battery_stat.txt", 438, NULL, &file_ops_stat);
	strcpy(result_buffer_stat, "initialized\n");
	if(proc_entry_stat == NULL){
		printk(KERN_ERR "Couldn't create proc entry 1\n");
		return -ENOMEM;
	}

	//start kthread to get battery information
	ts = kthread_run(batt_thread, NULL, "battchecl");
	return 0;
}

void cleanup_module(void){
	remove_proc_entry("battery_info.txt", root_dir_info);
	remove_proc_entry("battery_stat.txt", root_dir_stat);
	if(kthread_stop(ts)) printk(KERN_EMERG "The thread was not destroyed!\n");
	printk(KERN_INFO "Module unloaded successfully\n");
}
