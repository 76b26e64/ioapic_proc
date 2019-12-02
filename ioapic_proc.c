#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>   
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define BUFSIZE (10240) 

static ssize_t ioapic_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos, unsigned int offset, char *message) 
{
	if(*ppos > 0 || count < BUFSIZE){
		return 0;
	}

	void *addr = ioremap(0xFEC00000, 0x10);
	int len = 0;
	char buf[BUFSIZE];
	memset(buf, 0, BUFSIZE);

    writel(offset, addr);
    len+=sprintf(buf+len, message, readl(addr+0x10));

    iounmap(addr);
	if(copy_to_user(ubuf,buf,len)){
   		pr_info("copy_to_user ERROR\n");
		return -EFAULT;
	}

	*ppos = len;
	return len;
}

static ssize_t ioapic_id_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read(file, ubuf, count, ppos, 0, "I/O APIC ID : %08x\n");
}

static ssize_t ioapic_version_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read(file, ubuf, count, ppos, 1, "I/O APIC VERSION : %08x\n");
}

static ssize_t ioapic_arbitration_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read(file, ubuf, count, ppos, 2, "I/O APIC ARBITRATION : %08x\n");
}

static ssize_t ioapic_read_redirection_table(struct file *file, char __user *ubuf, size_t count, loff_t *ppos, unsigned int table_no) 
{
	if(*ppos > 0 || count < BUFSIZE){
		return 0;
	}

	void *addr = ioremap(0xFEC00000, 0x10);
	int len = 0;
	char buf[BUFSIZE];
	memset(buf, 0, BUFSIZE);
	
	int i = table_no * 2;
	int upper_register = 0;
	int lower_register = 0;

	writel(i+0x10, addr);
	lower_register = readl(addr+0x10);
	writel(i+0x11, addr);
	upper_register = readl(addr+0x10);
   	len+=sprintf(buf+len,"I/O APIC redirection table%02d(offset:0x%02x) : 0x%08x%08x\n",table_no, i+0x10, upper_register, lower_register);

	int interrupt_mask = (lower_register >> 16) & 0x1;
	int trigger_mode = (lower_register >> 15) & 0x1;
	int remote_irr = (lower_register >> 14) & 0x1;
	int interrupt_input_pin_polarlty = (lower_register >> 13) & 0x1;
	int delivery_status = (lower_register >> 12) & 0x1;
	int destination_mode= (lower_register >> 11) & 0x1;
	int delivery_mode = (lower_register >> 8) & 0x3;
	int interrupt_vector= (lower_register) & 0xff;

	int destination_field = 0;
	if (destination_mode == 0){ //Physical Mode
		destination_field = (upper_register >> 22) & 0x0F;
   		len+=sprintf(buf+len," Destination Field Logical Destination Address(APIC ID): %x\n",destination_field);
	}else{
		destination_field = (upper_register >> 22) & 0xFF;
   		len+=sprintf(buf+len," Destination Field Logical Destination Address(Set of Processors): %x\n",destination_field);
	}
		
   	len+=sprintf(buf+len," Interrupt Mask : 0x%x(%s)\n",interrupt_mask ,(interrupt_mask? "Masked":"Not masked"));
   	len+=sprintf(buf+len," Trigger Mode : 0x%x(%s)\n",trigger_mode ,(trigger_mode ? "Level":"Edge"));
   	len+=sprintf(buf+len," Remote IRR : 0x%x\n",remote_irr);
   	len+=sprintf(buf+len," Interrupt Input Pin Polarity (INTPOL) : 0x%x(%s)\n",interrupt_input_pin_polarlty,(interrupt_input_pin_polarlty? "Low Active":"High Active") );
   	len+=sprintf(buf+len," Delivery Status (DELIVS) : 0x%x(%s)\n",delivery_status,(delivery_status? "Send Pendign":"IDLE"));
    len+=sprintf(buf+len," Destination Mode (DESTMOD) : 0x%x(%s)\n",destination_mode,(destination_mode? "Logical Mode":"Physical Mode"));
    	
	char *delivery_mode_explain = "Not supported";
	switch(delivery_mode){
		case 0:
			delivery_mode_explain = "Fixed";
			break;
		case 1:
			delivery_mode_explain = "Lowest Priority";
			break;
		case 2:
			delivery_mode_explain = "SMI";
			break;
		case 3:
			delivery_mode_explain = "Reserved";
			break;
		case 4:
			delivery_mode_explain = "NMI";
			break;
		case 5:
			delivery_mode_explain = "INIT";
			break;
		case 6:
			delivery_mode_explain = "Reserved";
			break;
		case 7:
			delivery_mode_explain = "ExtINT";
			break;
		default:
			break;
	}

	len+=sprintf(buf+len," Delivery Mode (DELMOD) : 0x%x(%s)\n",delivery_mode, delivery_mode_explain);
   	len+=sprintf(buf+len," Interrupt Vector (INTVEC): %d\n",interrupt_vector);

    iounmap(addr);

	if(copy_to_user(ubuf,buf,len)){
   		pr_info("copy_to_user ERROR\n");
		return -EFAULT;
	}

	*ppos = len;
	return len;
}

static ssize_t ioapic_redirection_table_0_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 0);
}

static ssize_t ioapic_redirection_table_1_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 1);
}

static ssize_t ioapic_redirection_table_2_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 2);
}

static ssize_t ioapic_redirection_table_3_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 3);
}

static ssize_t ioapic_redirection_table_4_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 4);
}

static ssize_t ioapic_redirection_table_5_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 5);
}

static ssize_t ioapic_redirection_table_6_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 6);
}

static ssize_t ioapic_redirection_table_7_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 7);
}

static ssize_t ioapic_redirection_table_8_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 8);
}

static ssize_t ioapic_redirection_table_9_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 9);
}

static ssize_t ioapic_redirection_table_10_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 10);
}

static ssize_t ioapic_redirection_table_11_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 11);
}

static ssize_t ioapic_redirection_table_12_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 12);
}

static ssize_t ioapic_redirection_table_13_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 13);
}

static ssize_t ioapic_redirection_table_14_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 14);
}

static ssize_t ioapic_redirection_table_15_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 15);
}

static ssize_t ioapic_redirection_table_16_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 16);
}

static ssize_t ioapic_redirection_table_17_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 17);
}

static ssize_t ioapic_redirection_table_18_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 18);
}

static ssize_t ioapic_redirection_table_19_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 19);
}

static ssize_t ioapic_redirection_table_20_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 20);
}

static ssize_t ioapic_redirection_table_21_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 21);
}

static ssize_t ioapic_redirection_table_22_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 22);
}

static ssize_t ioapic_redirection_table_23_read(struct file *file, char __user *ubuf,size_t count, loff_t *ppos) 
{
	return ioapic_read_redirection_table(file, ubuf, count, ppos, 23);
}

static struct file_operations ioapic_ops_list[] = 
{
	{
		.owner = THIS_MODULE,
		.read = ioapic_id_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_version_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_arbitration_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_0_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_1_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_2_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_3_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_4_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_5_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_6_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_7_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_8_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_9_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_10_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_11_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_12_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_13_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_14_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_15_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_16_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_17_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_18_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_19_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_20_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_21_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_22_read,
	},
	{
		.owner = THIS_MODULE,
		.read = ioapic_redirection_table_23_read,
	},
};

static char *ioapic_name_list[] = 
{
	"id",
	"version",
	"arbitration",
	"redirection_table_0",
	"redirection_table_1",
	"redirection_table_2",
	"redirection_table_3",
	"redirection_table_4",
	"redirection_table_5",
	"redirection_table_6",
	"redirection_table_7",
	"redirection_table_8",
	"redirection_table_9",
	"redirection_table_10",
	"redirection_table_11",
	"redirection_table_12",
	"redirection_table_13",
	"redirection_table_14",
	"redirection_table_15",
	"redirection_table_16",
	"redirection_table_17",
	"redirection_table_18",
	"redirection_table_19",
	"redirection_table_20",
	"redirection_table_21",
	"redirection_table_22",
	"redirection_table_23",
};

static struct proc_dir_entry *ent_list[27];
struct proc_dir_entry *parent_dir;

static int ioapic_proc_init(void)
{

	parent_dir = proc_mkdir("ioapic", NULL);
	int listnum = sizeof(ioapic_ops_list)/sizeof(ioapic_ops_list[0]);
	int i;
	for(i = 0; i < listnum; i++){
		ent_list[i] = proc_create(ioapic_name_list[i] ,0444, parent_dir, &ioapic_ops_list[i]);
	}

	return 0;
}
 
static void simple_cleanup(void)
{
	int listnum = sizeof(ioapic_ops_list)/sizeof(ioapic_ops_list[0]);
	int i;
	for(i = 0; i < listnum; i++){
		remove_proc_entry(ioapic_name_list[i], parent_dir);
		proc_remove(ent_list[i]);
	}
	remove_proc_entry("ioapic", NULL);
}
 
module_init(ioapic_proc_init);
module_exit(simple_cleanup);

