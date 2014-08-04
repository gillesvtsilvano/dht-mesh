#include "dht.h"



int __init init_module(){
	//char myDev[10] = "virbr0";
	char myDev[10] = "eth0";

	dev = get_dev(myDev, strlen(myDev));
	if(!dev){
		return -ENXIO;
	}

	proc_create(PROC_NAME, 0644, NULL, &dht_proc_fops);
	dev_add_pack(&dht_pkt_type);

	dht_create();

	return 0;
}

void cleanup_module(){
	dev_remove_pack(&dht_pkt_type);
	remove_proc_entry(PROC_NAME, NULL);
	nbt_disassociate();
	dht_check();
	dht_destroy();
	nbt_destroy();
}

static struct packet_type dht_pkt_type = {
	.type = PROTO_TYPE,
	.func = dht_rcv,
};

int update_task_func(void* data){
	while(!kthread_should_stop()) {
		if (t){
			dht_check();
		}
		msleep_interruptible(UPDATE_DELAY);
	}

	return 0;
}

void dht_check(void){
	struct dht_node_t* pt = NULL, *ant = NULL, *target = NULL;
	uint8_t* addr = NULL;

	if (!dev){
		return;
	}

	if (!t) {
		return;
	}
	
	if (!t->head){
		return;
	}

	addr = nbt_get_mac(t->head->idx);

	if (addr) {
		if (maccmp(addr, dev->dev_addr)){
			dht_craft_msg_insert(addr, t->head->app, t->head->key, t->head->key_size, t->head->data, t->head->data_size);

			target = t->head;
			t->head = t->head->next;
			
			kfree(target);
			target = NULL;

			pt = t->head;
		} else {
			pt = t->head->next;
			ant = t->head;
		}
	}

	while(pt){
		addr = nbt_get_mac(pt->idx);

		if (addr)
			if (maccmp(addr, dev->dev_addr)){

				dht_craft_msg_insert(addr, pt->app, pt->key, pt->key_size, pt->data, pt->data_size);

				target = pt;

				if (!ant){
					pt = pt->next;
					t->head = pt;
				} else {
					ant->next = pt->next;
					pt = pt->next;
				}

				kfree(target);
				target = NULL;

				continue;
			}

		ant = pt;
		pt = pt->next;
	}
}

void dht_create(){
	if (!t){
		printk("DHT module started!\n");
		str = (char*) kmalloc(256, GFP_KERNEL);
		m = (char*) kmalloc(100, GFP_KERNEL);
		t = (struct dht_t*) kmalloc(sizeof(struct dht_t), GFP_KERNEL);
		t->head = NULL;

		retrive_tbl = (struct dht_t*) kmalloc(sizeof(struct dht_t), GFP_KERNEL);
		retrive_tbl->head = NULL;
			
		update_task = kthread_run(update_task_func, NULL, "dht_update_task");
	}
}

void dht_destroy(){
	struct dht_node_t* pt = NULL, *target = NULL;
		
	printk("DHT module stoped!\n");

	if (!t){
		return;
	}

	if (update_task)
		kthread_stop(update_task);

	if (!t->head){
		kfree(t);
		t = NULL;
		kfree(retrive_tbl);
		retrive_tbl = NULL;
		return;
	}
	
	pt = t->head;
	
	while(pt){
		target = pt;
		pt = pt->next;
		kfree(target);	
		target = NULL;
	};

	kfree(str);
	kfree(m);
	kfree(t);
	t = NULL;
}

void dht_craft_msg_insert(uint8_t* dst, uint8_t app, void* key, uint8_t key_size, void* data, uint8_t data_size){
	struct sk_buff *skb = NULL;
	uint8_t hlen, tlen, *p = NULL, ret;
	struct dht_msg_insert* msg = NULL;

	hlen = LL_RESERVED_SPACE(dev);
	tlen = dev->needed_tailroom;
	skb = alloc_skb(dev->hard_header_len + sizeof(struct dht_msg_insert) + hlen + tlen, GFP_ATOMIC);

	skb->protocol = htons(PROTO_TYPE);
	skb->dev = dev;
	skb->priority = TC_PRIO_CONTROL;
	skb_reset_network_header(skb);
	skb_put(skb, dev->hard_header_len + sizeof(struct dht_msg_insert));

	p = skb->data;
	
	memcpy(p, dst, ETH_ALEN);
	p += ETH_ALEN;
	memcpy(p, dev->dev_addr, ETH_ALEN);
	p += ETH_ALEN;
	memcpy(p, &skb->protocol, 2);
	p += 2;
	

	msg = (struct dht_msg_insert*) kmalloc(sizeof(struct dht_msg_insert), GFP_KERNEL);
	msg->type = DHT_INSERT_ID;
	msg->app = app;
	msg->idx = nbt_hash_func(key, key_size);
	strcpy(msg->key, key);
	msg->key_size = key_size;

	strcpy(msg->data, data);
	msg->data_size = data_size;

	memcpy(p, msg, sizeof(struct dht_msg_insert));

	ret = dev_queue_xmit(skb);
}

void dht_craft_msg_remove(uint8_t* dst, uint8_t app, void* key, uint8_t key_size){
	struct sk_buff *skb = NULL;
	uint8_t hlen, tlen, *p = NULL;
	struct dht_msg_remove* msg = NULL;

	hlen = LL_RESERVED_SPACE(dev);
	tlen = dev->needed_tailroom;
	skb = alloc_skb(dev->hard_header_len + sizeof(struct dht_msg_remove) + hlen + tlen, GFP_ATOMIC);
	skb->protocol = htons(PROTO_TYPE);
	skb->dev = dev;
	skb->priority = TC_PRIO_CONTROL;
	skb_reset_network_header(skb);
	skb_put(skb, dev->hard_header_len + sizeof(struct dht_msg_remove));
	p = skb->data;
	memcpy(p, dst, ETH_ALEN);
	p += ETH_ALEN;
	memcpy(p, dev->dev_addr, ETH_ALEN);
	p += ETH_ALEN;
	memcpy(p, &skb->protocol, 2);
	p += 2;
	
	msg = (struct dht_msg_remove*) kmalloc(sizeof(struct dht_msg_remove), GFP_KERNEL);
	msg->type = DHT_REMOVE_ID;
	msg->app = app;
	msg->idx = nbt_hash_func(key, key_size);
	memcpy(msg->key, key, key_size);
	msg->key_size = key_size;

	memcpy(p, msg, sizeof(struct dht_msg_remove));
	
	dev_queue_xmit(skb);
}



static int dht_rcv(struct sk_buff* skb, struct net_device* dev, struct packet_type *dht_pkt_type, struct net_device* orig){
	uint8_t rcv_type;
	uint8_t *p = NULL;
	void* msg = NULL;
	uint8_t src[6], dst[6];
	struct ethhdr *eth;

	//skb = skb_share_check(skb, GFP_ATOMIC);
	if (!skb)
		return -1;

	p = skb->data;
	rcv_type = *p;
	msg = p;
	eth = (struct ethhdr*)skb_mac_header(skb);
	skb_reset_network_header(skb);

	memcpy(src, skb->head, 6);
	memcpy(dst, skb->head + 6, 6);
	
	printk("dht_rcv(): src = %pM\tdst = %pM\n", eth->h_source, eth->h_dest);
	switch(rcv_type){
		case DHT_INSERT_ID:
			printk("DHT: got an insert packet from %s.\n", print_mac(src));
			dht_handle_insert(msg);
			break;	
		case DHT_REMOVE_ID:
			printk("DHT: got a remove packet from %s.\n", print_mac(src));
			dht_handle_remove(msg);
			break;
		case DHT_RETRIVE_ID:
			printk("DHT: got a retrive packet from %s.\n", print_mac(src));
			dht_handle_retrive(eth->h_source, msg);
			break;
		case DHT_RETRIVE_RESPONSE_ID:
			printk("DHT: got a retrive response packet from %s.\n", print_mac(src));
			dht_handle_retrive_response(msg);
			break;
		case DHT_NONE_ID:
			break;
		default:
			break;
	}	
	return 0;
}

int dht_handle_insert(void* msg){
	struct dht_node_t* pt = NULL, *ant = NULL, *newbie = NULL;
	struct dht_msg_insert* ins_msg = NULL;

	ins_msg = msg;
	
	newbie = (struct dht_node_t*) kmalloc(sizeof(struct dht_node_t), GFP_KERNEL);
	newbie->app = ins_msg->app;
	newbie->idx = ins_msg->idx;
	newbie->key = kmalloc(sizeof(uint8_t) * ins_msg->key_size, GFP_KERNEL);
	strcpy(newbie->key, ins_msg->key);
	newbie->key_size = ins_msg->key_size;
	newbie->data = kmalloc(sizeof(uint8_t) * ins_msg->data_size, GFP_KERNEL);
	strcpy(newbie->data, ins_msg->data);
	newbie->data_size = ins_msg->data_size;
	newbie->next = NULL;

	if (!t){
		return 0;
	}

	if (!t->head){
		t->head = newbie;
		return 0;
	}

	if (t->head->idx > newbie->idx){
		newbie->next = t->head;
		t->head = newbie;
		return 0;
	}

	ant = t->head;
	pt = t->head->next;
	
	while (pt){
		if (pt->idx > newbie->idx){
			newbie->next = pt;
			ant->next = newbie;	
			return 0;
		}
		ant = pt;
		pt = pt->next;
	}	
	ant->next = newbie;
	return 0;
}

int dht_handle_remove(void* msg){
	struct dht_node_t* pt = NULL, *ant = NULL, *target = NULL;
	struct dht_msg_remove* rmv_msg = NULL;
	uint32_t idx;

	rmv_msg = msg;

	idx = nbt_hash_func(rmv_msg->key, rmv_msg->key_size);

	pt = t->head;
	
	if (!pt) {
		return -2;
	}

	if (pt->app == rmv_msg->app && 
		!memcmp(pt->key, rmv_msg->key, (pt->key_size < rmv_msg->key_size ? pt->key_size : rmv_msg->key_size)) &&
			pt->key_size == rmv_msg->key_size)
	{
		target = pt;
		t->head = pt->next;
		kfree(target);
		return 0;
	}

	ant = pt;
	pt = pt->next;
	
	while (pt){
		if (pt->app == rmv_msg->app && 
		memcmp(pt->key, rmv_msg->key, (pt->key_size < rmv_msg->key_size ? pt->key_size : rmv_msg->key_size)) == 0 &&
			pt->key_size == rmv_msg->key_size)
		{
			target = pt;
			ant->next = pt->next;
			kfree(target);
			return 0;
		}
		ant = pt;
		pt = pt->next;
	}
	return -1;
}

int dht_retrive(uint8_t app, void* key, uint8_t key_size, void* data){
	struct dht_node_t* pt = NULL;

	uint8_t* addr = NULL;
	uint32_t idx;

	if(!dev){
		return -ENXIO;
	}

	if (!data) {
		return -2;
	}

	idx = nbt_hash_func(key, key_size);
	addr = nbt_get_mac(idx);

	if (!addr){
		return -1;
	}

	if (maccmp(addr, dev->dev_addr)){
		dht_craft_msg_retrive(addr, app, key, key_size, data);
		return 0;
	}

	pt = t->head;

	while(pt){
		if(!strcmp(pt->key, key) && pt->key_size == key_size && pt->app == app)
		{
			strcpy(data, pt->data);
			return 0;
		}
		pt = pt->next;
	}
	
	return -1;
}

int dht_handle_retrive(uint8_t* src, void* msg){
	struct sk_buff *skb = NULL;
	uint8_t hlen, tlen, *p = NULL;
	struct dht_msg_retrive *ret_msg = NULL;
	struct dht_msg_retrive_response* res_msg = NULL;
	struct dht_node_t* pt = NULL;
	
	printk("Received a retrive pkt from %s\n", print_mac(src));
	ret_msg = msg;

	pt = t->head;

	while (pt){
		if (!strcmp(pt->key, ret_msg->key) && pt->app == ret_msg->app && pt->key_size == ret_msg->key_size){
			break;
		}
		pt = pt->next;
	}

	hlen = LL_RESERVED_SPACE(dev);
	tlen = dev->needed_tailroom;
	skb = alloc_skb(dev->hard_header_len + sizeof(struct dht_msg_retrive_response) + hlen + tlen, GFP_ATOMIC);

	skb->protocol = htons(PROTO_TYPE);
	skb->dev = dev;
	skb->priority = TC_PRIO_CONTROL;
	skb_reset_network_header(skb);
	skb_put(skb, dev->hard_header_len + sizeof(struct dht_msg_retrive_response));

	p = skb->data;

	memcpy(p, src, ETH_ALEN);
	p += ETH_ALEN;
	memcpy(p, dev->dev_addr, ETH_ALEN);
	p += ETH_ALEN;
	memcpy(p, &skb->protocol, 2);
	p += 2;

	res_msg = (struct dht_msg_retrive_response*) kmalloc(sizeof(struct dht_msg_retrive_response), GFP_KERNEL);
	res_msg->type = DHT_RETRIVE_RESPONSE_ID;
	res_msg->app = ret_msg->app;
	res_msg->idx = nbt_hash_func(ret_msg->key, ret_msg->key_size);
	strcpy(res_msg->key, ret_msg->key);
	res_msg->key_size = ret_msg->key_size;

	if(pt){
		strcpy(res_msg->data, pt->data);
		res_msg->data_size = pt->data_size;

	} else {
		strcpy(res_msg->data, "");
		res_msg->data_size = 0;
	}

	memcpy(p, res_msg, sizeof(struct dht_msg_retrive_response));

	printk("Sending a retrive repsonse to %s with data %s for key %s\n", print_mac(src), (char*) res_msg->data, (char*) res_msg->key);
	dev_queue_xmit(skb);

	return 0;
}

void dht_craft_msg_retrive(uint8_t* dst, uint8_t app, void* key, uint8_t key_size, void* data){
	struct sk_buff *skb = NULL;
	uint8_t hlen, tlen, *p = NULL, tries = 0;
	struct dht_msg_retrive* msg = NULL;
	struct dht_node_t* pt = NULL, *newbie, *ant = NULL;

	printk("dht_craft_msg_retrive(): sending a retrive pkt to %s\n", print_mac(dst));
	hlen = LL_RESERVED_SPACE(dev);
	tlen = dev->needed_tailroom;
	skb = alloc_skb(dev->hard_header_len + sizeof(struct dht_msg_retrive) + hlen + tlen, GFP_ATOMIC);

	skb->protocol = htons(PROTO_TYPE);
	skb->dev = dev;
	skb->priority = TC_PRIO_CONTROL;
	skb_reset_network_header(skb);
	skb_put(skb, dev->hard_header_len + sizeof(struct dht_msg_retrive));

	p = skb->data;
	
	memcpy(p, dst, ETH_ALEN);
	p += ETH_ALEN;
	memcpy(p, dev->dev_addr, ETH_ALEN);
	p += ETH_ALEN;
	memcpy(p, &skb->protocol, 2);
	p += 2;
	
	msg = (struct dht_msg_retrive*) kmalloc(sizeof(struct dht_msg_retrive), GFP_KERNEL);
	msg->type = DHT_RETRIVE_ID;
	msg->app = app;
	msg->idx = nbt_hash_func(key, key_size);
	strcpy(msg->key, key);
	msg->key_size = key_size;

	memcpy(p, msg, sizeof(struct dht_msg_retrive));

	dev_queue_xmit(skb);
	
	newbie = (struct dht_node_t*) kmalloc(sizeof(struct dht_node_t), GFP_KERNEL);
	newbie->app = app;
	newbie->idx = nbt_hash_func(key, key_size);
	newbie->key = kmalloc(key_size, GFP_KERNEL);	
	strcpy(newbie->key, key);
	newbie->key_size = key_size;
	newbie->data = NULL;
	newbie->next = NULL;

	pt = retrive_tbl->head;

	if (!pt){
		retrive_tbl->head = newbie;
		goto receive;
	}
	ant = pt;
	pt = pt->next;
	while (pt){
		ant = pt;
		pt = pt->next;
	}
	ant->next = newbie;
receive:
	printk("dht_craft_msg_retrive(): Waiting for the response\n");
	while (!newbie->data){
		if (tries++ > 5)
			goto free;
		msleep_interruptible(1000);		// Trocar por wait e notify
	}


	strcpy(data, newbie->data);
	if (!ant){
		retrive_tbl->head = NULL;
	} else {
		ant->next = pt;
	}
free:
	kfree(newbie);
}

void dht_handle_retrive_response(void* msg){
	struct dht_msg_retrive_response * res_msg = NULL;
	struct dht_node_t* pt = NULL;
	res_msg = msg;

	pt = retrive_tbl->head;

	while (pt){
		if (!strcmp(pt->key, res_msg->key) && pt->key_size == res_msg->key_size && pt->app == res_msg->app){
			pt->data = kmalloc(res_msg->data_size, GFP_KERNEL);
			strcpy(pt->data, res_msg->data);
			pt->data_size = res_msg->data_size;
			break;
		}
		pt = pt->next;
	}
}

int dht_insert(uint8_t app, void* key, uint8_t key_size, void* data, uint8_t data_size){
	struct dht_node_t* newbie = NULL, * pt = NULL, *ant = NULL;

	uint8_t* addr = NULL;
	uint32_t idx;

	idx = nbt_hash_func(key, key_size);
	addr = nbt_get_mac(idx);

	if (!addr){
		return -1;
	}

	if(!dev){
		return -ENXIO;
	}

	if (maccmp(addr, dev->dev_addr)){
		dht_craft_msg_insert(addr, app, key, key_size, data, data_size);
		return 0;
	}

	newbie = (struct dht_node_t*) kmalloc(sizeof(struct dht_node_t), GFP_KERNEL);
	newbie->app = app;
	newbie->idx = idx;
	newbie->key = kmalloc(256, GFP_KERNEL);
	strcpy(newbie->key, key);
	newbie->key_size = key_size;
	newbie->data = kmalloc(256, GFP_KERNEL);
	strcpy(newbie->data, data);
	newbie->data_size = data_size;
	newbie->next = NULL;

	if (!t->head){
		t->head = newbie;
		return 0;
	}
	
	if (t->head->idx > newbie->idx){
		newbie->next = t->head;
		t->head = newbie;
		return 0;
	}

	ant = t->head;
	pt = t->head->next;
	
	while (pt){
		if (pt->idx > newbie->idx){
			newbie->next = pt;
			ant->next = newbie;
			return 0;
		}
		ant = pt;
		pt = pt->next;
	}
	ant->next = newbie;
	return 0;
}


int dht_remove(uint8_t app, void* key, uint8_t key_size){
	struct dht_node_t *pt = NULL, *ant = NULL, *target = NULL;
	uint32_t idx;
	uint8_t* addr = NULL;

	if (!t)
		return -3;

	idx = nbt_hash_func(key, key_size);
	addr = nbt_get_mac(idx);

	if (maccmp(addr, dev->dev_addr)){
		dht_craft_msg_remove(addr, app, key, key_size);
		return 0;
	}

	pt = t->head;

	if (pt->app == app && 
		!memcmp(pt->key, key, pt->key_size < key_size ? pt->key_size : key_size) &&
		pt->key_size == key_size)
	{
		target = pt;
		t->head = pt->next;
		kfree(target);
		return 0;
	}

	ant = pt;
	pt = pt->next;
	
	while (pt){
		if (pt->app == app && 
			!memcmp(pt->key, key, pt->key_size < key_size ? pt->key_size : key_size) &&
			pt->key_size == key_size)
		{
			target = pt;
			ant->next = pt->next;
			kfree(target);
			return 0;
		}
		ant = pt;
		pt = pt->next;
	}
	return -1;
}

static int dht_proc_show(struct seq_file *m, void *v){
	seq_printf(m, "%s\n", dht_print());
	return 0;
}

static int dht_proc_open(struct inode* inode, struct file *file){
	return single_open(file, dht_proc_show, NULL);
}

/*
 * Debug functions
 */
char* dht_print(){
	struct dht_node_t* pt = NULL;
	
	strcpy(str, "");
	if (!t){
		return str;
	}

	pt = t->head;

	if (!pt){
		return str;	
	}

	while (pt){
		dht_print_node(pt);
		pt = pt->next;
	}

	return str;
}

char* dht_print_node(struct dht_node_t* n){
	if (!strcmp(str, "")) {
		sprintf(str, "%u\t%u\t%s\t%s",
			n->app,
			n->idx,
			(char*) n->key,
			(char*) n->data
			);
	} else {
		sprintf(str, "%s\n%u\t%u\t%s\t%s",
			str,
			n->app,
			n->idx,
			(char*) n->key,
			(char*) n->data
			);
	}

	return str;
}
