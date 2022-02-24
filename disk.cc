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
unsigned int full_buffer = 1;
unsigned int available_buffer = 2;

long current_position;
long max_disk_queue;
long number_of_requesters;
long specified_buffer_size;
vector<queue<string>> requests;
map<long, string> buffer;

void init(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Please enter correct arguments." << endl;
        return;
    }

    queue<string> paths;
    for (int i = 2; i < argc; ++i)
        paths.push(argv[i]);

    current_position = 0;
    max_disk_queue = stoi(argv[1]);
    number_of_requesters = paths.size();
    specified_buffer_size = min(max_disk_queue, number_of_requesters);
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
}

void sendRequest(void* ptr) {
    thread_lock(lock);

    long requester_id = (long)ptr;
    queue<string> tracks = requests[requester_id];

    while (!tracks.empty()) {
        while(specified_buffer_size <= buffer.size() || buffer.count(requester_id)) {
            thread_wait(lock, available_buffer);
        }

        buffer.insert({requester_id, tracks.front()});

        cout << "requester " << requester_id << " track " << tracks.front() << endl;

        tracks.pop();

        if (tracks.empty()) {
            --number_of_requesters;
            specified_buffer_size = min(max_disk_queue, number_of_requesters);
        }

        if (specified_buffer_size > buffer.size())
            thread_broadcast(lock, available_buffer);
        else
            thread_broadcast(lock, full_buffer);
    }

    thread_unlock(lock);
}

void processRequest(void* ptr) {
    thread_lock(lock);

    while (specified_buffer_size > 0 || !buffer.empty()) {
        while(specified_buffer_size > buffer.size()) {
            thread_wait(lock, full_buffer);
        }

        long requester_id;
        long track;
        long min_distance = INT_MAX;
        for (const pair<long, string>& block : buffer) {
            long cur_distance = abs(current_position - stoi(block.second));
            if (cur_distance < min_distance) {
                min_distance = cur_distance;
                requester_id = block.first;
                track = stoi(block.second);
            }
        }

        current_position = track;

        cout << "service requester " << requester_id << " track " << track << endl;

        buffer.erase(requester_id);

        if (specified_buffer_size > buffer.size())
            thread_broadcast(lock, available_buffer);
    }

    thread_unlock(lock);
}

void startDiskScheduler(void* ptr) {
    thread_create(processRequest, nullptr);

    for (long i = 0; i < requests.size(); ++i) {
        long index = i;
        thread_create(sendRequest, (void*)index);
    }
}

int main(int argc, char* argv[]) {
    init(argc, argv);
    thread_libinit(startDiskScheduler, nullptr);
    return 0;
}
