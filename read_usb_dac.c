#include<stdio.h>
#include <fcntl.h>

#define MAX_PKT_SIZE 8
static  char dac_op_buf[MAX_PKT_SIZE];
static  char file_data_buf[MAX_PKT_SIZE]={0x01,0x02,0x03,0x04,0x05,0x06,0x02,0x03};

int main()
{
   int fd;
   int i=0;
   int wr_cnt=0;
   int rd_cnt=0;

   fd=open("/dev/lpc0",O_RDWR);
   printf("fd=%d\n",fd); 
   perror("open");

#if 0 
//   for(i=1;i<4;i++)
   for(i=1;i<2;i++)
   {
      wr_cnt = write(fd,file_data_buf,MAX_PKT_SIZE);
      printf("wr_cnt = %d\n",wr_cnt);

       sleep(1);
   }
#endif

#if 0
      rd_cnt=read(fd,dac_op_buf,MAX_PKT_SIZE);
      rd_cnt=read(fd,dac_op_buf,MAX_PKT_SIZE+3);
      rd_cnt=read(fd,dac_op_buf,MAX_PKT_SIZE+5);
      rd_cnt=read(fd,dac_op_buf,MAX_PKT_SIZE+8);
#endif

#if 1 
// for(i=1;i<3;i++)
   for(i=1;i<2;i++)
   {
//      rd_cnt=read(fd,dac_op_buf,MAX_PKT_SIZE + 16);
 //     rd_cnt=read(fd,dac_op_buf,MAX_PKT_SIZE + 10);
    rd_cnt=read(fd,dac_op_buf,MAX_PKT_SIZE);

//      rd_cnt=read(fd,dac_op_buf,3);
      printf("rd_cnt = %d\n",rd_cnt);
      printf("dac_op_buf=%s\n",dac_op_buf);
      //sleep(10);
   }
#endif
   close(fd);

return 0;
}




#if 0
int main()
{
int fd;
int i=0;
fd=open("/dev/custom0",O_RDWR);
printf("fd=%d\n",fd);
perror("open");
/*
for(i=1;i<10;i++)
{
read(fd,0,0);
sleep(2);
}
*/


for(i=1;i<10;i++)
{
write(fd,0,0);
sleep(2);
}
close(fd);

return 0;
}

#endif

