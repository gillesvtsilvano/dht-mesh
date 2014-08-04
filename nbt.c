#include "nbt.h"


int __init init_module(void){
	char myDev[10] = "eth0";
	dev = get_dev(myDev, strlen(myDev));
	if(!dev){
		dev = first_net_device(&init_net);
	}

	proc_create(PROC_NAME, 0644, NULL, &nbt_proc_fops);

	dev_add_pack(&nbt_pkt_type);
	nbt_create();
	return 0;
}

void cleanup_module(void){
	remove_proc_entry(PROC_NAME, NULL);
	dev_remove_pack(&nbt_pkt_type);
	nbt_disassociate();
	nbt_destroy();
}

static int nbt_proc_show(struct seq_file *m, void *v){
	seq_printf(m, "%s\n", nbt_print());
	return 0;
}

static int nbt_proc_open(struct inode* inode, struct file *file){
	return single_open(file, nbt_proc_show, NULL);
}

int update_task_func(void* data){
	while(!kthread_should_stop()) {
		if (nbt_table != 0) {
			nbt_update();
		}
		msleep_interruptible(UPDATE_DELAY);
	}
	return 0;
}

void nbt_associate(void){
	if (!dev)
		return;
	if (!nbt_table)
		return;

	nbt_insert_mac(dev->dev_addr);
	nbt_craft_msg_associate(dev->dev_addr);
}


void nbt_disassociate(void){
	if (!dev)
		return;
	if (!nbt_table)
		return;

	nbt_remove_mac(dev->dev_addr);
	nbt_craft_msg_disassociate(dev->dev_addr);
}


void nbt_update(void){
	if (nbt_table != 0)
		nbt_craft_msg_update();
}


void nbt_craft_msg_associate(uint8_t* mac){
	struct sk_buff* skb = 0;
	uint8_t hlen, tlen, *p = 0;

	if (nbt_table == 0){
		return;
	}
	if (nbt_table->head == 0){
		return;
	}

	if (mac == 0){
		return;
	}

    if (!dev)
            return;
 
    hlen = LL_RESERVED_SPACE(dev);
    tlen = dev->needed_tailroom;
    
    skb = alloc_skb(dev->hard_header_len + sizeof(struct nbt_msg_associate) + hlen + tlen, GFP_ATOMIC);


    skb->protocol = htons(NBT_PROTO_TYPE);
    skb->dev = dev;
    skb->priority = TC_PRIO_CONTROL;
    skb_reset_network_header(skb);
    skb_put(skb, dev->hard_header_len + sizeof(struct nbt_msg_associate));
    p = skb->data;
    memcpy(p, dev->broadcast, ETH_ALEN);
    p += ETH_ALEN;
    memcpy(p, dev->dev_addr, ETH_ALEN);
    //memcpy(p, mac, ETH_ALEN);
    p += ETH_ALEN;
    memcpy(p, &skb->protocol, 2);
    p += 2;
	*p = NBT_ASSOCIATE_ID;
	p += 1;
	memcpy(p, mac, ETH_ALEN);
	p += ETH_ALEN;

	dev_queue_xmit(skb);
}


void nbt_craft_msg_disassociate(uint8_t* mac){
	struct sk_buff* skb;
	uint8_t hlen, tlen, *p;

    if (!dev)
            return;

    hlen = LL_RESERVED_SPACE(dev);
    tlen = dev->needed_tailroom;
    
    skb = alloc_skb(dev->hard_header_len + sizeof(struct nbt_msg_disassociate) + hlen + tlen, GFP_ATOMIC);

    skb->protocol = htons(NBT_PROTO_TYPE);
    skb->dev = dev;
    skb->priority = TC_PRIO_CONTROL;
    skb_reset_network_header(skb);
    skb_put(skb, dev->hard_header_len + sizeof(struct nbt_msg_disassociate));
    p = skb->data;
    memcpy(p, dev->broadcast, ETH_ALEN);
    p += ETH_ALEN;
    memcpy(p, dev->dev_addr, ETH_ALEN);
    p += ETH_ALEN;
    memcpy(p, &skb->protocol, 2);
    p += 2;
	*p = NBT_DISASSOCIATE_ID;
	p += 1;
	memcpy(p, mac, ETH_ALEN);
    p += 6;

	dev_queue_xmit(skb);
}

void nbt_craft_msg_update(void){
		struct nbt_node_t* pt = 0;
		if (nbt_table == 0)
			return;

		pt = nbt_table->head;

		while (pt != 0){
			nbt_craft_msg_associate(pt->mac);
			pt = pt->next;
		}
}

static int nbt_rcv(struct sk_buff* skb, struct net_device* nd, struct packet_type *nbt_pkt_type, struct net_device *orig) {
	struct h {
		uint8_t dst[6];
		uint8_t src[6];
		uint16_t eth_t;
		uint8_t dht_t;
		uint8_t mac[8];
	} *h = NULL;
	//} *h = (struct h*) kmalloc(sizeof(struct h), GFP_ATOMIC);

	skb = skb_share_check(skb, GFP_ATOMIC);
	if (!skb)
		return -1;

	if (!skb_push(skb, ETH_ALEN*2 + 2))
		return -1;

	h = (struct h*) skb->data;
	//memcpy(h, skb->data, sizeof(struct h));
	//	p += ETH_ALEN * 2 + 2;

	switch(h->dht_t){
		case NBT_ASSOCIATE_ID:
			if (nbt_insert_mac(h->mac) == 0){ // If it succeeds
				nbt_craft_msg_update();
			}
			break;
		case NBT_DISASSOCIATE_ID:
			if (nbt_remove_mac(h->mac) == 0){
				nbt_craft_msg_update();
			}
			break;
		case NBT_UPDATE_ID:
			// TODO	
			break;
		default:
			break;
	}
	kfree(h);
	h = 0;
	return 0;	
}

int maccmp(uint8_t* mac1, uint8_t* mac2){
	int i = 0;
	uint8_t *m1 = 0, *m2 = 0;
	if (mac1 == 0){
		return -1;
	} 
	if (mac2 == 0){
		return 1;
	}

	m1 = mac1;
	m2 = mac2;

	for (; i < 6; ++i, ++m1, ++m2){
		if (*m1 > *m2 || *m1 < *m2){
		       	return *m1 - *m2;
		}
	}
	/* equals */
	return 0;
}


uint32_t nbt_hash_func(void* info, size_t size){
	int i = 0;
	uint32_t sum = 0;
	uint8_t* c = info;

	if (info == 0 || size == 0){
		return 0;
	}

	for(; i < size; ++i, ++c){
		sum += *c;
	}
	return sum;
}


void nbt_create(void){
	if (!nbt_table) {
		printk("NBT module started!\n");
		str = (char*) kmalloc(256, GFP_KERNEL);
		m = (char*) kmalloc(50, GFP_KERNEL);
		nbt_table = (struct nbt_t*) kmalloc(sizeof(struct nbt_t), GFP_KERNEL);
		nbt_table->head = 0;

		nbt_insert_mac(dev->dev_addr);

		update_task = kthread_run(update_task_func, NULL, "nbt_update_task");
	}
}


uint8_t* nbt_get_mac(uint32_t key){
	struct nbt_node_t* pt = 0;
	struct nbt_node_t* ant = 0;	
	
	if (!nbt_table) {
		return 0;
	}

	if (!nbt_table->head){
		return 0;
	}

	pt = nbt_table->head;

	if (pt->key > key){
		while (pt){
			ant = pt;
			pt = pt->next;
		}
		return ant->mac;
	}

	ant = pt;
	pt = pt->next;

	while (pt){
		if (pt->key > key){
			return ant->mac;
		}
		ant = pt;
		pt = pt->next;
	}
	return ant->mac;
}


int nbt_remove_neighbor(uint8_t* mac){
	struct nbt_node_t* pt = 0;
	struct nbt_node_t* ant = 0;
	
	if (nbt_table == 0){
		return -2;
	}
	if (mac == 0){
		return -2;
	}

	pt = nbt_table->head;

	if (pt == 0){
		return -3;
	}

	if (maccmp(pt->mac, mac) == 0){
		nbt_table->head = pt->next;
		kfree(pt);
		pt = 0;
		return 0;
	}
	ant = pt;
	pt = pt->next;
	while (pt != 0){
		if (maccmp(pt->mac, mac) == 0){
			ant->next = pt->next;
			kfree(pt);
			pt = 0;
			return 0;
		}
		ant = pt;
		pt = pt->next;
	}
	return -1;
}

int nbt_insert_neighbor(struct nbt_node_t* n){
	struct nbt_node_t* pt = 0;
	struct nbt_node_t* ant = 0;

	if (nbt_table == 0 || n == 0)
		return -1;	/* null pointer */

	pt = nbt_table->head;

	/* 
	 * When the list is empty, the new node will be inserted as a new head
	 */	
	if (pt == 0){
		nbt_table->head = n;
		return 0;
	}


	/* 
	 * If there is at least one node in the list, first we should try to avoid
	 * move the 'ant' pointer, checking the head and the new node
	 *
	 * If the new node has...
	 */
	if (n->idx < pt->idx){
		nbt_table->head = n;
		n->next = pt;
		return 0;
	} else if(n->idx == pt->idx) {
		if (n->key < pt->key){
			nbt_table->head = n;
			n->next = pt;
			return 0;
		} else if (n->key == pt->key) {
			if (maccmp(n->mac, pt->mac) < 0){
				nbt_table->head = n;
				n->next = pt;
				return 0;
			} else if (maccmp(n->mac, pt->mac) > 0){
				n->idx += 1 % MAX_SIZE;
				n->next = 0;
				return nbt_insert_neighbor(n);				
			} else {
				return 1;
				// Equals
			}
		} else {
			/* n->idx += 1 % UINT32_MAX; */
			n->idx += 1 % MAX_SIZE;
			n->next = 0;
			return nbt_insert_neighbor(n);				
		}
	}

	ant = pt; 
	pt = pt->next;

	while (pt != 0) {
		if (n->idx < pt->idx){
			ant->next = n;
			n->next = pt;
			return 0;
		} else { 
			if(n->idx == pt->idx) {
				if (n->key < pt->key){
					ant->next = n;
					n->next = pt;
					return 0;
				} else {
					if (n->key == pt->key) {
						if (maccmp(n->mac, pt->mac) < 0){
							ant->next = n;
							n->next = pt;
							return 0;
						} else if (maccmp(n->mac, pt->mac) > 0){
							n->idx += 1 % MAX_SIZE;
							n->next = 0;
							return nbt_insert_neighbor( n);
						} else {
							return 1;
						}	
					} else {
						/* n->idx += 1 % UINT32_MAX; */
						n->idx += 1 % MAX_SIZE;
						n->next = 0;
						return nbt_insert_neighbor( n);				
					}	
				}
			}
		}
		ant = pt; 
		pt = pt->next;	
	}
	ant->next = n;
	return 0;
}

int nbt_insert_mac(uint8_t* mac){
	struct nbt_node_t* n = 0;
	if (nbt_table == 0 || mac == 0)
		return -1;	/* null pointer */
	n = nbt_create_neighbor(mac);

	return nbt_insert_neighbor(n);
}

int nbt_remove_mac(uint8_t* mac){
	return nbt_remove_neighbor(mac);
}

struct nbt_node_t* nbt_create_neighbor(uint8_t* mac){
	struct nbt_node_t* newbie = 0;
	uint8_t* m = mac;
	int i = 0;

	newbie = (struct nbt_node_t*) kmalloc(sizeof( struct nbt_node_t), GFP_KERNEL);

	newbie->key = newbie->idx = nbt_hash_func(mac, 6);

	for (; i < 6; ++i, ++m){
		newbie->mac[i] = *m;
	}
	newbie->next = 0;

	return newbie;
}

void nbt_destroy(){
	struct nbt_node_t* pt = 0;
	struct nbt_node_t* target = 0;

	printk("NBT module stoped!\n");
	if (nbt_table == 0) {
		return;
	}

	if (update_task)
		kthread_stop(update_task);

	pt = nbt_table->head;

	while (pt != 0){
		target = pt;
		pt = pt->next;
		kfree(target);
		target = 0;
	}

	kfree(str);
	kfree(m);

	kfree(nbt_table);
	nbt_table = 0;
}

struct net_device* get_dev(char* d, size_t s){
	struct net_device* i = first_net_device(&init_net);
	while (i){
		if (memcmp(i->name, d, s) == 0)
			break;
		i = next_net_device(i);
	}
	return i;
}

/*
 * Print Functions
 */
char* print_mac(uint8_t* mac){
	uint8_t *aux = 0;

	strcpy(m, "");

	aux = mac;
	if (aux == 0){
		sprintf(m, "%s", "--:--:--:--:--:--");
	} else {
		sprintf(m, "%02x:%02x:%02x:%02x:%02x:%02x",
			*aux, *(aux + 1), *(aux + 2), *(aux + 3), *(aux + 4), *(aux + 5));
	}
	return m;
}

char* print_neighbor(struct nbt_node_t *n){
	if (!strcmp(str, "")) {
		sprintf(str, "%05u\t%05u\t%s", n->idx, n->key, print_mac(n->mac));
	} else {
		sprintf(str, "%s\n%05u\t%05u\t%s", str, n->idx, n->key, print_mac(n->mac));
	}
	return str;
}

char* print_neighbors(struct nbt_node_t *n){
	struct nbt_node_t* pt = n;

	while (pt){
		print_neighbor(pt);
		pt = pt->next;
	}

	return str;
}

char* nbt_print(void){
	strcpy(str, "");
	if (nbt_table)
		print_neighbors(nbt_table->head);

	return str;
}
