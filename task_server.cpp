string getAllTasksJSON() {
        stringstream ss;
        ss << "{\"success\":true,\"tasks\":[";
        for (size_t i = 0; i < allTasks.size(); i++) {
            ss << allTasks[i]->toJSON();
            if (i < allTasks.size() - 1) ss << ",";
        }
        ss << "]}";
        return ss.str();
    }
    string getPendingTasksJSON() {
        vector<Task*> pending;
        for (Task* t : allTasks) {
            if (!t->completed) pending.push_back(t);
        }
        stringstream ss;
        ss << "{\"success\":true,\"tasks\":[";
        for (size_t i = 0; i < pending.size(); i++) {
            ss << pending[i]->toJSON();
            if (i < pending.size() - 1) ss << ",";
        }
        ss << "]}";
        return ss.str();
    }
    string getTopTaskJSON() {
        if (taskHeap.empty()) {
            return "{\"success\":false,\"message\":\"No pending tasks\"}";
        }
        Task* top = taskHeap.top();
        return "{\"success\":true,\"task\":" + top->toJSON() + "}";
    }
    string getStatisticsJSON() {
        int total = allTasks.size();
        int completed = 0, pending = 0, overdue = 0;
        time_t now = time(0);
        for (Task* t : allTasks) {
            if (t->completed) {
                completed++;
            } else {
                pending++;
                if (t->deadline < now) overdue++;
            }
        }
        double completionRate = total > 0 ? (completed * 100.0) / total : 0;
        stringstream ss;
        ss << "{\"success\":true,";
        ss << "\"total\":" << total << ",";
        ss << "\"completed\":" << completed << ",";
        ss << "\"pending\":" << pending << ",";
        ss << "\"overdue\":" << overdue << ",";
        ss << "\"completionRate\":" << fixed << setprecision(1) << completionRate;
        ss << "}";
        return ss.str();
    }
    string sortByPriority() {
        vector<Task*> temp = allTasks;
        sort(temp.begin(), temp.end(), [](Task* a, Task* b) {
            return a->priority < b->priority;
        });
        stringstream ss;
        ss << "{\"success\":true,\"tasks\":[";
        for (size_t i = 0; i < temp.size(); i++) {
            ss << temp[i]->toJSON();
            if (i < temp.size() - 1) ss << ",";
        }
        ss << "]}";
        return ss.str();
    }