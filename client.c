#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/init.h>
#include <linux/etherdevice.h>
#include <linux/if_ether.h>
#include <linux/pkt_sched.h>
#include <linux/kthread.h>
#include <asm/errno.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>


extern int dht_insert(uint8_t app, void* key, uint8_t key_size, void* data, uint8_t data_size);
extern int dht_remove(uint8_t app, void* key, uint8_t key_size);
extern int dht_retrive(uint8_t app, void* key, uint8_t key_size, void* data);

int __init init_module(){
	char data[256];
	char key[256];
	
	#define _ARP_ 10
	#define _RARP_ 20
	strcpy(key, "192.168.0.153");
	strcpy(data, "00:aa:bb:dd:ee:ff");
	dht_insert(_ARP_, key, strlen(key), data, strlen(data));	
	strcpy(key, "192.168.0.1");
	strcpy(data, "ee:ff:aa:dd:33:22");
	dht_insert(_ARP_, key, strlen(key), data, strlen(data));
	strcpy(key, "192.168.0.254");
	strcpy(data, "aa:bb:cc:cc:bb:aa");
	dht_insert(_ARP_, key, strlen(key), data, strlen(data));

	strcpy(data, "192.168.0.153\0");
	strcpy(key, "00:aa:bb:dd:ee:ff\0");
	dht_insert(_RARP_, key, strlen(key), data, strlen(data));	
	strcpy(data, "192.168.0.1\0");
	strcpy(key, "ee:ff:aa:dd:33:22\0");
	dht_insert(_RARP_, key, strlen(key), data, strlen(data));
	strcpy(data, "192.168.0.254");
	strcpy(key, "aa:bb:cc:cc:bb:aa");
	dht_insert(_RARP_, key, strlen(key), data, strlen(data));

	strcpy(key, "192.168.0.153");
	strcpy(data, "");
	dht_retrive(_ARP_, key, strlen(key), data);

	printk("Got data %s for key %s\n", data, key);	
	return 0;
}

void cleanup_module(){

}
