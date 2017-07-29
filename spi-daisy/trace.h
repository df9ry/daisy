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

#ifndef _TRACE_H_
#define _TRACE_H_

#include <linux/module.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/completion.h>
#include <linux/workqueue.h>

#define TRACE_QUEUE_SIZE 256

struct trace_queue;
struct trace_entry;
struct trace_struct;

#ifdef Debug
struct trace_entry {
	struct list_head    list;
	struct trace_queue *queue;
	const char         *msg;
	int                 x1, x2, x3;
	void (*func)(struct trace_entry *e);
};
#endif

#ifdef Debug
struct trace_queue {
	struct list_head    free;
	struct list_head    fifo;
	struct semaphore    sem;
	spinlock_t          lock;
	u32                 overruns;
	struct completion  *completion;
	struct trace_entry  data[TRACE_QUEUE_SIZE];
};
#endif

#ifdef Debug
extern struct trace_struct {
	struct trace_queue       queue;
	struct workqueue_struct *workqueue;
	struct work_struct       work;
} __trace;
#endif

#ifdef Debug
static inline void __func_trace0(struct trace_entry *e)
{
	printk(KERN_DEBUG "spi-daisy: Trace %s\n", e->msg);
}
#endif

#ifdef Debug
static inline void __func_trace1(struct trace_entry *e)
{
	printk(KERN_DEBUG "spi-daisy: Trace %s [%d|0x%x]\n",
							e->msg, e->x1, e->x1);
}
#endif

#ifdef Debug
static inline void __func_trace2(struct trace_entry *e)
{
	printk(KERN_DEBUG "spi-daisy: Trace %s [%d|0x%x] [%d|0x%x]\n",
							e->msg, e->x1, e->x1, e->x2, e->x2);
}
#endif

#ifdef Debug
static inline void __func_trace3(struct trace_entry *e)
{
	printk(KERN_DEBUG "spi-daisy trace %s [%d|0x%x] [%d|0x%x] [%d|0x%x]\n",
							e->msg, e->x1, e->x1, e->x2, e->x2, e->x3, e->x3);
}

#endif

#ifdef Debug

 extern int trace_init(void);
 extern void trace_destroy(void);

#else

# define trace_init() 0
# define trace_destroy()

#endif

#ifdef Debug
extern void __trace_put(const char *msg, void (*func)(struct trace_entry *e),
						int x1, int x2, int x3);
# define trace0(M) \
	__trace_put(M, __func_trace0,      0,       0,       0 )
# define trace1(M,X1) \
	__trace_put(M, __func_trace1, (int)X1,      0,       0 )
# define trace2(M,X1,X2) \
	__trace_put(M, __func_trace2, (int)X1, (int)X2,      0 )
# define trace3(M,X1,X2,X3) \
	__trace_put(M, __func_trace3, (int)X1, (int)X2, (int)X3)
# define trace(M) trace0(M)

#else

# define trace(M)
# define trace1(M,X1)
# define trace2(M,X1,X2)
# define trace3(M,X1,X2,X3)

#endif

#endif //_TRACE_H_//

