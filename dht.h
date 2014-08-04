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


#ifndef _DHT_H_
#define _DHT_H_

#define DHT_MODULE_LICENSE "GPL"
#define DHT_MODULE_AUTHOR "Gilles V. T. Silvano <gillesvtsilvano@gmail.com>"
#define DHT_MODULE_DESC   "TODO"


#define PROC_NAME "dht"
#define PROTO_TYPE 0x0808
#define DHT_INSERT_ID 0x01
#define DHT_REMOVE_ID 0x02
#define DHT_RETRIVE_ID 0x03
#define DHT_ACK_ID 0x04
#define DHT_RESPONSE_ID 0x05
#define DHT_RETRIVE_RESPONSE_ID 0x06
#define DHT_CONFIRM_ID	0x07
#define DHT_NONE_ID 0x09

#define UPDATE_DELAY 10000

char* str = NULL;
char* m = NULL;

// Outdated
uint16_t seq;


static struct dht_t* t = NULL;
static struct dht_t* retrive_tbl = NULL;
struct net_device* dev = NULL;
static struct packet_type dht_pkt_type;
static struct task_struct *update_task = NULL;

struct dht_t {
	struct dht_node_t* head;
} dht_t;

struct dht_node_t{
	uint8_t app;
	uint32_t idx;
	void* key;
	uint8_t key_size;
	void* data;
	uint8_t data_size;
	struct dht_node_t* next;
} dht_node_t;

/* Structures that handle message information to be sent and received */
struct dht_msg_insert{
	uint8_t type;
	uint8_t app;
	uint32_t idx;
	uint8_t key_size;
	uint8_t data_size;
	uint8_t key[256];
	uint8_t data[256];
};


struct dht_msg_remove {
	uint8_t type;
	uint8_t app;
	uint32_t idx;
	uint8_t key_size;
	uint8_t key[256];
};

struct dht_msg_retrive{
	uint8_t type;
	uint8_t app;
	uint8_t idx;
	uint8_t key_size;
	uint8_t key[256];
};

struct dht_msg_retrive_response{
	uint8_t type;
	uint8_t app;
	uint8_t idx;
	uint8_t key_size;
	uint8_t data_size;
	uint8_t key[256];
	uint8_t data[256];
};


/* Funtions that handle data being sent from the node
 * These functions check if the data has to be send to another node
 * or if it needs to be stored localy
 */
int dht_handle_insert(void* msg);
int dht_handle_remove(void* msg);
int dht_handle_retrive(uint8_t* src, void* data);
void dht_handle_retrive_response(void *msg);

/* Functions that crafts new messages and try to send these packets though
 * the device configured in dht_create()
 */
void dht_craft_msg_insert(uint8_t* dst, uint8_t app, void* key, uint8_t key_size, void* data, uint8_t data_size);
void dht_craft_msg_remove(uint8_t* dst, uint8_t app, void* key, uint8_t key_size);
void dht_craft_msg_retrive(uint8_t* dst, uint8_t app, void* key, uint8_t key_size, void* data);

void dht_create(void);
void dht_destroy(void);
int dht_insert(uint8_t app, void* key, uint8_t key_size, void* data, uint8_t data_size);
int dht_remove(uint8_t app, void* key, uint8_t key_size);
int dht_retrive(uint8_t app, void* key, uint8_t key_size, void* data);
void dht_check(void);

int update_task_func(void* data);

static int dht_rcv(struct sk_buff* skb, struct net_device* dev, struct packet_type *dht_pkt_type, struct net_device* orig);


static int dht_proc_show(struct seq_file *m, void *v);

static int dht_proc_open(struct inode* inode, struct file *file);

static const struct file_operations dht_proc_fops = {
	.open 	= dht_proc_open,
	.read 	= seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

// Debug functions
char* dht_print(void);
char* dht_print_node(struct dht_node_t*);


/* Nbt functions defined at nbt.h */

extern int nbt_insert_mac(uint8_t*);
extern void nbt_print(void);
extern char* print_mac(uint8_t*);
extern int nbt_remove_mac(uint8_t*);
extern uint8_t* nbt_get_mac(uint32_t);
extern uint32_t nbt_hash_func(void* info, unsigned long size);
extern struct net_device* get_dev(char* d, size_t s);
extern void nbt_create(void);
extern void nbt_destroy(void);
extern void nbt_associate(void);
extern void nbt_disassociate(void);
extern void nbt_update(void);
extern int maccmp(uint8_t* mac1, uint8_t* mac2);


/*
 * Exports
 */
EXPORT_SYMBOL(dht_insert);
EXPORT_SYMBOL(dht_remove);
EXPORT_SYMBOL(dht_retrive);

MODULE_LICENSE(DHT_MODULE_LICENSE);
MODULE_AUTHOR(DHT_MODULE_AUTHOR);
MODULE_DESCRIPTION(DHT_MODULE_DESC);

#endif /* _DHT_H_ */
