#ifndef _STACK_H_
#define _STACK_H_

#include <algorithm>
#include <utility>
#include <new>
#include <mutex>
#include <thread>
#include <memory>
#include <condition_variable>

template <typename T>
class stack
{
public:
	stack() /* noexcept */;
	stack(stack<T> const&) /* strong */;
	stack<T>& operator=(stack<T> const&) /* strong */;
	size_t count() const /* noexcept */;
	void push(T const &) /* strong */;
	auto try_pop()->std::shared_ptr<T>;
	auto wait_and_pop()->std::shared_ptr<T>;

private:
	void swap(stack<T>&) /* noexcept */;
	T * array_;
	size_t array_size_;
	size_t count_;
	mutable std::mutex mutex_;
	std::condition_variable cond_;
};


template <typename T>
stack<T>::stack()
	: count_(0), array_size_(0), array_(nullptr) /* noexcept */
{
}


template <typename T>
void stack<T>::swap(stack<T>& obj) /* noexcept */
{
	std::lock(mutex_, obj.mutex_);

	using std::swap;

	swap(count_, obj.count_);
	swap(array_size_, obj.array_size_);
	swap(array_, obj.array_);

	mutex_.unlock();
	obj.mutex_.unlock();
}

template <typename T>
stack<T>::stack(stack<T> const& other) /* strong */
{
	std::lock_guard<std::mutex> lock(other.mutex_);

	auto narray = new T[other.array_size_];

	count_ = other.count_;
	array_size_ = other.array_size_;
	array_ = narray;

	try
	{
		std::copy(other.array_, other.array_ + count_, array_);
	}
	catch (...)
	{
		delete[] array_;
		throw "Failed to copy!\n";
	}
}


template <typename T>
stack<T>& stack<T>::operator=(stack<T> const& other) /* strong */
{
	if (this != &other)
	{
		stack<T> tmp(other);
		swap(tmp);
	}

	return *this;
}


template <typename T>
size_t stack<T>::count() const /* noexcept */
{
	std::lock_guard<std::mutex> lock(mutex_);
	return count_;
}


template <typename T>
void stack<T>::push(T const& obj)
{
	std::unique_lock<std::mutex> lock(mutex_);

	if (count_ == array_size_)
	{
		auto array_size = array_size_ == 0 ? 1 : array_size_ * 2;
		T* narray = new T[array_size];
		try
		{
			std::copy(array_, array_ + count_, narray);
		}
		catch (...)
		{
			delete[] narray;
			throw "Failed to copy!\n";
		}

		array_size_ = array_size;

		if (array_)
			delete[] array_;
		array_ = narray;
	}

	array_[count_] = obj;
	++count_;
	cond_.notify_all();
}


template <typename T>
auto stack<T>::try_pop() -> std::shared_ptr<T>
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (count_ == 0)
	{
		return nullptr;
	}

	--count_;

	return std::make_shared<T>(array_[count_]);
}


template <typename T>
auto stack<T>::wait_and_pop() -> std::shared_ptr<T>
{
	std::unique_lock<std::mutex> lock(mutex_);

	while (!count_)
	{
		cond_.wait(mutex_);
	}

	--count_;

	return std::make_shared<T>(array_[count_]);
}


#endif
