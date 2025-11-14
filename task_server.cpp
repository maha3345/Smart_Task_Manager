#include <iostream>
#include <vector>
#include <unordered_map>
#include <queue>
#include <stack>
#include <string>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <limits>
#include <fstream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

struct Task {
    int id;
    string name;
    string description;
    int priority;
    time_t deadline;
    int duration;
    bool isRecurring;
    int recurringDays;
    bool completed;
    time_t createdAt;

    Task(int _id, string _name, string _desc, int _priority, time_t _deadline,
         int _duration, bool _isRecurring = false, int _recurringDays = 0) {
        id = _id;
        name = _name;
        description = _desc;
        priority = _priority;
        deadline = _deadline;
        duration = _duration;
        isRecurring = _isRecurring;
        recurringDays = _recurringDays;
        completed = false;
        createdAt = time(0);
    }
};
private:
 string escapeJSON(const string& s) const {
        string result;
        for (char c : s) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c;
            }
        }
        return result;
    }

public:
    string toJSON() const {
        stringstream ss;
        ss << "{";
        ss << "\"id\":" << id << ",";
        ss << "\"name\":\"" << escapeJSON(name) << "\",";
        ss << "\"description\":\"" << escapeJSON(description) << "\",";
        ss << "\"priority\":" << priority << ",";
        ss << "\"deadline\":" << deadline << ",";
        ss << "\"duration\":" << duration << ",";
        ss << "\"isRecurring\":" << (isRecurring ? "true" : "false") << ",";
        ss << "\"recurringDays\":" << recurringDays << ",";
        ss << "\"completed\":" << (completed ? "true" : "false") << ",";
        ss << "\"createdAt\":" << createdAt;
        ss << "}";
        return ss.str();
    }

struct ComparePriority {
    bool operator()(Task* t1, Task* t2) {
        if (t1->priority == t2->priority) {
            return t1->deadline > t2->deadline;
        }
        return t1->priority > t2->priority;
    }
};

class TaskManager {
private:
    priority_queue<Task*, vector<Task*>, ComparePriority> taskHeap;
    unordered_map<int, Task*> taskMap;
    vector<Task*> allTasks;
    stack<pair<string, Task*>> undoStack;
    stack<pair<string, Task*>> redoStack;
    queue<Task*> recurringQueue;
    vector<Task*> allocatedTasks;
    int nextId;

    void rebuildHeap() {
        priority_queue<Task*, vector<Task*>, ComparePriority> newHeap;
        for (auto task : allTasks) {
            if (!task->completed) {
                newHeap.push(task);
            }
        }
        taskHeap = move(newHeap);
    }

public:
    TaskManager() : nextId(1) {}

    ~TaskManager() {
        for (Task* t : allocatedTasks) {
            delete t;
        }
        allocatedTasks.clear();
    }
};
  string addTask(string name, string description, int priority, time_t deadline,
                   int duration, bool isRecurring = false, int recurringDays = 0) {
        Task* newTask = new Task(nextId++, name, description, priority, deadline,
                                 duration, isRecurring, recurringDays);
        allocatedTasks.push_back(newTask);
        taskHeap.push(newTask);
        taskMap[newTask->id] = newTask;
        allTasks.push_back(newTask);
        if (isRecurring) recurringQueue.push(newTask);
        undoStack.push({"ADD", newTask});
        while (!redoStack.empty()) redoStack.pop();
        return "{\"success\":true,\"message\":\"Task added successfully\",\"task\":" + newTask->toJSON() + "}";
    }

  string removeTask(int id) {
        if (taskMap.find(id) == taskMap.end()) {
            return "{\"success\":false,\"message\":\"Task not found\"}";
        }
        Task* task = taskMap[id];
        undoStack.push({"DELETE", task});
        while (!redoStack.empty()) redoStack.pop();
        taskMap.erase(id);
        for (int i = 0; i < (int)allTasks.size(); i++) {
            if (allTasks[i]->id == id) {
                allTasks.erase(allTasks.begin() + i);
                break;
            }
        }
        rebuildHeap();
        return "{\"success\":true,\"message\":\"Task deleted: " + task->name + "\"}";
    }

    string markCompleted(int id) {
        if (taskMap.find(id) == taskMap.end()) {
            return "{\"success\":false,\"message\":\"Task not found\"}";
        }
        Task* task = taskMap[id];
        task->completed = true;
        rebuildHeap();
        return "{\"success\":true,\"message\":\"Task marked as completed\"}";
    }
    string sortByDeadline() {
        vector<Task*> temp = allTasks;
        sort(temp.begin(), temp.end(), [](Task* a, Task* b) {
            return a->deadline < b->deadline;
        });
        stringstream ss;
        ss << "{\"success\":true,\"tasks\":[";
        for (size_t i = 0; i < temp.size(); i++) {
            ss << temp[i]->toJSON();
            if (i < temp.size() - 1) ss << ",";
        }
        ss << "]}";
        return ss.str();
    }string sortByDeadline() {
        vector<Task*> temp = allTasks;
        sort(temp.begin(), temp.end(), [](Task* a, Task* b) {
            return a->deadline < b->deadline;
        });
        stringstream ss;
        ss << "{\"success\":true,\"tasks\":[";
        for (size_t i = 0; i < temp.size(); i++) {
            ss << temp[i]->toJSON();
            if (i < temp.size() - 1) ss << ",";
        }
        ss << "]}";
        return ss.str();
    }string sortByDuration() {
        vector<Task*> temp = allTasks;
        sort(temp.begin(), temp.end(), [](Task* a, Task* b) {
            return a->duration < b->duration;
        });
        stringstream ss;
        ss << "{\"success\":true,\"tasks\":[";
        for (size_t i = 0; i < temp.size(); i++) {
            ss << temp[i]->toJSON();
            if (i < temp.size() - 1) ss << ",";
        }
        ss << "]}";
        return ss.str();
    }string undoOperation() {
        if (undoStack.empty()) {
            return "{\"success\":false,\"message\":\"Nothing to undo\"}";
        }
        auto operation = undoStack.top();
        undoStack.pop();
        if (operation.first == "ADD") {
            Task* t = operation.second;
            taskMap.erase(t->id);
            for (int i = 0; i < (int)allTasks.size(); i++) {
                if (allTasks[i]->id == t->id) {
                    allTasks.erase(allTasks.begin() + i);
                    break;
                }
            }
            rebuildHeap();
            redoStack.push(operation);
            return "{\"success\":true,\"message\":\"Undone: Task addition reverted\"}";
        } else if (operation.first == "DELETE") {
            Task* t = operation.second;
            taskMap[t->id] = t;
            allTasks.push_back(t);
            rebuildHeap();
            redoStack.push(operation);
            return "{\"success\":true,\"message\":\"Undone: Task restored\"}";
        }
        return "{\"success\":false,\"message\":\"Unknown operation\"}";
    }string redoOperation() {
        if (redoStack.empty()) {
            return "{\"success\":false,\"message\":\"Nothing to redo\"}";
        }
        auto operation = redoStack.top();
        redoStack.pop();
        if (operation.first == "ADD") {
            Task* t = operation.second;
            taskMap[t->id] = t;
            allTasks.push_back(t);
            rebuildHeap();
            undoStack.push(operation);
            return "{\"success\":true,\"message\":\"Redone: Task added back\"}";
        } else if (operation.first == "DELETE") {
            Task* t = operation.second;
            taskMap.erase(t->id);
            for (int i = 0; i < (int)allTasks.size(); i++) {
                if (allTasks[i]->id == t->id) {
                    allTasks.erase(allTasks.begin() + i);
                    break;
                }
            }
            rebuildHeap();
            undoStack.push(operation);
            return "{\"success\":true,\"message\":\"Redone: Task deleted again\"}";
        }
        return "{\"success\":false,\"message\":\"Unknown operation\"}";
    }string processRecurringTasks() {
        int size = (int)recurringQueue.size();
        time_t currentTime = time(0);
        int generated = 0;
        for (int i = 0; i < size; i++) {
            Task* t = recurringQueue.front();
            recurringQueue.pop();

            if (t->isRecurring) {
                if (t->completed || t->deadline < currentTime) {
                    time_t newDeadline = t->deadline;
                    if (t->recurringDays <= 0) {
                    } else {
                        while (newDeadline <= currentTime) {
                            newDeadline += (time_t)t->recurringDays * 86400;
                        }
                        addTask(t->name, t->description, t->priority, newDeadline,
                                t->duration, true, t->recurringDays);
                        generated++;
                    }
                }
            }
            recurringQueue.push(t);
        }
        if (generated > 0) {
            return "{\"success\":true,\"message\":\"Generated " + to_string(generated) + " recurring task(s)\"}";
        }
        return "{\"success\":true,\"message\":\"No recurring tasks to generate\"}";
    }
    hi