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

#include "trace.h"

#ifdef Debug

struct trace_struct __trace;
static bool trace_print(void);
static u32 trace_overruns(void);

static void __worker(struct work_struct *ws) {
	u32 overruns;
	struct trace_struct *ts = container_of(ws, struct trace_struct, work);
	if (ts->queue.completion) {
		complete(ts->queue.completion);
		return;
	}
	while (trace_print());
	overruns = trace_overruns();
	if (overruns)
		printk(KERN_ERR "spi-daisy: %d debug entries lost\n", overruns);
}

int trace_init(void)
{
	int i;

	memset(&__trace, 0x00, sizeof(__trace));
	INIT_LIST_HEAD(&__trace.queue.free);
	INIT_LIST_HEAD(&__trace.queue.fifo);
	sema_init(&__trace.queue.sem, 0);
	spin_lock_init(&__trace.queue.lock);
	__trace.queue.completion = NULL;
	for (i = 0; i < TRACE_QUEUE_SIZE; ++i) {
		struct trace_entry *e = &__trace.queue.data[i];
		INIT_LIST_HEAD(&e->list);
		list_add_tail(&__trace.queue.free, &e->list);
	} // end for //
	INIT_WORK(&__trace.work, __worker);
	__trace.workqueue = create_singlethread_workqueue("spi-daisy trace");
	if (!__trace.workqueue)
		return -ENOMEM;
	schedule_work(&__trace.work);
	return 0;
}

void trace_destroy(void)
{
	struct completion completion;

	spin_lock(&__trace.queue.lock);
	/**/trace("Complete");
	/**/ __trace.queue.completion = &completion;
	/**/ up(&__trace.queue.sem);
	spin_unlock(&__trace.queue.lock);
	wait_for_completion(&completion);
	if (__trace.workqueue) {
		flush_workqueue(__trace.workqueue);
		destroy_workqueue(__trace.workqueue);
		__trace.workqueue = NULL;
	}
}

static bool trace_print(void) {
	struct trace_entry *e;
	int                 d = down_interruptible(&__trace.queue.sem);
	bool                r = 0;

	if (d)
		return 0;
	spin_lock(&__trace.queue.lock);
	/**/ if (!list_empty(&__trace.queue.fifo)) {
	/**/ 	struct list_head *_e = __trace.queue.fifo.next;
	/**/ 	e = list_entry(_e, struct trace_entry, list);
	/**/	e->func(e);
	/**/	list_move_tail(_e, &__trace.queue.free);
	/**/	r = 1;
	/**/ }
	spin_unlock(&__trace.queue.lock);
	return r;
}

static u32 trace_overruns(void) {
	u32 r;

	spin_lock(&__trace.queue.lock);
	/**/ r = __trace.queue.overruns;
	/**/ __trace.queue.overruns = 0;
	spin_unlock(&__trace.queue.lock);
	return r;
}

void __trace_put(const char *msg, void (*func)(struct trace_entry *e),
		int x1, int x2, int x3)
{
	if (__trace.queue.completion)
		return;
	spin_lock(&__trace.queue.lock);
	/**/ if (!list_empty(&__trace.queue.free)) {
	/**/	struct list_head *_e = __trace.queue.free.next;
	/**/	struct trace_entry *e = list_entry(_e, struct trace_entry, list);
	/**/	e->func = func;
	/**/	e->msg  = msg;
	/**/	e->x1   = x1;
	/**/	e->x2   = x2;
	/**/	e->x3   = x3;
	/**/	list_move_tail(_e, &__trace.queue.fifo);
	/**/	up(&__trace.queue.sem);
	/**/ } else {
	/**/	__trace.queue.overruns++;
	/**/ }
	spin_unlock(&__trace.queue.lock);
	schedule_work(&__trace.work);
}

#endif
