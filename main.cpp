#include <chrono>
#include <iostream>
#include <mutex>
#include <semaphore>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// Barrier
class Barrier {
private:
    unsigned n;
    unsigned count;
    std::mutex mutex;
    std::counting_semaphore<> turnstile;
    std::counting_semaphore<> turnstile2;
    
public:
    // crée une barrière qui s'ouvre à n entité
    Barrier(unsigned n) : turnstile(0), turnstile2(0) {
        this->n = n;
        this->count = 0;
    }

    void phase1() {
        this->mutex.lock();
        std::cout << (std::to_string(getid()) + " enter mutex in phase1 with count="+std::to_string(this->count)) << std::endl;
        this->count += 1;
        std::cout << (std::to_string(getid()) + " after incr in phase1, count="+std::to_string(this->count)) << std::endl;
        if (this->count == this->n) {
            // release n times
            for(unsigned i=0;i<this->n;i++)this->turnstile.release();
        }
        std::cout << (std::to_string(getid()) + " exit mutex in phase1 with count="+std::to_string(this->count)) << std::endl;
        this->mutex.unlock();
        this->turnstile.acquire();
    }

    size_t getid() {
        return std::hash<std::thread::id>{}(std::this_thread::get_id());
    }

    void phase2() {
        this->mutex.lock();
        std::cout << (std::to_string(getid()) + " enter mutex in phase2 with count="+std::to_string(this->count)) << std::endl;
        this->count -= 1;
        std::cout << (std::to_string(getid()) + " after decr in phase2, count="+std::to_string(this->count)) << std::endl;
        if (this->count == 0) {
            // release n times
            for (unsigned i=0; i<this->n; i++) {
                this->turnstile2.release();
                std::cout << "release" << std::endl;
            }
        }
        std::cout << (std::to_string(getid()) + " exit mutex in phase2 with count="+std::to_string(this->count)) << std::endl;
        this->mutex.unlock();
        this->turnstile2.acquire();
    }

    void wait() {
        this->phase1();
        this->phase2();
    }
};

// River problem

std::mutex mutex;
unsigned hackers = 0;
unsigned serfs = 0;
std::counting_semaphore hackersQueue(0);
std::counting_semaphore serfsQueue(0);
Barrier barrier(4);

void board(const std::string &type) {
    std::cout << type + " is boarding" << std::endl;
}

void rowBoat() {
    // std::this_thread::sleep_for(5000ms);
    std::cout << "Boarding is complete. Rowing" << std::endl;
    // std::this_thread::sleep_for(5000ms);
}

void hacker_fun() {
    bool isCaptain = false;

    mutex.lock();

    hackers++;

    if (hackers == 4) { // if there are exactly 4 hackers
        hackersQueue.release(4);
        hackers = 0;
        isCaptain = true;
    } else if (hackers == 2 && serfs > 2) { // if there are exactly 2 hackers and at least 2 serfs (the opposed case, at least 2 hackers and exactly two serfs, is handled in the serf function)
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
    barrier.wait();

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
    } else if (serfs == 2 && hackers > 2) {
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
    barrier.wait();

    if (isCaptain) {
        rowBoat();
        mutex.unlock();
    }    
}

int main(){
    std::cout << "Start" << std::endl;
    // 4 original threads

    std::vector<std::thread> threads;
    for(unsigned i=0;i<8;i++) {
        threads.emplace_back([]() {
            barrier.wait();
        });
    }

    // fin
    for(auto &thr : threads) {
        thr.join();
    }
    
    // barrier.phase1();
    // barrier.phase1();
    // barrier.phase1();

    // 4 other threads that come after the first turnstile has been closed, the second opened, but the 4 original threads did not come through phase 2 yet
    // barrier.phase1();
    // barrier.phase1();
    // barrier.phase1();
    // barrier.phase1();

    // make the 4 original threads go through phase 2
    // barrier.phase2();
    // barrier.phase2();
    // barrier.phase2();
    // barrier.phase2();

    // now, we have 4 threads waiting in the aquire of phase1(), but the only way to release them is to have count=4, and we already have count=4, so adding another threading will only make it go higher
    // SOFTLOCK
    std::cout << "End" << std::endl;
}
