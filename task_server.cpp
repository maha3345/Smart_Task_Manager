TaskManager taskManager;

string urlDecode(const string& str) {
    string result;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%' && i + 2 < str.length()) {
            string hex = str.substr(i + 1, 2);
            char ch = (char)strtol(hex.c_str(), nullptr, 16);
            result += ch;
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

unordered_map<string, string> parseQuery(const string& query) {
    unordered_map<string, string> params;
    stringstream ss(query);
    string pair;
    while (getline(ss, pair, '&')) {
        size_t pos = pair.find('=');
        if (pos != string::npos) {
            string key = urlDecode(pair.substr(0, pos));
            string value = urlDecode(pair.substr(pos + 1));
            params[key] = value;
        }
    }
    return params;
}
string handleRequest(const string& request) {
    size_t pos = request.find("GET /api/");
    if (pos == string::npos) return "";

    size_t endPos = request.find(" HTTP", pos);
    if (endPos == string::npos) return "";

    string path = request.substr(pos + 9, endPos - pos - 9);

    size_t queryPos = path.find('?');
    string endpoint = (queryPos != string::npos) ? path.substr(0, queryPos) : path;
    string query = (queryPos != string::npos) ? path.substr(queryPos + 1) : "";

    auto params = parseQuery(query);

    auto getInt = [&](const string& key, int defaultVal)->int {
        if (params.find(key) == params.end()) return defaultVal;
        try { return stoi(params[key]); } catch(...) { return defaultVal; }
    };
    auto getLong = [&](const string& key, long defaultVal)->long {
        if (params.find(key) == params.end()) return defaultVal;
        try { return stol(params[key]); } catch(...) { return defaultVal; }
    };
    auto getBoolStr = [&](const string& key)->bool {
        if (params.find(key) == params.end()) return false;
        string v = params[key];
        return (v == "1" || v == "true" || v == "True");
    };
    auto getString = [&](const string& key, const string& defaultVal="")->string {
        if (params.find(key) == params.end()) return defaultVal;
        return params[key];
    };
    if (endpoint == "addTask") {
        string name = getString("name", "Untitled");
        string desc = getString("description", "");
        int priority = getInt("priority", 3);
        long deadline = getLong("deadline", time(0) + 3600);
        int duration = getInt("duration", 30);
        bool isRecurring = getBoolStr("isRecurring");
        int recurringDays = getInt("recurringDays", 0);
        return taskManager.addTask(name, desc, priority, (time_t)deadline, duration, isRecurring, recurringDays);
    } else if (endpoint == "removeTask") {
        int id = getInt("id", -1);
        if (id < 0) return "{\"success\":false,\"message\":\"Invalid id\"}";
        return taskManager.removeTask(id);
    } else if (endpoint == "markCompleted") {
        int id = getInt("id", -1);
        if (id < 0) return "{\"success\":false,\"message\":\"Invalid id\"}";
        return taskManager.markCompleted(id);
    } else if (endpoint == "getAllTasks") {
        return taskManager.getAllTasksJSON();
    } else if (endpoint == "getPendingTasks") {
        return taskManager.getPendingTasksJSON();
    } else if (endpoint == "getTopTask") {
        return taskManager.getTopTaskJSON();
    } else if (endpoint == "getStats") {
        return taskManager.getStatisticsJSON();
    }
    else if (endpoint == "undo") {
        return taskManager.undoOperation();
    } else if (endpoint == "redo") {
        return taskManager.redoOperation();
    } else if (endpoint == "sortByPriority") {
        return taskManager.sortByPriority();
    } else if (endpoint == "sortByDeadline") {
        return taskManager.sortByDeadline();
    } else if (endpoint == "sortByDuration") {
        return taskManager.sortByDuration();
    } else if (endpoint == "processRecurring") {
        return taskManager.processRecurringTasks();
    }

    return "{\"success\":false,\"message\":\"Unknown endpoint\"}";
}

int main() {
    WSADATA wsaData;
    int wsaErr = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (wsaErr != 0) {
        cerr << "WSAStartup failed: " << wsaErr << endl;
        return 1;
    }

    SOCKET server_fd = INVALID_SOCKET;
    SOCKET new_socket = INVALID_SOCKET;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) {
        cerr << "socket failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        cerr << "setsockopt failed: " << WSAGetLastError() << endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    memset(address.sin_zero, 0, sizeof(address.sin_zero));

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        cerr << "bind failed: " << WSAGetLastError() << endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    if (listen(server_fd, 10) == SOCKET_ERROR) {
        cerr << "listen failed: " << WSAGetLastError() << endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    cout << "Task Manager Server running on http://localhost:8080" << endl;
    cout << "Open task_manager.html in your browser to use the interface" << endl;
    while (true) {
        addrlen = sizeof(address);
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket == INVALID_SOCKET) {
            cerr << "accept failed: " << WSAGetLastError() << endl;
            break;
        }

        char buffer[30000];
        int valread = recv(new_socket, buffer, (int)sizeof(buffer) - 1, 0);
        if (valread == SOCKET_ERROR) {
            cerr << "recv failed: " << WSAGetLastError() << endl;
            closesocket(new_socket);
            continue;
        }
        buffer[valread] = '\0';

        string request(buffer);
        string response;

        if (request.find("GET /api/") != string::npos) {
            string jsonResponse = handleRequest(request);
            response = "HTTP/1.1 200 OK\r\n";
            response += "Content-Type: application/json\r\n";
            response += "Access-Control-Allow-Origin: *\r\n";
            response += "Content-Length: " + to_string(jsonResponse.length()) + "\r\n";
            response += "\r\n";
            response += jsonResponse;
        } else {
            response = "HTTP/1.1 404 Not Found\r\n\r\n";
        }

        int sendRes = send(new_socket, response.c_str(), (int)response.length(), 0);
        if (sendRes == SOCKET_ERROR) {
            cerr << "send failed: " << WSAGetLastError() << endl;
        }
        closesocket(new_socket);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}
