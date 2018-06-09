#include <linux/slab.h>
#include <linux/random.h>
#include <linux/module.h>

static int __init init_malloc(void){
  int i;
  int randBytes;
  int randBytes2;
  void* tmp;
  long freeSpace;
  long usedSpace;

  for (i = 0; i < 500; i++){
    get_random_bytes(&randBytes, sizeof(randBytes));
    get_random_bytes(&randBytes2, sizeof(randBytes2));

    tmp = kmalloc(randBytes, GFP_KERNEL);
    kfree(tmp);

    tmp = kmalloc(randBytes2, GFP_KERNEL);
    kfree(tmp);
  }

  freeSpace = syscall(359);
  usedSpace = syscall(360);

  printf("used space... %lu, free space... %lu\n", usedSpace, freeSpace);
}

static int void __exit exit_malloc(void){

}

module_init(init_malloc);
module_exit(mallo_exit);

MODULE_AUTHOR("Group 17");
MODULE_LICENSE("GPL");
