#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>

extern int nbt_insert_mac(uint8_t*);
extern void nbt_print(void);
extern void print_mac(uint8_t*);
extern int nbt_remove_mac(uint8_t*);
extern uint8_t* nbt_get_mac(uint32_t);

int main_function(void){
	uint8_t mac1[6], mac2[6], mac3[6], mac4[6], mac5[6], mac6[6];
	
	memcpy(mac1, "\0", 6);
	memcpy(mac2, "\0", 6);
	memcpy(mac3, "\0", 6);
	memcpy(mac4, "\0", 6);
	memcpy(mac5, "\0", 6);
	memcpy(mac6, "\0", 6);

	memcpy(mac1, "\x9f\x00\x00\x00\x00\x00", 6);
	memcpy(mac2, "\xf2\x00\x00\x00\x00\x00", 6);
	memcpy(mac3, "\xf1\x00\x00\x00\x00\x00", 6);
	memcpy(mac4, "\xf1\x00\x00\x00\x00\x00", 6);
	memcpy(mac5, "\xff\xff\xff\xff\xff\xff", 6);
	memcpy(mac6, "\xff\xff\xff\xff\xff\xff", 6);


	printk(KERN_INFO "Main:\tInserting mac6("); print_mac(mac6); printk(KERN_INFO ")\n");
	nbt_insert_mac(mac6);
	nbt_print();
	//getc(stdin);

	printk(KERN_INFO "Main:\tInserting mac5("); print_mac(mac5); printk(KERN_INFO ")\n");

	nbt_insert_mac(mac5);
	nbt_print();
	//getc(stdin);

	printk(KERN_INFO "Main:\tInserting mac1("); print_mac(mac1); printk(KERN_INFO ")\n"); 
	nbt_insert_mac(mac1);
	nbt_print();
	//getc(stdin);
	
	printk(KERN_INFO "Main:\tInserting mac2("); print_mac(mac2); printk(KERN_INFO ")\n");

	nbt_insert_mac(mac2);
	nbt_print();
	//getc(stdin);

	printk(KERN_INFO "Main:\tInserting mac3("); print_mac(mac3); printk(KERN_INFO ")\n");

	nbt_insert_mac(mac3);
	nbt_print();
	//getc(stdin);

	printk(KERN_INFO "Main:\tInserting mac4("); print_mac(mac4); printk(KERN_INFO ")\n");
	nbt_insert_mac(mac4);
	nbt_print();
	//getc(stdin);

	printk(KERN_INFO "Main:\tRemoving mac3(");
	print_mac(mac3);
	printk(KERN_INFO ")\n");

	nbt_remove_mac(mac3);
	nbt_print();
	//getc(stdin);

	printk(KERN_INFO "Main:\tGetting mac by hash 15\n");
	print_mac(nbt_get_mac(15));
	
	return 0;
}

int init_module(void){
	printk(KERN_INFO "Main: Starting\n");
	main_function();
	return 0;
}

void cleanup_module(void){
	printk(KERN_INFO "Main:\tReleasing memory.\n");
	printk(KERN_INFO "Main:\tOk.\n");
}
