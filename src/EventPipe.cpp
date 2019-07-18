#include "EventPipe.h"
#include "Log.h"

#include <unistd.h>
#include <fcntl.h>

using namespace std;

EventPipe::EventPipe() {
    event_mutex_ = make_shared<mutex>();

    if(pipe(mPipes) < 0) {
        Log(ERROR) << "Failed to create pipe, won't be able to wake threads, errno = " << errno << '\n';
    }

    // FIXME: Use prepareSocket()?
    if(fcntl(mPipes[0], F_SETFL, O_NONBLOCK) < 0) {
        Log(WARNING) << "Failed to set pipe non-blocking mode\n";
    }
}

EventPipe::~EventPipe() {
    lock_guard<mutex> lock(*event_mutex_);

    if (mPipes[0] >= 0)
        close(mPipes[0]);

    if (mPipes[1] >= 0)
        close(mPipes[1]);
}

void EventPipe::setPipe() {
    lock_guard<mutex> lock(*event_mutex_);

    if (write(mPipes[1], "0", 1) < 0)
        Log(ERROR) << "Could not write to pipe, errno = " << errno << '\n';
}

void EventPipe::resetPipe() {
    lock_guard<mutex> lock(*event_mutex_);

    unsigned char buffer;

    while(read(mPipes[0], &buffer, 1) == 1)
        ;
}

int EventPipe::getSocket() {
    lock_guard<mutex> lock(*event_mutex_);

    return mPipes[0];
}