//
// Created by tar on 3/19/23.
// Hat tip: https://github.com/ftrias/TeensyThreads/blob/master/examples/Runnable/Runnable.h
//

#ifndef NETJUCE_TEENSY_RUNNABLE_H
#define NETJUCE_TEENSY_RUNNABLE_H

/*
 * This is an abstract class that is reusable to allow for easy definition of a
 * runnable function for std::thread.
 */

class Runnable {
protected:
    virtual void runTarget(void *arg) = 0;

public:
    virtual ~Runnable() {}

    static void runThread(void *arg) {
        auto *_runnable = static_cast<Runnable *> (arg);
        _runnable->runTarget(arg);
    }
};

#endif //NETJUCE_TEENSY_RUNNABLE_H
