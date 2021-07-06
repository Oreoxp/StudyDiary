 #include <pthread.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <math.h>
 #define NUM_THREADS	4

 void *BusyWork(void *t)
 {
    int i;
    long tid;
    double result=0.0;
    tid = (long)t;
    printf("线程： %ld 开始运行...\n",tid);
    for (i=0; i<1000000; i++)
    {
       result = result + sin(i) * tan(i);
    }
    printf("线程：%ld 完成. 返回值 = %e\n",tid, result);
    pthread_exit((void*) t);
 }

 int main (int argc, char *argv[])
 {
    pthread_t thread[NUM_THREADS];
    int rc;
    long t;
    void *status;

    // 创建线程属性对象
    pthread_attr_t attr;
    // 初始化线程分离属性 
    pthread_attr_init(&attr);
    // 设置线程分离属性    PTHREAD_CREATE_JOINABLE是使用下列函数前提
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for(t=0; t<NUM_THREADS; t++) {
       printf("Main: 创建 thread %ld\n", t);
       rc = pthread_create(&thread[t], &attr, BusyWork, (void *)t);  
       if (rc) {
          printf("ERROR：pthread_create() 的返回码是 %d\n", rc);
          exit(-1);
          }
       }

    // 释放属性
    pthread_attr_destroy(&attr);

   {// pthread_join()即是子线程合入主线程，主线程阻塞等待子线程结束，然后回收子线程资源。
    for(t=0; t<NUM_THREADS; t++) {
       rc = pthread_join(thread[t], &status);
       if (rc) {
          printf("ERROR：pthread_join() 的返回码是 %d\n", rc);
          exit(-1);
        }
        printf("Main: 已完成与状态为 %ld 的线程 %ld 的连接 \n", t, (long)status);
    }
   }

   //调试请注释上或下for循环

   {// pthread_detach()即主线程与子线程分离，子线程结束后，资源自动回收。
    for(t=0; t<NUM_THREADS; t++) {
       rc = pthread_detach(thread[t]);
       if (rc) {
          printf("ERROR：pthread_join() 的返回码是 %d\n", rc);
          exit(-1);
        }
        printf("Main: 已分离与状态为 %ld 的线程 %ld 的连接 \n", t, (long)status);
    }
   }
 
    printf("Main: 程序完成。 退出。\n");
    pthread_exit(NULL);
 }