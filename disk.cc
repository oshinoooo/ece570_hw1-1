#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <fstream>
#include <limits.h>
#include <utility>
#include <thread>

#include "thread.h"
#include "interrupt.h"

using namespace std;

unsigned int lock = 0;
unsigned int cond = 1;
//unsigned int cond_schedulers = 2;

long current_position;
long max_disk_queue;
long number_of_requesters;
long current_buffer_size;
vector<queue<string>> requests;
map<long, string> buffer;

// OK
vector<queue<string>> getRequests(queue<string>& paths) {
    vector<queue<string>> requests;
    while (!paths.empty()) {
        ifstream file(paths.front());
        if (!file.is_open())
            continue;

        queue<string> tracks;
        string track;
        while (file >> track)
            tracks.push(track);

        file.close();

        requests.push_back(tracks);
        paths.pop();
    }
    return requests;
}

void sendRequest(void* ptr) {
    long requester_id =  (long)ptr;
    queue<string> tracks = requests[requester_id];

    while (!tracks.empty()) {
        thread_lock(lock);

        while(current_buffer_size <= buffer.size() || buffer.count(requester_id)){
            thread_wait(lock, cond);
        }

        buffer.insert({requester_id, tracks.front()});

        cout << "requester " << requester_id << " track " << tracks.front() << endl;

        tracks.pop();

//        if (tracks.empty()) {
//            --number_of_requesters;
//            current_buffer_size = min(max_disk_queue, number_of_requesters);
//        }

        thread_unlock(lock);
        thread_broadcast(lock, cond);
    }
}

void processRequest(void* ptr) {
    while (current_buffer_size != 0 || !buffer.empty()) {
        thread_lock(lock);

        while(current_buffer_size > buffer.size()){
            thread_wait(lock, cond);
        }

        long requester_id;
        long track;
        long minDistance = INT_MAX;
        for (const pair<long, string>& block : buffer) {
            long curDistance = abs(current_position - stoi(block.second));
            if (curDistance < minDistance) {
                minDistance = curDistance;
                requester_id = block.first;
                track = stoi(block.second);
            }
        }

        current_position = track;

        cout << "service requester " << requester_id << " track " << track << endl;

        buffer.erase(requester_id);

        if (requests[requester_id].empty()) {
            --number_of_requesters;
            current_buffer_size = min(max_disk_queue, number_of_requesters);
        }

        thread_unlock(lock);
        thread_broadcast(lock, cond);
    }
}

void startDiskScheduler(void* ptr) {
    thread_create(processRequest, nullptr);

    for (long i = 0; i < requests.size(); ++i) {
        long index = i;
        thread_create(sendRequest, (void*)index);
    }
}

// OK
int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Please enter correct arguments." << endl;
        return 0;
    }

    // get paths
    queue<string> paths;
    for (int i = 2; i < argc; ++i)
        paths.push(argv[i]);

    current_position = 0;
    max_disk_queue = stoi(argv[1]);
    number_of_requesters = paths.size();
    current_buffer_size = min(max_disk_queue, number_of_requesters);
    requests = getRequests(paths);

//    for (int i = 0; i < requests.size(); ++i) {
//        while (!requests[i].empty()) {
//            cout << requests[i].front() << endl;
//            requests[i].pop();
//        }
//    }
    
    thread_libinit(startDiskScheduler, nullptr);
    return 0;
}
