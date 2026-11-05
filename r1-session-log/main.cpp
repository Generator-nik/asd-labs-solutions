#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>

using namespace std;


class MyStack {
private:
    string* data;
    int capacity;
    int top_index;

public:
    MyStack() {
        capacity = 16;
        top_index = -1;
        data = new string[capacity];
    }

    MyStack(const MyStack& other) {
        capacity = other.capacity;
        top_index = other.top_index;
        data = new string[capacity];
        for (int i = 0; i <= top_index; ++i) data[i] = other.data[i];
    }

    MyStack& operator=(const MyStack& other) {
        if (this != &other) {
            delete[] data;
            capacity = other.capacity;
            top_index = other.top_index;
            data = new string[capacity];
            for (int i = 0; i <= top_index; ++i) data[i] = other.data[i];
        }
        return *this;
    }

    ~MyStack() {
        delete[] data;
    }

    void push(const string& val) {
        if (top_index + 1 >= capacity) {
            capacity *= 2;
            string* new_data = new string[capacity];
            for (int i = 0; i <= top_index; ++i) new_data[i] = data[i];
            delete[] data;
            data = new_data;
        }
        data[++top_index] = val;
    }

    void pop() {
        if (top_index >= 0) top_index--;
    }

    string top() const {
        return data[top_index];
    }

    bool empty() const {
        return top_index == -1;
    }

    int size() const {
        return top_index + 1;
    }
};

struct Session {
    int line_num;
    string name;
};

int main(int argc, char* argv[]) {
    if (argc < 3) return 1;

    ifstream input(argv[1]);
    ofstream output(argv[2]);

    if (!input.is_open() || !output.is_open()) return 1;

    
    unordered_map<string, MyStack> user_stacks;
    
    vector<Session> open_sessions; 
    int sessions_count = 0;        
    
    
    vector<string> scan_errors; 
    vector<string> eof_errors;

    string line;
    int line_number = 0;

    
    int current_active = 0;
    int max_active = 0;
    int peak_line = 0;

    
    bool in_transaction = false;
    int begin_line_num = 0;
    unordered_map<string, MyStack> tx_snapshot_stacks;
    vector<Session> tx_snapshot_sessions;
    int tx_snapshot_active = 0;

    while (getline(input, line)) {
        line_number++;

        istringstream iss(line);
        string command, name;
        
        if (!(iss >> command)) continue; 

        
        if (command == "BEGIN") {
            if (in_transaction) {
                scan_errors.push_back("ERROR " + to_string(line_number) + " nested_begin");
            } else {
                in_transaction = true;
                begin_line_num = line_number;
                
                tx_snapshot_stacks = user_stacks;
                tx_snapshot_sessions = open_sessions;
                tx_snapshot_active = current_active;
            }
            continue;
        }
        else if (command == "COMMIT") {
            if (!in_transaction) {
                scan_errors.push_back("ERROR " + to_string(line_number) + " no_transaction");
            } else {
                in_transaction = false;
            }
            continue;
        }
        else if (command == "ROLLBACK") {
            if (!in_transaction) {
                scan_errors.push_back("ERROR " + to_string(line_number) + " no_transaction");
            } else {
                in_transaction = false;
                
                user_stacks = tx_snapshot_stacks;
                open_sessions = tx_snapshot_sessions;
                current_active = tx_snapshot_active;
            }
            continue;
        }

        
        if (command == "LOGIN") {
            if (!(iss >> name)) {
                scan_errors.push_back("ERROR " + to_string(line_number) + " bad_event");
                continue;
            }
            
            string extra;
            if (iss >> extra) {
                scan_errors.push_back("ERROR " + to_string(line_number) + " bad_event");
                continue;
            }

            bool valid_name = true;
            for (char c : name) {
                if (!isalnum(c)) { valid_name = false; break; }
            }
            if (!valid_name) {
                scan_errors.push_back("ERROR " + to_string(line_number) + " bad_event");
                continue;
            }

            user_stacks[name].push(name);
            open_sessions.push_back({line_number, name});
            
            current_active++;
            if (current_active > max_active) {
                max_active = current_active;
                peak_line = line_number;
            }

        } else if (command == "LOGOUT") {
            if (!(iss >> name)) {
                scan_errors.push_back("ERROR " + to_string(line_number) + " bad_event");
                continue;
            }
            
            string extra;
            if (iss >> extra) {
                scan_errors.push_back("ERROR " + to_string(line_number) + " bad_event");
                continue;
            }

            bool valid_name = true;
            for (char c : name) {
                if (!isalnum(c)) { valid_name = false; break; }
            }
            if (!valid_name) {
                scan_errors.push_back("ERROR " + to_string(line_number) + " bad_event");
                continue;
            }

            if (user_stacks.find(name) == user_stacks.end() || user_stacks[name].empty()) {
                scan_errors.push_back("ERROR " + to_string(line_number) + " logout_without_login");
            } else {
                user_stacks[name].pop();
                sessions_count++; 
                
                for (int i = open_sessions.size() - 1; i >= 0; --i) {
                    if (open_sessions[i].name == name) {
                        open_sessions.erase(open_sessions.begin() + i);
                        break;
                    }
                }
                current_active--;
            }
        } else {
            scan_errors.push_back("ERROR " + to_string(line_number) + " bad_event");
        }
    }

    
    if (in_transaction) {
        
        eof_errors.push_back("ERROR " + to_string(begin_line_num) + " transaction_not_closed");
    }

    
    sort(open_sessions.begin(), open_sessions.end(), [](const Session& a, const Session& b) {
        return a.line_num < b.line_num;
    });
    for (const auto& s : open_sessions) {
        eof_errors.push_back("ERROR " + to_string(s.line_num) + " session_not_closed");
    }

    
    for (const string& err : scan_errors) {
        output << err << "\n";
    }
    
    for (const string& err : eof_errors) {
        output << err << "\n";
    }
    
    output << "SESSIONS " << sessions_count << "\n";

    
    cerr << "# stats: peak_concurrent=" << max_active << " at line=" << peak_line << "\n";

    return 0;
}