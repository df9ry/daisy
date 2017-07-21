/* Copyright 2017 Tania Hagn
 *
 * This file is part of Daisy.
 *
 *    Daisy is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Daisy is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Daisy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/ip.h>

#include "l2_queue.h"

struct l2_queue *l2_queue_new(size_t size) {
	int    i;
	struct l2_queue *q = kmalloc(
			sizeof(struct l2_queue) + sizeof(struct l2_entry) * size,
			GFP_KERNEL
	);

	if (!q) {
		printk(KERN_ERR "daisy: L2: Unable to alloc l2_queue()\n");
		return NULL;
	}

	INIT_LIST_HEAD(&q->free);
	INIT_LIST_HEAD(&q->fifo);
	spin_lock_init(&q->lock);
	for (i = 0; i < size; i++) {
		INIT_LIST_HEAD(&q->data[i].list);
		q->data[i].queue = q;
		q->data[i].skb = NULL;
	} //end for //

	return q;
}

void l2_queue_del(struct l2_queue *q) {
	struct list_head *_e;
	struct l2_entry  *e;

	if (!q)
		return;
	while (!list_empty(&q->fifo)) {
		_e = q->fifo.next;
		e = list_entry(_e, struct l2_entry, list);
		if (e->skb)
			dev_kfree_skb(e->skb);
		list_del(_e);
	} // end while //
	kfree(q);
}
