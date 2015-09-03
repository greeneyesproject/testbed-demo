/*
 * Queue.h
 *
 *  Created on: 18/feb/2015
 *      Author: luca
 */

#ifndef SRC_NETWORK_QUEUE_H_
#define SRC_NETWORK_QUEUE_H_

#include <boost/thread.hpp>
#include <boost/chrono/chrono.hpp>
#include <Messages/Message.h>

#include <queue>

using namespace std;

template<class Type>
class Queue {

private:

	static const uchar _DEBUG = 0;

	std::queue<Type*> the_queue;
	unsigned short _queueSize;
	mutable boost::mutex the_mutex;
	boost::condition_variable the_condition_variable;
	bool _active;

	boost::chrono::milliseconds _waitPeriod;

public:

	Queue(unsigned int waitPeriod = 1000, unsigned short queueSize = 0) :
			_waitPeriod(waitPeriod) {
		_active = true;
		_queueSize = queueSize;
	}

	~Queue() {
		if (_DEBUG > 0)
			cout << "Queue::~Queue" << endl;
		if (_DEBUG > 1)
			cout << "Queue::~Queue: pre lock" << endl;
		//boost::mutex::scoped_lock lock(the_mutex);
		_active = false;
		if (_DEBUG > 1)
			cout << "Queue::~Queue: post lock" << endl;
		while (!the_queue.empty()) {
			Type* item = the_queue.front();
			the_queue.pop();
			delete item;
		}

		if (_DEBUG > 1)
			cout << "Queue::~Queue: end" << endl;
	}

	void setActive(bool active) {
		boost::mutex::scoped_lock lock(the_mutex);
		_active = active;

	}

	bool push(Type* const & data) {
		if (_DEBUG > 0)
			cout << "Queue::push" << endl;
		boost::mutex::scoped_lock lock(the_mutex);
		bool const was_empty = the_queue.empty();
		bool inserted = false;
		if (!_queueSize || (_queueSize && _queueSize > the_queue.size())) {
			the_queue.push(data);
			inserted = true;
		}

		lock.unlock(); // unlock the mutex

		if (was_empty && inserted) {
			the_condition_variable.notify_one();
		}
		return inserted;
	}

	void wait_and_pop(Type*& popped_value) {
		boost::mutex::scoped_lock lock(the_mutex);
		try {
			while (_active && the_queue.empty()) {
				the_condition_variable.wait_for(lock, _waitPeriod);
				if (_DEBUG > 2)
					cout << "Queue::wait_and_pop: unlocked " << endl;
			}
			if (_active) {
				popped_value = the_queue.front();
				the_queue.pop();
			}
		} catch (exception &e) {
			if (_DEBUG > 0)
				cout << "Queue::wait_and_pop: " << e.what() << endl;
		}
	}

	void wait() {
		boost::mutex::scoped_lock lock(the_mutex);
		while (_active && the_queue.empty()) {
			the_condition_variable.wait_for(lock, _waitPeriod);
			if (_DEBUG > 2)
				cout << "Queue::wait: unlocked " << endl;
		}
	}

	void front(Type*& popped_value) {
		boost::mutex::scoped_lock lock(the_mutex);
		popped_value = the_queue.front();
	}

	void wait_and_front(Type*& popped_value) {
		boost::mutex::scoped_lock lock(the_mutex);
		while (_active && the_queue.empty()) {
			the_condition_variable.wait_for(lock, _waitPeriod);
			if (_DEBUG > 2)
				cout << "Queue::wait_and_front: unlocked " << endl;
		}
		popped_value = the_queue.front();
	}

	void pop() {
		boost::mutex::scoped_lock lock(the_mutex);
		if (!the_queue.empty()) //To prevent issues if queue flushed between first() and pop()
			the_queue.pop();
	}

	void flush() {
		boost::mutex::scoped_lock lock(the_mutex);
		while (!the_queue.empty()) {
			the_queue.pop();
		}
	}

	void notify(){
		the_condition_variable.notify_all();
	}

};

#endif /* SRC_NETWORK_QUEUE_H_ */
