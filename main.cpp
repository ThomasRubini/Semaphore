#include <chrono>
#include <iostream>
#include <mutex>
#include <semaphore>
#include <thread>
#include <vector>
#include <barrier>

using namespace std::chrono_literals;

// River problem

std::mutex mutex;
unsigned hackers = 0;
unsigned serfs = 0;
std::counting_semaphore hackersQueue(0);
std::counting_semaphore serfsQueue(0);
std::barrier barrier(4);

void board(const std::string &type) {
    std::cout << type + " is boarding" << std::endl;
}

void rowBoat() {
    std::cout << "Boarding is complete. Rowing" << std::endl;
}

void hacker_fun() {
    bool isCaptain = false;

    mutex.lock();

    hackers++;
    if (hackers == 4) { // if there are exactly 4 hackers
        hackersQueue.release(4);
        hackers = 0;
        isCaptain = true;
    } else if (hackers == 2 && serfs >= 2) { // if there are exactly 2 hackers and at least 2 serfs (the opposed case, at least 2 hackers and exactly two serfs, is handled in the serf function)
        hackersQueue.release(2);
        serfsQueue.release(2);

        serfs -= 2;
        hackers = 0;
        isCaptain = true;
    } else {
        mutex.unlock(); // if a combination has not been achieved, release the mutex
    }

    hackersQueue.acquire(); // block hackers until captain sais we can board (via signal())

    if (isCaptain) {
        std::cout << "4 people were found. Boarding new boat" << std::endl;
    }

    board("hacker");
    barrier.arrive_and_wait();

    if (isCaptain) {
        rowBoat();
        mutex.unlock();
    }
}

void serf_fun() {
    bool isCaptain = false;

    mutex.lock();

    serfs++;
    if (serfs == 4) {
        serfsQueue.release(4);
        serfs = 0;
        isCaptain = true;
    } else if (serfs == 2 && hackers >= 2) {
        serfsQueue.release(2);
        hackersQueue.release(2);

        serfs = 0;
        hackers -= 2;
        isCaptain = true;
    } else {
        // if a combination has not been achieved, release the mutex
        mutex.unlock();
    }

    // block serfs until captain sais we can board (via signal())
    serfsQueue.acquire();

    if (isCaptain) {
        std::cout << "Boarding new boat" << std::endl;
    }

    board("serf");
    barrier.arrive_and_wait();

    if (isCaptain) {
        rowBoat();
        mutex.unlock();
    }
}

int main(){
    // prepare hackers and serfs
    std::vector<std::thread> hackers;
    for (unsigned i = 0; i < 10; i++) {
        hackers.emplace_back(hacker_fun);
    }

    std::vector<std::thread> serfs;
    for (unsigned i = 0; i < 10; i++) {
        serfs.emplace_back(serf_fun);
    }

    // join hackers and serfs (to ensure main exits last)
    for (auto &hacker : hackers) {
        hacker.join();
    }

    for (auto &serf : serfs) {
        serf.join();
    }
}
