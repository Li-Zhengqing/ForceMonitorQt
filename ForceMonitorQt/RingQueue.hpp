#pragma once

#include <iostream>
#include <mutex>
#ifndef GLOG_NO_ABBREVIATED_SEVERITIES
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>
#endif
#include <QDebug>

enum QueueStatus {
	EMPTY, NORMAL, FULL
};

template<typename T>
class RingQueue {
private:
	T* data;
	unsigned int size;
	unsigned int entry;
	unsigned int exit;
	QueueStatus status;

	// Mutex Lock
	std::mutex mtx;
public:
	RingQueue();
	RingQueue(unsigned int size);
	~RingQueue();

	unsigned int length();
	long enqueue_memcpy(T* src, size_t size);
	long dequeue_memcpy(T* dst, size_t size);

	unsigned int get_length();
	long enqueue_from_array(T* src, size_t size);
	long dequeue_to_array(T* src, size_t size);

	// TODO:
	// long enqueue_from_array_force(T* src, size_t size);
};

// #include "RingQueue.h"

template<typename T>
RingQueue<T>::RingQueue(unsigned size) {
	this->size = size;
	this->data = new T[size];
	this->entry = 0;
	this->exit = 0;
	this->status = QueueStatus::EMPTY;
}

template<typename T>
RingQueue<T>::RingQueue() {
	this->size = 0;
	this->data = NULL;
	this->entry = 0;
	this->exit = 0;
	this->status = QueueStatus::FULL;
}

template<typename T>
RingQueue<T>::~RingQueue() {
	delete[] this->data;
	// std::cout << "RingQueue Destructed" << std::endl;
}

template<typename T>
unsigned int RingQueue<T>::length() {
	// std::cout << this->status << std::endl;
	if (this->status == QueueStatus::EMPTY) {
		return 0;
	}
	else if (this->status == QueueStatus::FULL) {
		return this->size;
	}
	else {
		if (this->exit < this->entry) {
			return this->entry - this->exit;
		}
		else {
			return this->entry + (this->size - this->exit);
		}
	}
}

template<typename T>
long RingQueue<T>::enqueue_memcpy(T* src, size_t size) {
	if (this->status == QueueStatus::FULL) {
		return 0;
	}

	unsigned int _size = this->size - this->length();

	if (size < _size) {
		_size = size;
	}

	unsigned int _entry = this->entry;
	T* _start = this->data + _entry;

	// std::cout << "_size = " << _size << ", _entry = " << _entry << " this->size = " << this->size << " sizeof(T) = " << sizeof(T) << std::endl;
	
	// FIXME:
	if (_entry + _size < this->size) {
		memcpy(_start, src, _size * sizeof(T));
		// return _size;
	}
	else {
		memcpy(_start, src, (this->size - _entry) * sizeof(T));
		_start = this->data;
		memcpy(_start, src + (this->size - _entry), (_size - (this->size - _entry)) * sizeof(T));
		// return _size;
	}
	
	
	if (this->length() + _size == 0) {
		this->status = QueueStatus::EMPTY;
	}
	else if (this->length() + _size == this->size) {
		this->status = QueueStatus::FULL;
	}
	else {
		this->status = QueueStatus::NORMAL;
	}
	// Update entry
	this->entry += _size;
	if (this->entry > this->size) {
		this->entry -= this->size;
	}

	return _size;
}

template<typename T>
long RingQueue<T>::dequeue_memcpy(T* dst, size_t size) {

	if (this->status == QueueStatus::EMPTY) {
		return 0;
	}

	unsigned int _size = this->length();

	if (size < _size) {
		_size = size;
	}

	unsigned int _exit = this->exit;
	T* _start = this->data + _exit;

	if (_exit + _size < this->size) {
		memcpy(dst, _start, _size * sizeof(T));
		// return _size;
	}
	else {
		memcpy(dst, _start, (this->size - _exit) * sizeof(T));
		_start = this->data;
		memcpy(dst + (this->size - _exit), _start, (_size - (this->size - _exit)) * sizeof(T));
		// return _size;
	}

	if (this->length() - _size == 0) {
		this->status = QueueStatus::EMPTY;
	}
	else if (this->length() - _size == this->size) {
		this->status = QueueStatus::FULL;
	}
	else {
		this->status = QueueStatus::NORMAL;
	}
	// Update exit
	this->exit += _size;
	if (this->exit > this->size) {
		this->exit -= this->size;
	}

	return _size;
}

template<typename T>
unsigned int RingQueue<T>::get_length() {
	// Mutex lock on entry
	this->mtx.lock();
	std::cout << "get_length; Locked" << std::endl;

	unsigned int _ret = this->length();

	// Mutex unlock on exit
	this->mtx.unlock();
	std::cout << "get_length; Unlocked" << std::endl;

	return _ret;
}

template<typename T>
long RingQueue<T>::enqueue_from_array(T* src, size_t size) {
	// OutputDebugString(L"Enqueue Operation\n");
	// Mutex lock on entry
	this->mtx.lock();
	std::cout << "enqueue; Locked" << std::endl;
	
	long _ret = this->enqueue_memcpy(src, size);

	// Mutex unlock on exit
	this->mtx.unlock();
	std::cout << "enqueue; Unlocked" << std::endl;

	return _ret;
}

template<typename T>
long RingQueue<T>::dequeue_to_array(T* dst, size_t size) {
	// OutputDebugString(L"Dequeue Operation\n");
	// Mutex lock on entry
	this->mtx.lock();
	std::cout << "dequeue; Locked" << std::endl;

	long _ret = this->dequeue_memcpy(dst, size);

	// Mutex unlock on exit
	this->mtx.unlock();
	std::cout << "dequeue; Unlocked" << std::endl;

	return _ret;
}