仓库中的代码实现了

- [x] 无锁队列，CAS，没有解决ABA问题，代码见LockFreeQueue.h
- [x] 无锁栈，CAS，没有解决ABA问题，代码见LockFreeList.h
- [x] 单锁栈，代码见WithLockList.h
- [x] 单锁队列，代码见WithLockQueue.h
- [x] 双锁队列，代码见WithTwoLockQueue.h
- [ ] SPSC(single-producer/single-consumer)循环数组(TODO)
- [ ] 测试及性能对比(TODO)

仓库代码细节的文章介绍可以https://blog.csdn.net/qq_43593404/article/details/130598220?csdn_share_tail=%7B%22type%22%3A%22blog%22%2C%22rType%22%3A%22article%22%2C%22rId%22%3A%22130598220%22%2C%22source%22%3A%22qq_43593404%22%7D

下文同为仓库代码介绍

之前比赛一直有看到无锁化编程优化部分，但一直没有实践过，(这里主要是使用的CAS,当然lock-free实现其实有好几种，这取决于具体的硬件原语)。于是调研了一下无锁化队列和栈的工作(PS:市面上无锁化的讲解很多，但是往往是**碎片化**、**有bug**、**不够系统深入的**)于是我基于相关论文及boost等相关代码作了详细的调研分析，并加入了思考，得出该文。(本人非此方向的研究者，尽力思考得出的一些结论，但浅浅的调研一下已经发现内中的复杂性，欢迎交流指正)

首先，我们来阐述几个概念：

### 无锁化(lock free)，无等待(wait free)，非阻塞(non-blocking)

关于这些概念这里其实有多种分类的解释，我以一种概念解释为例，其他解释大体类似。

**阻塞与非阻塞**:从OS的角度出发，把这里的阻塞认为是操作系统中阻塞、线程调度的概念。这时阻塞是一个系统调用，当线程调用该阻塞调用时，当线程阻塞时进行睡眠状态，操作系统可以从调度器中删除该线程，此时该线程不会占用CPU的时间片，而另一个事件发生时，线程再被放回调度器，并分配时间片运行，此时正在运行阻塞调用的线程称为阻塞线程那么此时非阻塞函数就是那些不阻塞的函数。非阻塞数据结构是指所有操作都是非阻塞的数据结构。所有无锁数据结构本质上都是非阻塞的。此时我们知道**自旋锁(spinlock)**是非阻塞同步的一个例子:如果一个线程有锁，那么等待的线程不会挂起，而是必须循环，直到持有锁的线程释放它。自旋锁这种忙碌等待循环的算法并不是**lock free**的。(因为如果持有锁的线程挂起，那么没有线程可以继续运行。)



**lock free**:这里lock free和non-blocking主要区别就是如果在该lock free数据结构上执行操作的任何线程在该操作期间的任何时刻被挂起，那么访问该数据结构的其他线程必须仍然能够完成它们的任务。而lock-free的数据结构概念上是不使用任何互斥锁的数据结构，所谓的无锁有很多种无锁，即实现方式多种多样。举个例子，一个多线程访问的读写共享队列既可以通过**CAS(Compare And Swap)**，也可以通过**分区实现数据并行**，将共享队列拆分到多个线程，每个线程负责一个队列。(注：前者方案会出现**数据争抢、ABA问题**，后者方案则会出现**负载均衡**问题，由此出现work stealing的解决方案，该方案又会引出针对每个线程的队列加锁问题，这里问题的具体讨论我会放到线程池的方案总结与实现篇，这里先带过)。

同时无锁化的数据结构有可能是限制于某些场景的，也就是**by case**的。以生产者消费者队列为例，存在**SPSC(single-producer/single-consumer)**以及**MPMC(multi-producers/multi-consumers)**等。前者的无锁化不需要使用CAS，通过**ringbuffer**使用write index和read index两个index的原子变量来实现，而后者则需要可能需要CAS等硬件原语来实现，效率会差很多，此时前者ringbuffer的方案是不能用于后者SPSC的情况的。(注:**ringbuffer**两个index原子变量的方案也可以适用于一部分**SPMC(single-producer/multi-consumers)**的情况，但存在一些限制条件)

最后，数据结构是无锁的，并不意味着线程不需要相互等待。在无锁算法中，当存在高争用时，线程可能会发现它必须重试其操作的次数不受限制。也就是可能会出现**活锁(livelock)**的情况，这也是lock-free和wait-free的主要区别。



**wait-free**:wait-free数据结构是一种lock-free的数据结构，它具有一个附加属性，即不管其他线程的行为如何，访问该数据结构的每个线程都可以在有限的步骤内完成其操作。而由于与其他线程的冲突，可能涉及无限次重试的算法因此不是无等待的。换句话说，也就是没有**活锁(livelock)**这种情况。

![概念展示](C:\Users\偏执狂\Desktop\non-block\概念展示.png)

我这里实现的CAS无锁化栈和队列都存在ABA问题，关于CAS实现中ABA问题的具体解释和解决方案，可以参考：

https://en.wikipedia.org/wiki/ABA_problem

### ABA问题的简单阐述

wiki中给了一个例子，无锁化的栈实现

```C++
/* Naive lock-free stack which suffers from ABA problem.*/
class Stack {
  std::atomic<Obj*> top_ptr;
  //
  // Pops the top object and returns a pointer to it.
  //
  Obj* Pop() {
    while (1) {
      Obj* ret_ptr = top_ptr;
      if (!ret_ptr) return nullptr;
      // For simplicity, suppose that we can ensure that this dereference is safe
      // (i.e., that no other thread has popped the stack in the meantime).
      Obj* next_ptr = ret_ptr->next;
      // If the top node is still ret, then assume no one has changed the stack.
      // (That statement is not always true because of the ABA problem)
      // Atomically replace top with next.
      if (top_ptr.compare_exchange_weak(ret_ptr, next_ptr)) {
        return ret_ptr;
      }
      // The stack has changed, start over.
    }
  }
  //
  // Pushes the object specified by obj_ptr to stack.
  //
  void Push(Obj* obj_ptr) {
    while (1) {
      Obj* next_ptr = top_ptr;
      obj_ptr->next = next_ptr;
      // If the top node is still next, then assume no one has changed the stack.
      // (That statement is not always true because of the ABA problem)
      // Atomically replace top with obj.
      if (top_ptr.compare_exchange_weak(next_ptr, obj_ptr)) {
        return;
      }
      // The stack has changed, start over.
    }
  }
};
```

栈一开始 *top* → A → B → C

Thread 1 starts running pop:

```C++
ret = A;
next = B;
```

而后在CAS前被中断，走到Thread2

```C++
{ // Thread 2 runs pop:
  ret = A;
  next = B;
  compare_exchange_weak(A, B)  // Success, top = B
  return A;
} // Now the stack is top → B → C
{ // Thread 2 runs pop again:
  ret = B;
  next = C;
  compare_exchange_weak(B, C)  // Success, top = C
  return B;
} // Now the stack is top → C
delete B;
{ // Thread 2 now pushes A back onto the stack:
  A->next = C;
  compare_exchange_weak(C, A)  // Success, top = A
}
```

此时栈是top->A->C，然后thread1，compare_exchange_weak(A, B)，而此时B已经被Pop出去了，内存被释放，访问自由内存会产生未定义行为，导致崩溃。以上就是ABA问题的一个例子。

### 无锁化栈和队列的对比与实现

本文实现的无锁化栈和队列都是通过CAS(Compare And Swap)硬件原语实现，会涉及到相关的细节

解释完上面的问题，用CAS实现lock-free的时候，栈和队列是不同。我们这里假设栈和队列都是单链表实现，那么此时栈和队列是存在有区别的。

如图所示，栈是头插头出，队列是尾插头出，所以两者在push插入节点时会有较大区别。(实际上在pop上也存在一些区别，具体下面会讲)

![stack](C:\Users\偏执狂\Desktop\non-block\stack.png)

![queue](C:\Users\偏执狂\Desktop\non-block\queue.png)

我们知道，push插入节点有两个步骤:1.将新节点与原节点相连2.更新头或尾指针(head/tail)，这两步在头插(也就是栈中)，可以只使用一次CAS，而队列的尾插，则需要使用两次CAS，一次连接，一次更新head/tail指针

首先，从无锁化栈的push插入入手，核心代码如下所示

```C++
template<typename T>
class LockFreeList
{
    atomic<Node<T> *> head;
public:
    void pushFront(const T &value)
    {
        auto *node = new Node<T>(value);
        node->next = head.load();
        while(!head.compare_exchange_weak(node->next, node)); 
    }
};

```

首先解释一下CAS硬件原语，这条复杂的指令由硬件实现为原子性的，即A.CAS(B,C)，如果当前值A和期望值B相等时，把预测值C赋值给当前值A，如果当前值A与期望值不等时，把当前值A赋值给期望值B

简单解释一下流程：

node->next = head.load();对于每个线程来说，将新node与head相连，此时做CAS检查，head是否等于node->next，如果相等，head是该线程读取的head，node连接到head之前，而后将让head指针指向node节点，而如果head与node->next不相等，则说明此时的head已经被其他线程更新，则连接应该针对该新head重新连接，于是再次执行node->next = head.load()



无锁化队列的push如下

![队列无锁化插入](C:\Users\偏执狂\Desktop\non-block\队列无锁化插入.png)

无锁化队列的实现时，和无锁化栈的实现有一些区别，第一点是无锁化队列需要使用dummy node，即虚拟头节点，不然最后一个节点和第一个节点要特殊处理，比较麻烦。第二点是之前的头插改成了尾插，因此整体上要做两次CAS。

即E1-E3新建节点,E4-E16是做向尾部tail指针的next再连接一个新的节点node，执行这项连接操作时会有中间会有两种情况，如果是tail不是最后一个(即此时的tail被更新了)，即E12-E13，则先通过CAS将tail指针更新到next节点并继续循环，如果此时的tail是最后一个，则通过CAS执行连接操作即E9，退出循环，而后在循环外，通过CAS操作将tail指针移动到新节点。第三点，因为这里有两次CAS，且一次是node的next指针进行的CAS，因而node 的实现的next要重新实现用atomic<Node_New<T> *>next而不是之前的Node_New<T> * next。(注：这里我检查了一下boost库，里面也是类似的实现方式)

上面分析完了push，对于pop删除/出队而言。无锁化栈和无锁化队列仍有些许区别。

从无锁化栈的pop删除，核心代码如下：

```C++
bool popFront(T &value)
    {
        Node<T>* temp = head.load();//留一个快照snapshot
        do {
            if (temp == nullptr) return false;//这句判断要在循环里面
        } while (!head.compare_exchange_weak(temp,temp->next));
        value = temp->data;
        delete temp;
        return true;
    }
```

简单解释一下：

temp的判断要放到循环里面，用while随时判断temp是否为nullptr，如果head和temp相等的时候，要把head指针指向temp->next，而如果不等则证明此时的head被其他的线程更新过了，则将head 赋值给temp，再次进行循环。

虽然二者都是头插，看上去代码好像是可以一样的。但栈只有一个指针，而队列有两个指针要更新，所以会有一个corner case，即head==tail的时候，不一定是队列空了，也可能是此时的tail还没更新，如下面无锁化队列的pop删除，伪代码所示：

![队列无锁化删除](C:\Users\偏执狂\Desktop\non-block\队列无锁化删除.png)

D6-D10展示了上述判断，不再赘述。

(注:解释一下D12行的备注，为什么要返回next的值？因为用了dummy node虚拟头节点，所以数据和真实的节点有错位(下面双锁队列也是如此)。这行代码为什么要在CAS之前。这是因为，要返回的值*pvalue是dequeue后新的head的值，也就是原来的next，而该next可能会被另一个线程释放掉，那为什么放在CAS之前该node就不会在得到pvalue之前释放掉呢，因为链表是连续不间断的，pvalue在CAS之前就说明当前节点还没有被CAS，那当前节点的next节点自然不可能完成CAS并释放)

无锁化栈的具体实现代码在https://github.com/xuhaoran1/Non-blocking-queue-and-stack/blob/master/LockFreeList.h

无锁化队列的具体实现代码在https://github.com/xuhaoran1/Non-blocking-queue-and-stack/blob/master/LockFreeQueue.h

### 双锁队列线程同步

最后给出双锁队列实现线程同步的分析及实现细节。

多线程阻塞队列的平凡实现是用一把大锁，在添加enqueue/push和删除dequeue/pop操作之前锁住整个队列，再进行操作。

但是，这其中其实有一个潜在的性能瓶颈：enqueue和dequeue操作都要锁住整个队列，而enqueue和dequeue的具体操作就会发现他们的操作其实不一定是冲突的，或者说大部分情况下是不冲突的，那这样做就是白白浪费性能。于是自然的想法把那个队列锁拆成两个：一个队列头部锁（head lock)和一个队列尾部锁(tail lock)。但是这里会有两个corner case存在一些问题，：第一种就是往空队列里插入第一个节点的时候，第二种就是从只剩最后一个节点的队列中删除最后那个节点的时候。当我们向空队列中插入第一个节点的时候，我们需要同时修改队列的head和tail指针，使他们同时指向这个新插入的节点，换句话说，我们此时即需要拿到head lock又需要拿到tail lock。而另一种情况是对只剩一个节点的队列进行dequeue的时候，我们也是需要同时修改head和tail指针使他们指向NULL，亦即我们需要同时获得head和tail lock。这具有极大的**死锁(deadlock)**风险。因此如果要这么设计的话，就要保证enqueue和dequeue对head lock和tail lock的请求顺序（lock ordering）是一致的等来避免死锁，然而这无法解决包含多次的加锁/解锁操作，造成不必要的开销问题。于是存在一个trick，构造dummy node，虚拟头节点，同时，保持队列头出尾入。

算法伪代码如下所示

```C++
typedef struct node_t {
    TYPE value; 
    node_t *next
} NODE;
 
typedef struct queue_t {
    NODE *head; 
    NODE *tail;
    LOCK q_h_lock;
    LOCK q_t_lock;
} Q;
 
initialize(Q *q) {
   node = new_node()   // Allocate a free node
   node->next = NULL   // Make it the only node in the linked list
   q->head = q->tail = node   // Both head and tail point to it
   q->q_h_lock = q->q_t_lock = FREE   // Locks are initially free
}
 
enqueue(Q *q, TYPE value) {
   node = new_node()       // Allocate a new node from the free list
   node->value = value     // Copy enqueued value into node
   node->next = NULL       // Set next pointer of node to NULL
   lock(&q->q_t_lock)      // Acquire t_lock in order to access Tail
      q->tail->next = node // Link node at the end of the queue
      q->tail = node       // Swing Tail to node
   unlock(&q->q_t_lock)    // Release t_lock
｝
 
dequeue(Q *q, TYPE *pvalue) {
   lock(&q->q_h_lock)   // Acquire h_lock in order to access Head
      node = q->head    // Read Head
      new_head = node->next       // Read next pointer
      if new_head == NULL         // Is queue empty?
         unlock(&q->q_h_lock)     // Release h_lock before return
         return FALSE             // Queue was empty
      endif
      *pvalue = new_head->value   // Queue not empty, read value
      q->head = new_head  // Swing Head to next node
   unlock(&q->q_h_lock)   // Release h_lock
   free(node)             // Free node
   return TRUE            // Queue was not empty, dequeue succeeded
}
```

这个算法中队列总会包含至少一个节点。dequeue每次返回的不是头节点，而是头节点的下一个节点中的数据：如果head->next不为空的话就把这个节点的数据取出来作为返回值，同时再把head指针指向这个节点，此时旧的头节点就可以被free掉了。这个在队列初始化时插入空节点的技巧使得enqueue和dequeue彻底相互独立了，也就不需要同时获得两把锁了。

实现代码在https://github.com/xuhaoran1/Non-blocking-queue-and-stack/blob/master/WithTwoLockQueue.h

**本文参考:**

wiki:https://en.wikipedia.org/wiki/ABA_problem

paper:

Simple, Fast, and Practical Non-Blocking and Blocking Concurrent Queue Algorithms

Implementing Lock-Free Queues