#include <iostream>
#include <vector>
#include <fstream>
#include <queue>
#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "thread.h"

using namespace std;

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

class DiskScheduler {
public:
    DiskScheduler(int max_disk_queue, int number_of_requesters, vector<queue<string>> requests) {
        this->current_position = 0;
        this->max_disk_queue = max_disk_queue;
        this->number_of_requesters = number_of_requesters;
        this->current_buffer_size = min(max_disk_queue, number_of_requesters);
        this->requests = requests;
    }

    void startDiskScheduler() {
        // start requesters' threads
        vector<thread> threads;
        for (int i = 0; i < requests.size(); ++i)
            threads.push_back(thread(&DiskScheduler::sendRequest, this, i, requests[i]));

        // start processor's thread
        threads.push_back(thread(&DiskScheduler::process, this));

        // wait
        for (int i = 0; i < threads.size(); ++i)
            threads[i].join();
    }

    void sendRequest(int requester_id, queue<string> tracks) {
        while (!tracks.empty()) {
            unique_lock<mutex> lock(m);

            function<bool()> func = [&]() {
                if (current_buffer_size == buffer.size())
                    return false;

                for (const auto& block : buffer) {
                    if (block.first == requester_id)
                        return false;
                }

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

    void process() {
        while (current_buffer_size != 0 || !buffer.empty()) {
            unique_lock<mutex> lock(m);

            function<bool()> func = [this]() {
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

            cout << "service requester " << requester_id << " track " << track << endl;

            buffer.erase(requester_id);

            c_v.notify_all();
        }
    }

private:
    int current_position;
    int max_disk_queue;
    int number_of_requesters;
    int current_buffer_size;

    mutex m;
    condition_variable c_v;

    map<int, string> buffer;

    vector<queue<string>> requests;
};

int main(int argc, char* argv[]) {
    // --------------------

//    if (argc < 3) {
//        cout << "Please enter correct arguments." << endl;
//        return 0;
//    }
//
//    int max_disk_queue = stoi(argv[1]);
//
//    // get paths
//    queue<string> paths;
//    for (int i = 2; i < argc; ++i)
//        paths.push(argv[i]);

    // --------------------

    int max_disk_queue = 3;

    queue<string> paths;
    paths.push("../TestCases/disk.in0");
    paths.push("../TestCases/disk.in1");
    paths.push("../TestCases/disk.in2");
    paths.push("../TestCases/disk.in3");
    paths.push("../TestCases/disk.in4");

    // --------------------

    int number_of_requesters = paths.size();

    vector<queue<string>> requests = getRequests(paths);

    DiskScheduler diskScheduler(max_disk_queue, number_of_requesters, requests);
    diskScheduler.startDiskScheduler();

    return 0;
}
