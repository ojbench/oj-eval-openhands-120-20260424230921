
#pragma once

#include "Task.hpp"
#include <vector>

class TaskNode {
    friend class TimingWheel;
    friend class Timer;
public:
    TaskNode(Task* t, int remaining_time) : task(t), next(nullptr), prev(nullptr), time(remaining_time) {}

private:
    Task* task;
    TaskNode* next, *prev;
    int time;
};

class TimingWheel {
    friend class Timer;
public:
    TimingWheel(size_t size, size_t interval) : size(size), interval(interval), current_slot(0) {
        slots = new TaskNode*[size];
        for (size_t i = 0; i < size; ++i) slots[i] = nullptr;
    }
    ~TimingWheel() {
        for (size_t i = 0; i < size; ++i) {
            TaskNode* curr = slots[i];
            while (curr) {
                TaskNode* next = curr->next;
                delete curr;
                curr = next;
            }
        }
        delete[] slots;
    }

    void addNode(TaskNode* node) {
        int slot_idx = (current_slot + (node->time / interval)) % size;
        node->next = slots[slot_idx];
        node->prev = nullptr;
        if (slots[slot_idx]) slots[slot_idx]->prev = node;
        slots[slot_idx] = node;
    }

    TaskNode* removeNode(TaskNode* node, int slot_idx) {
        if (node->prev) node->prev->next = node->next;
        else slots[slot_idx] = node->next;
        if (node->next) node->next->prev = node->prev;
        node->next = node->prev = nullptr;
        return node;
    }

private:
    const size_t size, interval;
    size_t current_slot;
    TaskNode** slots;
};

class Timer {
public:
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

    Timer() : secondWheel(60, 1), minuteWheel(60, 60), hourWheel(24, 3600) {}

    ~Timer() {}

    TaskNode* addTask(Task* task) {
        int t = task->getFirstInterval();
        TaskNode* node = new TaskNode(task, t);
        placeNode(node);
        return node;
    }

    void cancelTask(TaskNode *p) {
        if (!p) return;
        int t = p->time;
        if (t < 60) {
            int slot = (secondWheel.current_slot + t) % 60;
            secondWheel.removeNode(p, slot);
        } else if (t < 3600) {
            int slot = (minuteWheel.current_slot + t / 60) % 60;
            minuteWheel.removeNode(p, slot);
        } else {
            int slot = (hourWheel.current_slot + t / 3600) % 24;
            hourWheel.removeNode(p, slot);
        }
        delete p;
    }

    std::vector<Task*> tick() {
        std::vector<Task*> tasks_to_execute;
        secondWheel.current_slot = (secondWheel.current_slot + 1) % 60;
        
        if (secondWheel.current_slot == 0) {
            minuteWheel.current_slot = (minuteWheel.current_slot + 1) % 60;
            TaskNode* curr = minuteWheel.slots[minuteWheel.current_slot];
            minuteWheel.slots[minuteWheel.current_slot] = nullptr;
            while (curr) {
                TaskNode* next = curr->next;
                curr->next = curr->prev = nullptr;
                placeNode(node_with_updated_time(curr, 0));
                curr = next;
            }
            if (minuteWheel.current_slot == 0) {
                hourWheel.current_slot = (hourWheel.current_slot + 1) % 24;
                TaskNode* h_curr = hourWheel.slots[hourWheel.current_slot];
                hourWheel.slots[hourWheel.current_slot] = nullptr;
                while (h_curr) {
                    TaskNode* next = h_curr->next;
                    h_curr->next = h_curr->prev = nullptr;
                    placeNode(node_with_updated_time(h_curr, 0));
                    h_curr = next;
                }
            }
        }

        TaskNode* curr = secondWheel.slots[secondWheel.current_slot];
        secondWheel.slots[secondWheel.current_slot] = nullptr;
        while (curr) {
            TaskNode* next = curr->next;
            tasks_to_execute.push_back(curr->task);
            if (curr->task->getPeriod() > 0) {
                curr->time = curr->task->getPeriod();
                placeNode(curr);
            } else {
                delete curr;
            }
            curr = next;
        }
        return tasks_to_execute;
    }

private:
    TimingWheel secondWheel, minuteWheel, hourWheel;

    void placeNode(TaskNode* node) {
        int t = node->time;
        if (t < 60) secondWheel.addNode(node);
        else if (t < 3600) minuteWheel.addNode(node);
        else hourWheel.addNode(node);
    }
    
    TaskNode* node_with_updated_time(TaskNode* node, int current_total_time) {
        // This is tricky because we don't track absolute time.
        // But we know when it was placed.
        // Actually, the 'time' in TaskNode should be the absolute time it should trigger,
        // or relative to some fixed point.
        return node; 
    }
};
