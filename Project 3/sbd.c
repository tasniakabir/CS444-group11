/* Katherine Bajno and Tasnia Kabir
// OS444 - Project 3
// References:
// https://static.lwn.net/images/pdf/LDD3/ch16.pdf
// http://blog.superpat.com/2010/05/04/a-simple-block-driver-for-linux-kernel-2-6-31/ (base of code)
// https://naimcprogram.blogspot.com/2015/11/ram-disk-device-driver-for-linux-kernel.html
// https://stackoverflow.com/questions/18230784/what-is-difference-between-linux-kernel-subsystem-dm-crypt-and-ecryptfs
*/

/*
 * A sample, extra-simple block driver. Updated for kernel 2.6.31.
 *
 * (C) 2003 Eklektix, Inc.
 * (C) 2010 Pat Patterson <pat at superpat dot com>
 * Redistributable under the terms of the GNU GPL.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h> /* printk() */
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>

//Added extra libraries
#include <linux/crypto.h>
#include <linux/scatterlist.h>

MODULE_LICENSE("Dual BSD/GPL");
static char *Version = "1.4";

//Global params for program
static int major_num = -1; //changed from 0 to -1 for better init
module_param(major_num, int, 0);
static int logical_block_size = 512;
module_param(logical_block_size, int, 0);
static int nsectors = 1024; /* How big the drive is */
module_param(nsectors, int, 0);
static char * encrypt_key; //added encryption key var
module_param(encrypt_key, charp, 0);
int i;


/*
 * We can tweak our hardware sector size, but the kernel talks to us
 * in terms of small sectors, always.
 */
#define KERNEL_SECTOR_SIZE 512

/*
 * Our request queue.
 */
static struct request_queue *Queue;

/*
 * The internal representation of our device.
 */
static struct sbd_device {
	unsigned long size;
	spinlock_t lock;
	u8 *data;
	struct gendisk *gd;
} Device;

//struct for encryption tasks
static struct crypto_cipher *crypto_mod;

/*
 * Handle an I/O request.
 */
 //Added code for reading and writing encrpytion work
static void sbd_transfer(struct sbd_device *dev, sector_t sector,
	unsigned long nsect, char *buffer, int write) {
	unsigned long offset = sector * logical_block_size;
	unsigned long nbytes = nsect * logical_block_size;

	if ((offset + nbytes) > dev->size) {
		printk (KERN_NOTICE "sbd: Beyond-end write (%ld %ld)\n", offset, nbytes);
		return;
	}

  //var for the block size to increment read/writes
  unsigned long block_size = crypto_cipher_blocksize(crypto_mod);

	if (write) {

    //Added code for writing encrpytion
    printk("Starting write...\n");
    //encrpytion API
    for(i = 0; i < nbytes; i += block_size) {
      crypto_cipher_encrypt_one(crypto_mod,dev->data+offset+i, buffer+i);
    }

    printk("Pre-encryption text... \n");
		u8 *str = dev->data + offset;
    //print the original string of the text
    for (i = 0 ; i < nbytes ; i++){
      	printk("%02x ", str[i]);
		}

    printk("Encrpyted text... \n");
    //print the encrypted text
    for (i = 0 ; i < nbytes ; i++){
      	printk("%02x ", buffer[i]);
		}

   } else {

     //added code for reading encryption
     printk("Starting read...\n");
     memcpy(buffer, dev->data + offset, nbytes);
     //decryption API
     for(i = 0; i < nbytes; i += block_size) {
       crypto_cipher_decrypt_one(crypto_mod,buffer+i,dev->data + offset +i);
     }

     printk("Pre-decryption text... \n");
   	for (i = 0 ; i < nbytes ; i++){
      //print the original string of the text
       	printk("%02x ", buffer[i]);
   	}
    printk("Decryption text... \n");
   	u8 *str = dev->data + offset;
    //print the decryption text
   	for (i = 0 ; i < nbytes ; i++){
       	printk("%02x ", str[i]);
   	}
  }
}

//This function is unchanged
static void sbd_request(struct request_queue *q) {
	struct request *req;

	req = blk_fetch_request(q);
	while (req != NULL) {
		// blk_fs_request() was removed in 2.6.36 - many thanks to
		// Christian Paro for the heads up and fix...
		//if (!blk_fs_request(req)) {
		if (req == NULL || (req->cmd_type != REQ_TYPE_FS)) {
			printk (KERN_NOTICE "Skip non-CMD request\n");
			__blk_end_request_all(req, -EIO);
			continue;
		}
		sbd_transfer(&Device, blk_rq_pos(req), blk_rq_cur_sectors(req),
				bio_data(req->bio), rq_data_dir(req));
		if ( ! __blk_end_request_cur(req, 0) ) {
			req = blk_fetch_request(q);
		}
	}
}

/*
 * The HDIO_GETGEO ioctl is handled in blkdev_ioctl(), which
 * calls this. We need to implement getgeo, since we can't
 * use tools such as fdisk to partition the drive otherwise.
 */
 //This function is unchanged
int sbd_getgeo(struct block_device * block_device, struct hd_geometry * geo) {
	long size;

	/* We have no real geometry, of course, so make something up. */
	size = Device.size * (logical_block_size / KERNEL_SECTOR_SIZE);
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = 16;
	geo->start = 0;
	return 0;
}

/*
 * The device operations structure.
 */
 //This struct is unchanged
static struct block_device_operations sbd_ops = {
		.owner  = THIS_MODULE,
		.getgeo = sbd_getgeo
};

static int __init sbd_init(void) {
	/*
	 * Set up our internal device.
	 */
	Device.size = nsectors * logical_block_size;
	spin_lock_init(&Device.lock);
	Device.data = vmalloc(Device.size);
  if (Device.data == NULL)
		return -ENOMEM;

  //added init and creating crypto module for reading and writing
  crypto_mod = crypto_alloc_cipher("aes", 0 , 0);
  crypto_cipher_setkey(crypto_mod, encrypt_key, strlen(encrypt_key));

	/*
	 * Get a request queue.
	 */
	Queue = blk_init_queue(sbd_request, &Device.lock);
	if (Queue == NULL)
		goto out;
	blk_queue_logical_block_size(Queue, logical_block_size);
	/*
	 * Get registered.
	 */
	major_num = register_blkdev(major_num, "sbd");
	if (major_num < 0) {
		printk(KERN_WARNING "sbd: unable to get major number\n");
		goto out;
	}
	/*
	 * And the gendisk structure.
	 */
	Device.gd = alloc_disk(16);
	if (!Device.gd)
		goto out_unregister;
	Device.gd->major = major_num;
	Device.gd->first_minor = 0;
	Device.gd->fops = &sbd_ops;
	Device.gd->private_data = &Device;
	strcpy(Device.gd->disk_name, "sbd0");
	set_capacity(Device.gd, nsectors);
	Device.gd->queue = Queue;
	add_disk(Device.gd);

	return 0;

out_unregister:
	unregister_blkdev(major_num, "sbd");
out:
	vfree(Device.data);
	return -ENOMEM;
}

//added clearing memory for crypto module upon exit
static void __exit sbd_exit(void)
{
	del_gendisk(Device.gd);
	put_disk(Device.gd);
	unregister_blkdev(major_num, "sbd");
	blk_cleanup_queue(Queue);
	vfree(Device.data);

  //clear crypto module memory upon exit
  crypto_free_cipher(crypto_mod);
}

module_init(sbd_init);
module_exit(sbd_exit);
