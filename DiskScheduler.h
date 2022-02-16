#ifndef ECE570_HW1_WINDOWS_DISKSCHEDULER_H
#define ECE570_HW1_WINDOWS_DISKSCHEDULER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>

using namespace std;

class DiskScheduler {
public:
    DiskScheduler(int max_disk_queue, int number_of_requesters);
    void sendRequest(int requester_id, queue<string> tracks);
    void process();

private:
    int current_position;
    int max_disk_queue;
    int number_of_requesters;
    int current_buffer_size;
    mutex m;
    condition_variable c_v;
    map<int, string> buffer;
};

#endif
