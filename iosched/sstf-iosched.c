/*
 * elevator sstf
 * 
 * Maintains two queues: a 'before' queue for requests with a sector before
 * the read head, and an 'after' queue for requests after the read head.
 * Dispatches the closest requests from either the before or after queue.
 *
 * Albert Morgan (2016)
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

/* Define this to turn on very verbose logging */
#define SSTF_LOGGING

struct sstf_data {
	struct list_head list[2];
	int cur_list;
	sector_t last_sector;
};

/* Enable/disable logging */
#ifdef SSTF_LOGGING
#define SSTF_LOG(rq) printk("\n" "SSTF_LOG" " %lu %lu %llu\n", \
				jiffies, jiffies - rq->start_time, \
				blk_rq_pos(rq))
#define SSTF_PRINT(msg) printk("\n%s\n", msg);
#else
#define SSTF_LOG(rq) do {} while(0)
#define SSTF_PRINT(msg) do {} while(0)
#endif

static void sstf_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	SSTF_PRINT("REQUESTS HAVE BEEN COALESCED");
	list_del_init(&next->queuelist);
}

static void sstf_merged(struct request_queue *q, struct request *rq, int type)
{
	SSTF_PRINT("REQUEST HAS BEEN MERGED");
}


/* Choose whether to dispatch from the before or after list */
static int sstf_dispatch(struct request_queue *q, int force)
{
	struct sstf_data *sd = q->elevator->elevator_data;
	struct request *rq;

	if (list_empty(&sd->list[0]) && list_empty(&sd->list[1]))
		return 0;

	if (list_empty(&sd->list[sd->cur_list]))
		sd->cur_list = !sd->cur_list;

	rq = list_first_entry(&sd->list[sd->cur_list], struct request, queuelist);
	sd->last_sector = rq_end_sector(rq);
	list_del_init(&rq->queuelist);
	elv_dispatch_add_tail(q, rq);

	return 1;
}

/* Insertion-sort new elements into the before or after list */
static void sstf_add_in_order(struct request *rq, struct list_head *list)
{
	struct request *r;

	list_for_each_entry(r, list, queuelist) {
		if (blk_rq_pos(r) > blk_rq_pos(rq)) {
			list_add_tail(&rq->queuelist, &r->queuelist);
			return;
		}
	}

	list_add_tail(&rq->queuelist, list);
}

/* Add a new request to the before or after list */
static void sstf_add_request(struct request_queue *q, struct request *rq)
{
	struct sstf_data *sd = q->elevator->elevator_data;

	if (sd->last_sector < blk_rq_pos(rq)) {
		sstf_add_in_order(rq, &sd->list[sd->cur_list]);
	} else {
		sstf_add_in_order(rq, &sd->list[!sd->cur_list]);
	}
}

static struct request *
sstf_former_request(struct request_queue *q, struct request *rq)
{
	struct sstf_data *sd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &sd->list[0] || rq->queuelist.prev == &sd->list[1])
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
sstf_latter_request(struct request_queue *q, struct request *rq)
{
	struct sstf_data *sd = q->elevator->elevator_data;

	if (rq->queuelist.next == &sd->list[0] || rq->queuelist.next == &sd->list[1])
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static int sstf_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct sstf_data *sd;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	sd = kmalloc_node(sizeof(*sd), GFP_KERNEL, q->node);
	if (!sd) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = sd;

	INIT_LIST_HEAD(&sd->list[0]);
	INIT_LIST_HEAD(&sd->list[1]);

	sd->last_sector = 0;

	sd->cur_list = 0;

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void sstf_exit_queue(struct elevator_queue *e)
{
	struct sstf_data *sd = e->elevator_data;

	BUG_ON(!list_empty(&sd->list[0]));
	BUG_ON(!list_empty(&sd->list[1]));
	kfree(sd);
}

static void sstf_completed(struct request_queue *q, struct request *rq)
{	
	SSTF_LOG(rq);
}


static struct elevator_type elevator_sstf = {
	.ops = {
		.elevator_merge_req_fn		= sstf_merged_requests,
		.elevator_merged_fn		= sstf_merged,
		.elevator_dispatch_fn		= sstf_dispatch,
		.elevator_add_req_fn		= sstf_add_request,
		.elevator_former_req_fn		= sstf_former_request,
		.elevator_latter_req_fn		= sstf_latter_request,
		.elevator_init_fn		= sstf_init_queue,
		.elevator_exit_fn		= sstf_exit_queue,
		.elevator_completed_req_fn	= sstf_completed
	},
	.elevator_name = "sstf",
	.elevator_owner = THIS_MODULE,
};

static int __init sstf_init(void)
{
	return elv_register(&elevator_sstf);
}

static void __exit sstf_exit(void)
{
	elv_unregister(&elevator_sstf);
}

module_init(sstf_init);
module_exit(sstf_exit);

MODULE_AUTHOR("Albert Morgan");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SSTF IO Scheduler");
