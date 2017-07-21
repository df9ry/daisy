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
#include <linux/list.h>

#include "l1_queue.h"

struct l1_queue *l1_queue_new(size_t size) {
	int    i;
	struct l1_queue *q = kmalloc(
			sizeof(struct l1_queue) + sizeof(struct l1_entry) * size,
			GFP_KERNEL
	);

	if (!q) {
		printk(KERN_ERR "daisy: L1: Unable to alloc l1_queue()\n");
		return NULL;
	}

	INIT_LIST_HEAD(&q->free);
	INIT_LIST_HEAD(&q->fifo);
	INIT_LIST_HEAD(&q->prio);
	spin_lock_init(&q->lock);
	for (i = 0; i < size; i++) {
		INIT_LIST_HEAD(&q->data[i].list);
		q->data[i].queue = q;
	} //end for //

	return q;
}

void l1_queue_del(struct l1_queue *q) {
	kfree(q);
}
