/* Katherine Bajno and Tasnia Kabir
// OS444 - Project 2
// References:
// https://elixir.bootlin.com/linux/v4.1/source/block/noop-iosched.c
// http://classes.engr.oregonstate.edu/eecs/fall2011/cs411/proj03.pdf
// https://www.geeksforgeeks.org/disk-scheduling-algorithms/
*/

#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

struct sstf_data {
	struct list_head queue;
};

static void sstf_merged_requests(struct request_queue *q, struct request *rq, struct request *next) {

  list_del_init(&next->queuelist);

	printk(KERN_DEBUG "sstf merging requests function\n");

}

static int sstf_dispatch(struct request_queue *q, int force) {

  struct sstf_data *sd = q->elevator->elevator_data;

	printk(KERN_DEBUG "sstf dispatch function\n");

	if (!list_empty(&sd->queue)) {

		struct request *req = list_entry(sd->queue.next, struct request, queuelist);
		list_del_init(&req->queuelist);
		elv_dispatch_sort(q, req);

		return 1;
	}

  return 0;
}

static void sstf_add_request(struct request_queue *q, struct request *rq){

		struct sstf_data *d = q->elevator->elevator_data;
	  struct request *cur_node;
	  struct list_head *cur_pos;
	  int position = 0;

		printk(KERN_DEBUG "sstf add request function \n");

		//if the list is empty, add the request to tail
	  if (list_empty(&d->queue)) {
	    list_add_tail(&rq->queuelist, &d->queue);
	    return;
	  }
		else{
		  sector_t head = q->end_sector;

				//iterate through list queue
			 	list_for_each (cur_pos, &d->queue) {
			    cur_node = list_entry(cur_pos, struct request, queuelist);

					//if the distance from the head to the req is less than the dis btwn head and the cur request, loop
			    long dis_head_cur_node;
			    long dis_head_cur_pos;
					dis_head_cur_node = abs(blk_rq_pos(rq) - head);
					dis_head_cur_pos = abs(blk_rq_pos(cur_node) - head);

			    if (dis_head_cur_node < dis_head_cur_pos) {
			      list_add_tail(&rq->queuelist, cur_pos);
			      position = 1;
			      break;
			    }

			    //set head position to current position from this iteration
			    head = blk_rq_pos(cur_node);
			  }

			  //if we didn't insert, just add to the end
			  if (!position) {
			    list_add_tail(&rq->queuelist, cur_pos);
			  }
	}

}

static struct request *sstf_former_request(struct request_queue *q, struct request *rq) {

  //printk(KERN_DEBUG "sstf former request function\n");

  struct sstf_data *d = q->elevator->elevator_data;

	if (rq->queuelist.prev == &d->queue) return NULL;

	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *sstf_latter_request(struct request_queue *q, struct request *rq){

  //printk(KERN_DEBUG "sstf latter request function \n");

  struct sstf_data *d = q->elevator->elevator_data;

	if (rq->queuelist.next == &d->queue) return NULL;

	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static int sstf_init_queue(struct request_queue *q, struct elevator_type *e) {

  //printk(KERN_DEBUG "sstf init queue function\n");


  struct sstf_data *d;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	d = kmalloc_node(sizeof(*d), GFP_KERNEL, q->node);
	if (!d) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = d;

	INIT_LIST_HEAD(&d->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void sstf_exit_queue(struct elevator_queue *e) {

  //printk(KERN_DEBUG "sstf exit queue function\n");

  struct sstf_data *d = e->elevator_data;

	BUG_ON(!list_empty(&d->queue));
	kfree(d);
}

static struct elevator_type elevator_sstf = {
	.ops = {
		.elevator_merge_req_fn		= sstf_merged_requests,
		.elevator_dispatch_fn		= sstf_dispatch,
		.elevator_add_req_fn		= sstf_add_request,
		.elevator_former_req_fn		= sstf_former_request,
		.elevator_latter_req_fn		= sstf_latter_request,
		.elevator_init_fn		= sstf_init_queue,
		.elevator_exit_fn		= sstf_exit_queue,
	},
	.elevator_name = "sstf",
	.elevator_owner = THIS_MODULE,
};

static int __init sstf_init(void) {
	return elv_register(&elevator_sstf);
}

static void __exit sstf_exit(void) {
	elv_unregister(&elevator_sstf);
}

module_init(sstf_init);
module_exit(sstf_exit);


MODULE_AUTHOR("Katherine Bajno and Tasnia Kabir");
MODULE_LICENSE("Oregon State University");
MODULE_DESCRIPTION("SSTF IO scheduler");
