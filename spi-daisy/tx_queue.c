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
#include <linux/slab.h>

#include "tx_queue.h"

static void dummy_work(struct work_struct *ws) {}

struct tx_queue *tx_queue_new(size_t size) {
	int    i;
	struct tx_queue *q = kmalloc(
			sizeof(struct tx_queue) + sizeof(struct tx_entry) * size,
			GFP_KERNEL
	);

	if (!q) {
		printk(KERN_ERR "spi-daisy: Unable to alloc tx_queue()\n");
		return NULL;
	}

	INIT_LIST_HEAD(&q->free);
	INIT_LIST_HEAD(&q->fifo);
	INIT_LIST_HEAD(&q->prio);
	spin_lock_init(&q->lock);
	for (i = 0; i < size; i++) {
		struct tx_entry *e = &q->data[i];
		INIT_LIST_HEAD(&e->list);
		INIT_WORK(&e->tx_work, dummy_work);
		e->queue = q;
	} //end for //

	return q;
}

void tx_queue_del(struct tx_queue *q) {
	if (!q)
		return;

	while (1) {
		struct tx_entry *e = tx_entry_get(q);

		if (!e)
			break;
		cancel_work(&e->tx_work);
		e->erc = -EINTR;
		if (e->andthen)
			e->andthen(e->erc, e->user_data);
		else
			complete((struct completion *)e->user_data);
	} // end while //

	kfree(q);
}
