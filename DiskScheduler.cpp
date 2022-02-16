//
// Created by LYL on 2022/2/15.
//

#include "DiskScheduler.h"

#include <iostream>
#include <functional>

DiskScheduler::DiskScheduler(int max_disk_queue, int number_of_requesters) {
    this->current_position = 0;
    this->max_disk_queue = max_disk_queue;
    this->number_of_requesters = number_of_requesters;
    this->current_buffer_size = min(max_disk_queue, number_of_requesters);
}

void DiskScheduler::sendRequest(int requester_id, queue<string> tracks) {
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

void DiskScheduler::process() {
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
