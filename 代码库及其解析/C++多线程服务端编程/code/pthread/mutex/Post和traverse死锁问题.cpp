#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <iostream>

/*

总问题：：一边插入一边读可能造成死锁或者迭代器失效


read端问题  临界区太大，多端调用容易阻塞
write端问题 
             关键看 write 端的 post() 该如何写。按照前面的描述，如果  g_foos.unique() 为 true , 
             我们可以放心地在原地( in-place) 修改 FooList。如果 g_foos.unique() 为 false ，
             说明这时别的线程正在读取 FooList , 我们不能原地修改，而是复制一份, 
             在副本上修改。这样就避免了死锁。


*/
using namespace std;

#define NUM_THREADS     5

class Foo{
public:
    Foo(long key) {
        key_ = key;
        cout << "Foo::init  key = " << key_ << endl;
    }
    ~Foo() {
        key_ = 0;
        cout << "Foo::~Foo" << endl;
    }
    void doit() const{
        cout << "Foo::doit  key = " << key_ << endl;
    }
private:
    long key_;
};

shared_ptr< std::vector<Foo> > shared_foos;
pthread_mutex_t g_mutex;
std::vector<Foo> foos ;

void post(const Foo& f)
{
	pthread_mutex_lock(&g_mutex);
	foos.push_back(f);
    pthread_mutex_unlock(&g_mutex);
}

void post2(const Foo& f)//改进版
{
	printf("postIn");
	pthread_mutex_lock(&g_mutex);
	if (!shared_foos.unique())
	{
		shared_foos.reset(new std::vector<Foo>(*shared_foos));
		printf("copy the whole listln"); //练习：将这句话移出临界区
	}
	assert(shared_foos.unique());
	shared_foos->push_back(f);
    pthread_mutex_unlock(&g_mutex);
}

void traverse()
{
	pthread_mutex_lock(&g_mutex);
	for (std::vector<Foo>::const_iterator it = foos.begin();
				it != foos.end(); ++it)
    {
			it->doit();
    }//读取时临界区过大
    pthread_mutex_unlock(&g_mutex);
}

void traverse2()//改进版
{
    //缩小临界区，就算多个线程调用traverse2也不会阻塞
	pthread_mutex_lock(&g_mutex);
    std::vector<Foo> local_foos = foos;
    pthread_mutex_unlock(&g_mutex);

	for (std::vector<Foo>::const_iterator it = local_foos.begin();
				it != local_foos.end(); ++it)
    {
			it->doit();
    }
}

void *thread_post(void * attr){
    long thiskey = (long)(attr);
    Foo* newfoo = new Foo(thiskey);

    post(*newfoo);
}

void *thread_traverse(void * attr){
    traverse();
}



int main (int argc, char *argv[])
 {
    pthread_t threads[NUM_THREADS];
    long taskids[NUM_THREADS];
    int rc;
    long t;

    pthread_mutex_init(&g_mutex, 0);

    for(t=0; t < NUM_THREADS; t++){
       taskids[t] = 10000 + t ;
       printf("In main: creating thread %ld\n", t);
       rc = pthread_create(&threads[t], NULL, thread_post, (void *)taskids[t]);
       if (rc){
          printf("ERROR; return code from pthread_create() is %d\n", rc);
          exit(-1);
       }
    }

    for(t=0; t < NUM_THREADS; t++){
       taskids[t] = 10000 + t ;
       printf("In main: creating read thread %ld\n", t);
       rc = pthread_create(&threads[t], NULL, thread_traverse, NULL);
       if (rc){
          printf("ERROR; return code from pthread_create() is %d\n", rc);
          exit(-1);
       }
    }

    cout<<"over"<<endl;
    /* Last thing that main() should do */
    pthread_exit(NULL);
 }