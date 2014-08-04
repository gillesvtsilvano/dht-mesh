#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/init.h>
#include <linux/etherdevice.h>
#include <linux/if_ether.h>
#include <linux/pkt_sched.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>

#ifndef _NBT_H_
#define _NBT_H_

#define NBT_MODULE_LICENSE "GPL"
#define NBT_MODULE_AUTHOR "Gilles V. T. Silvano <gillesvtsilvano@gmail.com"
#define NBT_MODULE_DESC   "TODO"

#define PROC_NAME "nbt"
#define NBT_PROTO_TYPE 0x0909
#define MAX_SIZE 50
#define NBT_ASSOCIATE_ID 0x01
#define NBT_DISASSOCIATE_ID 0x02
#define NBT_UPDATE_ID 0x03

#define UPDATE_DELAY 10000
#define MSG_BUFFER_SIZE 256

char* str = NULL;
char* m = NULL;

static struct task_struct *update_task;
static struct net_device* dev = NULL;
static struct nbt_t* nbt_table;
struct nbt_node_t* nbt_create_neighbor(uint8_t* mac);

int maccmp(uint8_t* mac1, uint8_t* mac2);
uint32_t nbt_hash_func(void* info, size_t size);
uint8_t* nbt_get_mac(uint32_t key);

void nbt_create(void);
void nbt_destroy(void);

int nbt_remove_neighbor(uint8_t* mac);
int nbt_insert_neighbor(struct nbt_node_t* n);

int nbt_insert_mac(uint8_t* mac);
int nbt_remove_mac(uint8_t* mac);

struct nbt_node_t* nbt_create_neighbor(uint8_t* mac);

static struct packet_type nbt_pkt_type;

static int nbt_proc_show(struct seq_file *m, void *v);
static int nbt_proc_open(struct inode* inode, struct file *file);
static const struct file_operations nbt_proc_fops;

struct nbt_msg_associate;
struct nbt_msg_disassociate;
struct nbt_msg_update;

void nbt_craft_msg_associate(uint8_t* mac);
void nbt_craft_msg_disassociate(uint8_t *mac);
void nbt_craft_msg_update(void);

void nbt_associate(void);
void nbt_disassociate(void);
void nbt_update(void);

int update_task_func(void* data);

static int nbt_rcv(struct sk_buff* skb, struct net_device* dev, struct packet_type *nbt_pkt_type, struct net_device *orig);

struct net_device* get_dev(char* d, size_t s);


struct nbt_msg_associate {
	uint8_t type;
	uint8_t mac[6];
} *nbt_msg_associate;

struct nbt_msg_disassociate {
	uint8_t type;
	uint8_t mac[6];
} *nbt_msg_disassociate;

/* não utilizada */
struct nbt_msg_update{
	uint8_t type;
	uint8_t seq;
	uint8_t idx;
} *nbt_msg_update;

struct nbt_node_t {
	uint32_t idx;
	uint32_t key;
	uint8_t mac[6];
	struct nbt_node_t *next;
} nbt_node_t;


struct nbt_t{
	struct nbt_node_t* head;
} nbt_t;


static struct packet_type nbt_pkt_type = {
	.type = NBT_PROTO_TYPE,
	.func = nbt_rcv,	
};

static const struct file_operations nbt_proc_fops = {
	.open 	= nbt_proc_open,
	.read 	= seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};


/* debug function */
char* print_mac(uint8_t* mac);
char* nbt_print(void);
char* print_neighbor(struct nbt_node_t *n);
char* print_neighbors(struct nbt_node_t *n);


/*
 * Exports
 */
EXPORT_SYMBOL(nbt_associate);
EXPORT_SYMBOL(nbt_disassociate);
EXPORT_SYMBOL(nbt_update);
EXPORT_SYMBOL(maccmp);
EXPORT_SYMBOL(nbt_hash_func);
EXPORT_SYMBOL(nbt_create);
EXPORT_SYMBOL(nbt_get_mac);
EXPORT_SYMBOL(nbt_insert_mac);
EXPORT_SYMBOL(nbt_remove_mac);
EXPORT_SYMBOL(nbt_destroy);
EXPORT_SYMBOL(get_dev);
EXPORT_SYMBOL(print_mac);
EXPORT_SYMBOL(nbt_print);
EXPORT_SYMBOL(nbt_node_t);
EXPORT_SYMBOL(nbt_t);


MODULE_LICENSE(NBT_MODULE_LICENSE);
MODULE_AUTHOR(NBT_MODULE_AUTHOR);
MODULE_DESCRIPTION(NBT_MODULE_DESC);

#endif /* _NBT_H_ */
