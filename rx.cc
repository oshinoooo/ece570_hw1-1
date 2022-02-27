#include <cstdlib>
#include <stdio.h>
#include <iostream>
#include <deque>
#include <fstream>
#include <algorithm>
#include "thread.h"
#include "interrupt.h"

using namespace std;

struct Request {
    int id;
    int track;
};

int max_disk_queue, threadNum, lastTrack;
unsigned int mutexT, waitTillFull, waitTillAvailable;
char** filenames;
// 理解清楚这个serviced数组的用处，使得一个requester中的不同序号request保持同步，即一个被服务了才能发出下一个的请求！
bool* serviced;
deque<Request*> diskQ;

int abs(int i) {
    return (i > 0) ? i : -i;
}

bool cmp(Request* i, Request* j) {
    return (abs(i->track - lastTrack) < abs(j->track - lastTrack));
}

// disk队列是否满了
bool isFull() {
    return max_disk_queue <= diskQ.size();
}

// 发送请求
void sendRequest(Request* r) {
    cout << "requester " << r->id << " track " << r->track << endl;
    diskQ.push_back(r);
    sort(diskQ.begin(), diskQ.end(), cmp);
    serviced[r->id] = false; // 已进入请求队列但是还没被服务
}

void serviceRequest() {
    Request* r = diskQ.front(); // 服务队首线程
    diskQ.pop_front();
    cout << "service requester " << r->id << " track " << r->track << endl;
    serviced[r->id] = true; // 被服务后就置为真
    lastTrack = r->track;
    delete r;
}

void requester(void* id) {
    long requesterID = (long)id; // 64位系统 void* 为8字节，用long来接，避免报错说精度损失
    ifstream in(filenames[requesterID]);
    int track = 0;
    Request* r;

    while (true) {
        in >> track; // 按行读取不同序号的request
        if(in.eof()) break;

        r = new Request;
        r->id = (int)requesterID;
        r->track = track;
        thread_lock(mutexT);

        while (isFull() || !serviced[r->id]) // || 右边表示这个requester中的上一个序号的request处理了才能发出下一个
            thread_wait(mutexT, waitTillAvailable);

        sendRequest(r);
        // 叫醒waitTillFull队列中的线程，他们再次查看队列线程数是否达到max_disk_queue以尽可能有最多选择
        thread_broadcast(mutexT, waitTillFull);
    }
    in.close();

    while (!serviced[requesterID])  // 当这个requester中的所有序号的request都处理完了，serviced位置为真就可以跳出这个while等待了
        thread_wait(mutexT, waitTillAvailable);

    threadNum--; // 因为都处理完了，所以线程数减少1
    if (threadNum < max_disk_queue) { // When fewer than max_disk_queue requester threads are alive,
        max_disk_queue--; // the largest number of requests in the queue is equal to the number of living requester threads.
        thread_broadcast(mutexT, waitTillFull);
    }
    thread_unlock(mutexT);
}

void service(void* arg) {
    thread_lock(mutexT);
    while (max_disk_queue > 0) {
        while (!isFull()) // 队列中线程数还不到传入的限制参数时，等待，以实现
            thread_wait(mutexT, waitTillFull); // the disk queue has the largest possible number of requests.
            // thread_wait释放锁并把线程放到condition的等待队列中，这里的condition队列是waitTillFull
        if (!diskQ.empty())
            serviceRequest();
        // 叫醒所有在condition等待队列waitTillAvailable上的的线程，上面服务了一个request了，那现在diskQ队列肯定有空位了
        thread_broadcast(mutexT, waitTillAvailable);
    }
    thread_unlock(mutexT);
}

void start(void* arg) {
    thread_create(service, NULL); // 开启服务线程

    for (int i = 0; i < threadNum; i++) { // 开启threadNum条请求线程
        thread_create(requester, (void*)(long)i);
    }
}

int main(int argc, char** argv) {
    if (argc <= 2)
        return 0;

    max_disk_queue = atoi(argv[1]); // 手动传入的最大队列长度
    threadNum = argc - 2; // 推导出来的线程数量
    lastTrack = 0;
    mutexT = 1;
    waitTillFull = 2; // 设为任意值均可
    waitTillAvailable = 3; // // 设为任意值均可

    serviced = new bool[threadNum]; // 初始化服务列表
    for (int i = 0; i < threadNum; i++) {
        serviced[i] = true;
    }
    // the input file for requester r is argv[r+2], where 0 <= r < (number of requesters).
    filenames = argv + 2; // char* argv[] = char** argv 这样就好理解了

    thread_libinit(&start, NULL);

    delete serviced;
    return 0;
}