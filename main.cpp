#include <iostream>
#include <vector>
#include <fstream>
#include <queue>
#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <limits.h>

#include "thread.h"

using namespace std;

int current_position;
int max_disk_queue;
int number_of_requesters;
int current_buffer_size;

vector<queue<string>> requests;
map<int, string> buffer;
mutex m;
condition_variable c_v;

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

void sendRequest(int requester_id, queue<string> tracks) {
    while (!tracks.empty()) {
        unique_lock<mutex> lock(m);

        function<bool()> func = [&]() {
            if (current_buffer_size == buffer.size())
                return false;

            if (buffer.count(requester_id))
                return false;

            return true;
        };

        c_v.wait(lock, func);

        buffer.insert({requester_id, tracks.front()});

        cout << "requester " << requester_id << " track " << tracks.front() << endl;

        tracks.pop();

        if (tracks.empty()) {
            --number_of_requesters;
            current_buffer_size = min(max_disk_queue, number_of_requesters);
        }

        c_v.notify_all();
    }
}

void processRequest() {
    while (current_buffer_size != 0 || !buffer.empty()) {
        unique_lock<mutex> lock(m);

        function<bool()> func = []() {
            return current_buffer_size <= buffer.size();
        };

        c_v.wait(lock, func);

        int requester_id;
        int track;
        int minDistance = INT_MAX;
        for (const auto& block : buffer) {
            int curDistance = abs(current_position - stoi(block.second));
            if (curDistance < minDistance) {
                minDistance = curDistance;
                requester_id = block.first;
                track = stoi(block.second);
            }
        }

        current_position = track;

        cout << "service requester " << requester_id << " track " << track << endl;

        buffer.erase(requester_id);

        c_v.notify_all();
    }
}

void startDiskScheduler() {
    vector<thread> threads;
    for (int i = 0; i < requests.size(); ++i)
        threads.push_back(thread(sendRequest, i, requests[i]));

    threads.push_back(thread(processRequest));

    for (int i = 0; i < threads.size(); ++i)
        threads[i].join();
}

int main(int argc, char* argv[]) {
/*    if (argc < 3) {
        cout << "Please enter correct arguments." << endl;
        return 0;
    }

    // get paths
    queue<string> paths;
    for (int i = 2; i < argc; ++i)
        paths.push(argv[i]);*/

    queue<string> paths;
    paths.push("../TestCases/disk.in0");
    paths.push("../TestCases/disk.in1");
    paths.push("../TestCases/disk.in2");
    paths.push("../TestCases/disk.in3");
    paths.push("../TestCases/disk.in4");

    current_position = 0;
    max_disk_queue = 3;
//    max_disk_queue = stoi(argv[1]);
    number_of_requesters = paths.size();
    current_buffer_size = min(max_disk_queue, number_of_requesters);
    requests = getRequests(paths);

    startDiskScheduler();

    return 0;
}
