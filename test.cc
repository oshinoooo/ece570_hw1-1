#include <iostream>

#include "thread.h"
#include "interrupt.h"

using namespace std;

unsigned int lock = 1;
unsigned int cond = 2;

bool flag = true;

void test1(void* ptr) {
    thread_lock(lock);

    // 1
    int num = *((int*)ptr);

    while (flag) {
        cout << "waiting..." << endl;
        thread_wait(lock, cond);
    }

    // 2
    int num = *((int*)ptr);

    cout << "stop waiting." << endl;

    for (int i = 0; i < 100; ++i) {
        cout << num << " ";
    }
    cout << endl;

    thread_unlock(lock);
}

void test2(void* ptr) {
    thread_lock(lock);

    for (int i = 0; i < 100; ++i) {
        cout << 'G' << " ";
    }
    cout << endl;

    flag = false;

    thread_unlock(lock);

    thread_broadcast(lock, cond);
}

void start(void* ptr) {
    int num = 1;
    thread_create(test1, &num);
    thread_create(test2, nullptr);
}

int main() {
    thread_libinit(start, nullptr);
    return 0;
}