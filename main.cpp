#include "DiskScheduler.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <queue>

using namespace std;

queue<string> readFile(const string& path) {
    ifstream file(path);
    if (!file.is_open())
        return {};

    queue<string> out;
    string name;
    while (file >> name)
        out.push(name);

    file.close();
    return out;
}

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
    paths.push("..\\TestCases\\disk.in0");
    paths.push("..\\TestCases\\disk.in1");
    paths.push("..\\TestCases\\disk.in2");
    paths.push("..\\TestCases\\disk.in3");
    paths.push("..\\TestCases\\disk.in4");

    // --------------------

    int number_of_requesters = paths.size();

    // read requests from files
    vector<queue<string>> requests;
    while (!paths.empty()) {
        requests.push_back(readFile(paths.front()));
        paths.pop();
    }

    DiskScheduler diskScheduler(max_disk_queue, number_of_requesters);

    // start requesters' threads
    vector<thread> threads;
    for (int i = 0; i < requests.size(); ++i)
        threads.push_back(thread(&DiskScheduler::sendRequest, &diskScheduler, i, requests[i]));

    // start processor's thread
    threads.push_back(thread(&DiskScheduler::process, &diskScheduler));

    // wait
    for (int i = 0; i < threads.size(); ++i)
        threads[i].join();

    return 0;
}
