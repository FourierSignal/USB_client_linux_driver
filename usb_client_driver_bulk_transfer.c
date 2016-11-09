/*
PATH=$PATH:/home/jaguar/Documents/Emb_linux/em_lin/practice/Linux_srcARM/gcc-linaro-arm-linux-gnueabihf-4.9-2014.09_linux/bin/

sudo make ARCH=arm  CROSS_COMPILE=/home/jaguar/Documents/Emb_linux/em_lin/practice/Linux_srcARM/gcc-linaro-arm-linux-gnueabihf-4.9-2014.09_linux/bin/arm-linux-gnueabihf-   -C /root/beagleboneb_emb_linux_2015/emb_linux_kernel1/bb-kernel/KERNEL  M=`pwd` modules

cp custom_transfer_usb.ko  /home/jaguar/Documents/Emb_linux/em_lin/practice/rootfs_3/rfs_bb_static_1.17.4/usr/lib/modules/
PATH=$PATH:/home/jaguar/Documents/Emb_linux/em_lin/practice/Linux_srcARM/gcc-linaro-arm-linux-gnueabihf-4.9-2014.09_linux/bin/

(or)

sudo make -C  /lib/modules/4.3.2/build/  M=`pwd` modules


*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include<linux/kfifo.h>
#include <linux/delay.h>
#if 1
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kref.h>
#include <linux/usb.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#endif



//#define MAX_PKT_SIZE 1
//#define MAX_PKT_SIZE 8

#define USB_LPC1768_MINOR_BASE	0


/*
#define CUSTOM_EP_IN       0x83
#define CUSTOM_EP_OUT      0x02
*/
/*
#define CUSTOM_EP_IN       0x81
#define CUSTOM_EP_OUT      0x01
*/
static int custom_probe(struct usb_interface *interface, const struct usb_device_id *id);
static void custom_disconnect(struct usb_interface *interface);
static void lpc1768_delete(struct kref *pkref);

/* Structure to hold all of our device specific stuff */
struct usb_lpc1768 {
   struct usb_device *device;	     /* the usb device for this device */
   struct usb_interface *interface;  /* the interface for this device */
   struct kfifo Rx_kfifo_obj;
   wait_queue_head_t Rd_waitQ;
   char *bulk_in_buffer;    /* the buffer to receive data */
   size_t bulk_in_size;		     /* the size of the receive buffer */
   __u8	bulk_in_endpointAddr;	     /* the address of the bulk in endpoint */
   __u8	bulk_out_endpointAddr;       /* the address of the bulk out endpoint */

   unsigned char *bulk_out_buffer;    /* the buffer to receive data */


   struct kref kref;
};


static struct usb_device_id custom_table[] =
{
	{ USB_DEVICE(0x1ff9, 0x0520) },
	{} /* Terminating entry */
};
MODULE_DEVICE_TABLE (usb, custom_table);

static struct usb_driver custom_driver =
{
	.name = "custom_driver2",
	.id_table = custom_table,
	.probe = custom_probe,
	.disconnect = custom_disconnect,
};



#if 0
static struct usb_device *device;
static struct usb_class_driver class;
#endif

/*
static  char dac_op_buf;
static  char dac_op_buf[MAX_PKT_SIZE];
static  char file_data_buf[MAX_PKT_SIZE]={0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
*/

static int lpc_open(struct inode *inode, struct file *file)
{
   struct usb_lpc1768 *dev;
   struct usb_interface *interface;
   int retval = 0;

   interface = usb_find_interface(&custom_driver, iminor(inode));
   if (!interface) {
      printk (KERN_INFO "%s - error, can't find device for minor %d",__FUNCTION__, iminor(inode));
      retval = -ENODEV;
      goto exit;
   }

   dev = usb_get_intfdata(interface);
   if (!dev) {
      retval = -ENODEV;
      goto exit;
   }

   /* increment our usage count for the device */
   kref_get(&dev->kref);
   
   /* save our object in the file's private structure */
   file->private_data = dev;

exit:
   return retval;
}


static int lpc_close(struct inode *inode, struct file *file)
{

   struct usb_lpc1768 *dev;
   
   dev = (struct usb_lpc1768 *)file->private_data;
   if (dev == NULL) return -ENODEV;

   /* decrement the count on our device */
   kref_put(&dev->kref, lpc1768_delete);

   file->private_data = NULL;

	return 0;
}



static void lpc_dac_read_callback(struct urb *urb)
{
   struct usb_lpc1768 *dev;
   printk("\n%s ",__FUNCTION__);
   if (urb->status && !(urb->status == -ENOENT || urb->status == -ECONNRESET || urb->status == -ESHUTDOWN))
   {
      printk("\nurb->status %d",urb->status);
   }
   printk("urb->status=%d\n",urb->status);

   dev=(struct usb_lpc1768 *)urb->context;
   //wakeup blocked read
  // wake_up(&(dev->Rd_waitQ)); // mark Any sleeping RD-thread as Runnable.
//  kfree(urb->transfer_buffer);
//    printk("data=%s\n",dev->bulk_in_buffer);
    printk("data=%s\n",urb->transfer_buffer);
   printk(KERN_INFO "alloc: pktbuff=%p  actual_length=%d transfer_buffer_length=%d\n", urb->transfer_buffer,urb->actual_length,urb->transfer_buffer_length);
   printk(KERN_INFO "number_of_packets=%d error_count=%d\n",urb->number_of_packets,urb->error_count);
    kfree(urb->transfer_buffer);
   urb->transfer_buffer = NULL;
}






static ssize_t lpc_dac_read(struct file *file, char __user *buf, size_t count, loff_t *off)
{

    int retval = 0;
    int read_cnt=0;
    struct urb *urb = NULL;
   struct usb_lpc1768 *dev;
   dev = (struct usb_lpc1768 *)file->private_data;
   char *pktbuff;
   int pktbuff_size;
   printk("\n %s ",__FUNCTION__);
#if 0
    retval = usb_bulk_msg(dev->device, usb_rcvbulkpipe(dev->device, dev->bulk_in_endpointAddr),
			                  dev->bulk_in_buffer, min(dev->bulk_in_size, count), &read_cnt,100);
	if (retval)
	{
		printk(KERN_ERR "Bi mesg ret=%d\n", retval);
	//	return retval;
        goto error;
	}

   	printk(KERN_ERR "read_cnt=%d\n",read_cnt);
   	printk(KERN_ERR "dev->bulk_in_buffer=%s\n",dev->bulk_in_buffer);

  if (copy_to_user(dev->bulk_in_buffer, buf, count)) {
      retval = -EFAULT;
      goto error;
   }


#endif

#if 1
   urb = usb_alloc_urb(0, GFP_KERNEL);
   if (!urb) {
      retval = -ENOMEM;
      goto error;
   }

   printk(KERN_INFO "alloc: urb=%p\n", urb);

//   pktbuff_size = count;
#if 1
   if(count % 8)
   pktbuff_size=((count/8)+1)*8;
   else
   pktbuff_size=(count/8)*8;
#endif

//   pktbuff= kmalloc(count, GFP_KERNEL);
   pktbuff= kmalloc(pktbuff_size, GFP_KERNEL);
   if (!pktbuff)
   {
      printk(KERN_INFO "Could not allocate bulk_in_buffer");
      goto error;
   }

   printk(KERN_INFO "alloc: pktbuff=%p  count=%d\n", pktbuff,count);

   urb->transfer_buffer = pktbuff;
   urb->transfer_buffer_length = pktbuff_size;

   printk(KERN_INFO "alloc: pktbuff=%p  actual_length=%d transfer_buffer_length=%d\n", urb->transfer_buffer,urb->actual_length,urb->transfer_buffer_length);

   /* initialize the urb */
   usb_fill_bulk_urb(urb,dev->device,usb_rcvbulkpipe(dev->device, dev->bulk_in_endpointAddr),
                         urb->transfer_buffer,pktbuff_size,lpc_dac_read_callback,dev);
   
   /* send the data out the bulk port */
   retval = usb_submit_urb(urb, GFP_KERNEL);
   if (retval) {
      printk(KERN_INFO "\n-failed submitting read urb, error %d\n",retval);
      goto error;
   }
   printk(KERN_INFO "\n-submitting read urb, retval %d\n", retval);
//   block on cond

   /* release our reference to this urb, the USB core will eventually free it entirely */
   usb_free_urb(urb);

#endif
#if 0
   mdelay(5);   

   printk("urb->status=%d\n",urb->status);
   mdelay(5);
   printk("urb->status=%d\n",urb->status);

   mdelay(5);
   printk("urb->status=%d\n",urb->status);

   mdelay(5);
   printk("urb->status=%d\n",urb->status);
   mdelay(5);
   printk("urb->status=%d\n",urb->status);

#endif

#if 0

    if(kfifo_is_empty(&(dev->Rx_kfifo_obj)) )   //Buffer is empty - dont allow read
    {
        if(file->f_flags & O_NONBLOCK)
            return -EAGAIN; //this will set error number to 11 but return value of read system call will be -1
        else //for blocking mode of operation
        {
            printk("Rd:adding user process to read waitQ -\n");
            //user process will be suspended till data becomes available in the kfifo of device
            retval = wait_event_interruptible(dev->Rd_waitQ,(kfifo_len(&(dev->Rx_kfifo_obj)) > 0));
            if (retval < 0)
            {
                return retval;  //-ERESTARTSYS if it was interrupted by a signal
            }
            if (retval == 0) //condition evaluated to false after timeout.
            {
             ; //printk("RDwaitQ-woke up by ISR\n");
            }
            printk(KERN_DEBUG "Rd:process %i (%s) wokeup\n",current->pid, current->comm);
        }
    }

    if(access_ok(VERIFY_WRITE,(void __user*)buf,(unsigned long)count))
    {
        //printk("Rd:access_ok for __user*)buf\n");
        read_cnt = kfifo_out(&(dev->Rx_kfifo_obj),(unsigned char*)buf,count);
        //printk("%d bytes:%s read from kfifo:Rd\n",read_cnt,buf);
        return read_cnt;
    }
    else
    {
        //printk("Rd:access_  Not ok for __user*)buf\n");
        return -EFAULT;
    }
#endif
error:
   return retval;
}



//char *buf = NULL;

static void lpc_file_write_callback(struct urb *urb)
{

   printk("%s \n",__FUNCTION__);
   if (urb->status && !(urb->status == -ENOENT || urb->status == -ECONNRESET || urb->status == -ESHUTDOWN))
   {
      printk("%s - nonzero write bulk status received: %d", __FUNCTION__, urb->status);
   }
      printk("urb->status=%d\n",urb->status);
  // usb_free_coherent(urb->dev, urb->transfer_buffer_length,urb->transfer_buffer, urb->transfer_dma);

  kfree(urb->transfer_buffer);
  urb->transfer_buffer = NULL;
}

static ssize_t lpc_file_write(struct file *file, const char __user *user_buf, size_t count,loff_t *off)
{

   int retval = 0;
   struct urb *urb = NULL;
   char *buf = NULL;
   int wr_cnt=0;

   struct usb_lpc1768 *dev;
   dev = (struct usb_lpc1768 *)file->private_data;

   /* verify that we actually have some data to write */
   if (count == 0) 
      return retval;

#if 0

   buf = kmalloc(count, GFP_KERNEL);
   if (!buf) {
      retval = -ENOMEM;
      return retval;
   }

   if (copy_from_user(buf, user_buf, count)) {
      retval = -EFAULT;
      goto error;
   }


    retval = usb_bulk_msg(dev->device, usb_sndbulkpipe(dev->device,dev->bulk_out_endpointAddr),
			 &buf, count, &wr_cnt,0);
	if (retval)
	{
		printk(KERN_ERR "Bo mesg ret=%d\n", retval);
		return retval;
	}

   	printk(KERN_ERR "write_cnt=%d\n",wr_cnt);
#endif

#if 1
   printk(KERN_INFO "%s \n",__FUNCTION__);

   urb = usb_alloc_urb(0, GFP_KERNEL);
   if (!urb) {
      retval = -ENOMEM;
      goto error;
   }
   if (count == 0) goto exit;
   /*usb_buffer_alloc*/
//   buf = usb_alloc_coherent(dev->device, count, GFP_KERNEL, &urb->transfer_dma);
   buf = kmalloc(count, GFP_KERNEL);
   if (!buf) {
      retval = -ENOMEM;
      goto error;
   }
   urb->transfer_buffer = buf;

   if (copy_from_user(buf, user_buf, count)) {
      retval = -EFAULT;
      goto error;
   }


//   memcpy(buf,file_data_buf,count);

   /* initialize the urb */
   usb_fill_bulk_urb(urb,dev->device,usb_sndbulkpipe(dev->device,dev->bulk_out_endpointAddr),
                         urb->transfer_buffer,count,lpc_file_write_callback,dev);

   //urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
   
   /* send the data out the bulk port */
   retval = usb_submit_urb(urb, GFP_KERNEL);
   if (retval) {
      printk(KERN_INFO "\n- failed submitting write urb, error %d",retval);
      goto error;
   }
   printk(KERN_INFO "\n- submitting write urb, retval %d",retval);
   /* release our reference to this urb, the USB core will eventually free it entirely */
   usb_free_urb(urb);

exit:
   return count;
#endif
error:
   //usb_free_coherent(dev->device, count, buf, urb->transfer_dma);
   // usb_free_urb(urb);
   kfree(buf);
   return retval;

}



static struct file_operations lpc1768_fops =
{
	.owner = THIS_MODULE,
	.open = lpc_open,
	.release = lpc_close,
	.read = lpc_dac_read,
	.write = lpc_file_write,
};

/* 
 * usb class driver info in order to get a minor number from the usb core,
 * and to have the device registered with devfs and the driver core
 */
static struct usb_class_driver lpc1768_class = {
   .name = "usb/lpc%d",
   .fops = &lpc1768_fops,
//   .devnode = S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH,
   .minor_base = USB_LPC1768_MINOR_BASE,
};

/*
 struct usb_lpc1768 *dev = container_of(d,struct usb_lpc1768,kref);

#define container_of(ptr, type, member) 
({ const typeof( ((type *)0)->member )  *__mptr = (ptr); (type *)( (char *)__mptr - offsetof(type,member) );})
*/
static void lpc1768_delete(struct kref *pkref)
{	
   struct usb_lpc1768 *dev = container_of(pkref,struct usb_lpc1768,kref);
   usb_put_dev(dev->device);
   kfree (dev->bulk_in_buffer);
   kfree (dev);
}

static int custom_probe(struct usb_interface *interface, const struct usb_device_id *id)
{

    struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
    int i;
    int retval=0;
    struct usb_lpc1768 *dev = NULL;
printk("custom_probe:\n"); 
    dev = kmalloc(sizeof(struct usb_lpc1768), GFP_KERNEL);
    if (dev == NULL) {
      printk(KERN_INFO "Out of memory");
      goto error;
    }
    memset(dev, 0x00, sizeof (*dev));
    kref_init(&dev->kref);

   dev->device = usb_get_dev(interface_to_usbdev(interface));
   dev->interface = interface;

   iface_desc = interface->cur_altsetting;
   for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i)
   {
      endpoint = &iface_desc->endpoint[i].desc;
      if (!dev->bulk_in_endpointAddr &&(endpoint->bEndpointAddress & USB_DIR_IN) 
                   &&((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)== USB_ENDPOINT_XFER_BULK))
      {
	     /* we found a bulk in endpoint - save information */
	     dev->bulk_in_endpointAddr = endpoint->bEndpointAddress;
//	     dev->bulk_in_size = endpoint->wMaxPacketSize;
	     dev->bulk_in_size = 8;
	     dev->bulk_in_buffer = kmalloc(dev->bulk_in_size, GFP_KERNEL);
	     if (!dev->bulk_in_buffer)
         {
	        printk(KERN_INFO "Could not allocate bulk_in_buffer");
	        goto error;
	     }
#if 0         
        kfifo_init(&(dev->Rx_kfifo_obj), dev->bulk_in_buffer, dev->bulk_in_size);  
        init_waitqueue_head(&dev->Rd_waitQ);       
#endif
      }


      if (!dev->bulk_out_endpointAddr && !(endpoint->bEndpointAddress & USB_DIR_IN)
                 &&((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)== USB_ENDPOINT_XFER_BULK))
      {
	    /* we found a bulk out endpoint */
	    dev->bulk_out_endpointAddr = endpoint->bEndpointAddress;
      }

    }
      if (!(dev->bulk_in_endpointAddr && dev->bulk_out_endpointAddr))
      {
         printk(KERN_INFO "Could not find both bulk-in and bulk-out endpoints");
         goto error;
      }

      /* save our data pointer in this interface  - needed later */
      usb_set_intfdata(interface, dev);

       /*  register the device now, as it is ready */
      retval = usb_register_dev(interface, &lpc1768_class);
      if (retval)
      {
         /* something prevented us from registering this driver */
         printk(KERN_INFO "Not able to get a minor for this device.");
         usb_set_intfdata(interface, NULL);
         goto error;
      }

      /* let the user know what node this device is now attached to */
      printk(KERN_INFO "USB lpc1768 device now attached to lpc%d",interface->minor);



      printk(KERN_INFO "custom device (%04X:%04X) plugged\n", id->idVendor,id->idProduct);
      printk(KERN_INFO "ID->bInterfaceClass: %02X\n",iface_desc->desc.bInterfaceClass);
      printk(KERN_INFO "ID->bNumEndpoints: %02X\n",iface_desc->desc.bNumEndpoints);

      for (i = 0; i < iface_desc->desc.bNumEndpoints; i++){
      endpoint = &iface_desc->endpoint[i].desc;
      printk(KERN_INFO "\nEp[%d]->bEndpointAddress: 0x%02X\n",i, endpoint->bEndpointAddress);
      printk(KERN_INFO "Ep[%d]->bmAttributes: 0x%02X\n",i, endpoint->bmAttributes);
      printk(KERN_INFO "Ep[%d]->wMaxPacketSize: 0x%04X (%d)\n",i, endpoint->wMaxPacketSize,endpoint->wMaxPacketSize);
      }

      
      return 0;

error:
/*
   usb_put_dev(dev->device);
   kfree (dev->bulk_in_buffer);
   kfree (dev);
*/
   if (dev) kref_put(&dev->kref, lpc1768_delete);
   return retval;


#if 0
      device = interface_to_usbdev(interface);
      class.name = "usb/custom%d";
	  class.fops = &fops;
	  if ((retval = usb_register_dev(interface, &class)) < 0)
	  {
		/* Something prevented us from registering this driver */
		printk(KERN_ERR " %dNot able to get a minor for this device.",retval);
	  }
	  else
	  {
		printk(KERN_INFO "Minor obtained: %d\n", interface->minor);
	  }
return retval;
#endif
}


static void custom_disconnect(struct usb_interface *interface)
{

//   usb_deregister_dev(interface, &class);

   struct usb_lpc1768 *dev;
   int minor = interface->minor;
printk("custom_disconnect:\n");

   dev = usb_get_intfdata(interface);
   usb_set_intfdata(interface, NULL);

   usb_deregister_dev(interface, &lpc1768_class);

   /* decrement our usage count */
   kref_put(&dev->kref, lpc1768_delete);
   printk(KERN_INFO "custom intf%d now disconnected\n",interface->cur_altsetting->desc.bInterfaceNumber);
   printk(KERN_INFO "lpc%d now removed\n", minor);

}


static int __init custom_init(void)
{
	return usb_register(&custom_driver);
}

static void __exit custom_exit(void)
{
	usb_deregister(&custom_driver);
}

module_init(custom_init);
module_exit(custom_exit);

MODULE_LICENSE("GPL");
//MODULE_DESCRIPTION("USB CUSTOM Driver");
