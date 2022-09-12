#pragma once
#include <deque>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>

template<typename T, typename Allocator = std::allocator<T>>
class deque_s {
  public:
    using QMutexLocker = QMutexLocker<QMutex>;

    void pop_front_wait(T &buf)
    {
        // std::unique_lock<std::mutex> lk(mutex);
        QMutexLocker lock(&qmutex);

        while (data.empty()) {
            // condition.wait(lk);
            qcondition.wait(lock.mutex());
        }

        buf = data.front();
        data.pop_front();
    }

    // void pop_all_wait(T &buf) { }

    bool empty()
    {
        // std::unique_lock<std::mutex> lk(mutex);
        QMutexLocker lock(&qmutex);
        bool         retval = data.empty();

        return retval;
    }

    void push_back(const T &buf)
    {
        // std::unique_lock<std::mutex> lk(mutex);
        QMutexLocker lock(&qmutex);

        data.push_back(buf);
        // condition.notify_one();
        qcondition.notify_one();
    }

    void clear()
    {
        QMutexLocker lock(&qmutex);
        data.clear();
    }

  private:
    std::deque<T, Allocator> data;
    // std::mutex               mutex;
    // std::condition_variable  condition{ mutex };

    QMutex         qmutex;
    QWaitCondition qcondition;
};