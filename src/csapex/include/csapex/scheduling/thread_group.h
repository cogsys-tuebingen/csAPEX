#ifndef THREAD_GROUP_H
#define THREAD_GROUP_H

/// PROJECT
#include <csapex/scheduling/scheduler.h>

/// SYSTEM
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <deque>

namespace csapex
{

class ThreadGroup : public Scheduler
{
public:
    enum {
        UNDEFINED_THREAD = -1,
        PRIVATE_THREAD = 0,
        DEFAULT_GROUP_ID = 1,
        MINIMUM_THREAD_ID = 2
    };
public:
    static int nextId();

public:
    ThreadGroup(int id, std::string name);
    ThreadGroup(std::string name);
    ~ThreadGroup();

    int id() const;
    std::string name() const;
    const std::thread &thread() const;

    virtual bool isEmpty() const override;

    virtual void setPause(bool pause) override;
    virtual void stop() override;

    virtual void add(TaskGenerator *generator) override;
    virtual void add(TaskGenerator *generator, const std::vector<TaskPtr>& initial_tasks) override;

    virtual std::vector<TaskPtr> remove(TaskGenerator* generator) override;

    virtual void schedule(TaskPtr schedulable) override;

private:
    void startThread();

private:
    static int next_id_;

    int id_;
    std::string name_;

    std::thread thread_;

    std::vector<TaskGenerator*> generators_;
    std::condition_variable_any work_available_;
    std::condition_variable_any pause_changed_;

    std::recursive_mutex tasks_mtx_;
    std::deque<TaskPtr> tasks_;

    std::recursive_mutex state_mtx_;
    std::atomic<bool> running_;
    std::atomic<bool> pause_;

};

}

#endif // THREAD_GROUP_H

