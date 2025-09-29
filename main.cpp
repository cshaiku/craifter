#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <memory>
#include <algorithm>
#include <cstdlib>

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r\f\v");
    return s.substr(start, end - start + 1);
}

namespace fs = std::filesystem;

enum class TaskStatus { pending, in_progress, completed };
enum class Priority { low, medium, high };

class TodoItem {
public:
    std::string id;
    std::string task;
    TaskStatus status;
    Priority priority;

    TodoItem(const std::string& i, const std::string& t, Priority p = Priority::medium)
        : id(i), task(t), status(TaskStatus::pending), priority(p) {}

    void display() const {
        std::cout << "[" << id << "] " << task << " (";
        switch (status) {
            case TaskStatus::pending: std::cout << "pending"; break;
            case TaskStatus::in_progress: std::cout << "in progress"; break;
            case TaskStatus::completed: std::cout << "completed"; break;
        }
        std::cout << ", ";
        switch (priority) {
            case Priority::low: std::cout << "low"; break;
            case Priority::medium: std::cout << "medium"; break;
            case Priority::high: std::cout << "high"; break;
        }
        std::cout << ")" << std::endl;
    }
};

class TodoList {
private:
    std::vector<std::unique_ptr<TodoItem>> items;

public:
    void add(const std::string& id, const std::string& task, Priority p = Priority::medium) {
        items.emplace_back(std::make_unique<TodoItem>(id, task, p));
    }

    void updateStatus(const std::string& id, TaskStatus status) {
        auto it = std::find_if(items.begin(), items.end(), [&](const auto& item) { return item->id == id; });
        if (it != items.end()) {
            (*it)->status = status;
        }
    }

    void display() const {
        for (const auto& item : items) {
            item->display();
        }
    }
};

class Session {
private:
    std::string name;
    fs::path base_path;
    fs::path commands_path;
    fs::path data_path;
    fs::path results_path;
    fs::path notes_path;

public:
    Session(const std::string& n, const fs::path& base = "/root/craifter/sessions")
        : name(n), base_path(base / name) {
        commands_path = base_path / "commands";
        data_path = base_path / "data";
        results_path = base_path / "results";
        notes_path = base_path / "notes";
    }

    void createFolders() {
        fs::create_directories(commands_path);
        fs::create_directories(data_path);
        fs::create_directories(results_path);
        fs::create_directories(notes_path);
    }

    void saveCommand(const std::string& cmd) {
        std::ofstream file(commands_path / (name + "_command.txt"), std::ios::app);
        file << cmd << std::endl;
    }

    void saveNote(const std::string& note) {
        std::ofstream file(notes_path / (name + "_note.txt"), std::ios::app);
        file << note << std::endl;
    }

    void saveData(const std::string& data) {
        std::ofstream file(data_path / (name + "_data.txt"), std::ios::app);
        file << data << std::endl;
    }

    void saveResult(const std::string& result) {
        std::ofstream file(results_path / (name + "_result.txt"), std::ios::app);
        file << result << std::endl;
    }

    void playback() const {
        // Playback: display and execute contents
        std::cout << "Playback for session: " << name << std::endl;
        auto processFile = [](const fs::path& path, bool execute = false) {
            if (fs::exists(path)) {
                std::ifstream file(path);
                std::string line;
                while (std::getline(file, line)) {
                    std::cout << line << std::endl;
                    if (execute) {
                        std::string cmd = trim(line);
                        if (!cmd.empty() && cmd.front() == '"' && cmd.back() == '"') {
                            cmd = cmd.substr(1, cmd.size() - 2);
                        }
                        std::cout << "Executing: " << cmd << std::endl;
                        system(cmd.c_str());
                    }
                }
            }
        };

        std::cout << "Commands:" << std::endl;
        processFile(commands_path / (name + "_command.txt"), true);
        std::cout << "Notes:" << std::endl;
        processFile(notes_path / (name + "_note.txt"), false);
        // Similarly for data and results
    }

    std::string getName() const { return name; }
};

class AIHelper {
private:
    TodoList todo_list;
    std::vector<std::unique_ptr<Session>> sessions;
    fs::path sessions_base = "/root/craifter/sessions";
    fs::path sessions_file = sessions_base / "sessions.txt";

public:
    AIHelper() {
        fs::create_directories(sessions_base);
        loadSessions();
    }

    ~AIHelper() {
        saveSessions();
    }

    void loadSessions() {
        if (fs::exists(sessions_file)) {
            std::ifstream file(sessions_file);
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty() && fs::exists(sessions_base / line)) {
                    sessions.emplace_back(std::make_unique<Session>(line));
                }
            }
        }
    }

    void saveSessions() {
        std::ofstream file(sessions_file);
        for (const auto& sess : sessions) {
            file << sess->getName() << std::endl;
        }
    }

    void run() {
        std::string command;
        while (true) {
            std::cout << "Craifter> ";
            std::getline(std::cin, command);
            if (command == "exit") break;
            processCommand(command);
        }
    }

    void executeCommand(const std::string& cmd) {
        processCommand(cmd);
    }

private:
    void processCommand(const std::string& cmd) {
        if (cmd == "help") {
            displayHelp();
        } else if (cmd.find("addtodo") == 0) {
            // Example: addtodo id task priority
            // Simple parsing
            size_t space1 = cmd.find(' ', 8);
            size_t space2 = cmd.find(' ', space1 + 1);
            if (space1 != std::string::npos && space2 != std::string::npos) {
                std::string id = cmd.substr(8, space1 - 8);
                std::string task = cmd.substr(space1 + 1, space2 - space1 - 1);
                std::string prio_str = cmd.substr(space2 + 1);
                Priority prio = Priority::medium;
                if (prio_str == "high") prio = Priority::high;
                else if (prio_str == "low") prio = Priority::low;
                todo_list.add(id, task, prio);
                std::cout << "Added todo: " << id << std::endl;
            }
        } else if (cmd.find("updatetodo ") == 0) {
            // updatetodo id status
            size_t space = cmd.find(' ', 12);
            if (space != std::string::npos) {
                std::string id = cmd.substr(12, space - 12);
                std::string status_str = cmd.substr(space + 1);
                TaskStatus status = TaskStatus::pending;
                if (status_str == "in_progress") status = TaskStatus::in_progress;
                else if (status_str == "completed") status = TaskStatus::completed;
                todo_list.updateStatus(id, status);
                std::cout << "Updated todo: " << id << std::endl;
            }
        } else if (cmd == "showtodos") {
            todo_list.display();
        } else if (cmd == "listsessions") {
            std::cout << "Sessions:" << std::endl;
            for (const auto& sess : sessions) {
                std::cout << "  " << sess->getName() << std::endl;
            }
        } else if (cmd.find("newsession ") == 0) {
            // newsession name
            std::string name = cmd.substr(11);
            sessions.emplace_back(std::make_unique<Session>(name, sessions_base));
            sessions.back()->createFolders();
            saveSessions();
            std::cout << "Created session: " << name << std::endl;
        } else if (cmd.find("savecommand ") == 0) {
            // savecommand session cmd
            size_t space = cmd.find(' ', 12);
            if (space != std::string::npos) {
                std::string sess_name = cmd.substr(12, space - 12);
                std::string command = cmd.substr(space + 1);
                auto it = std::find_if(sessions.begin(), sessions.end(), [&](const auto& s) { return s->getName() == sess_name; });
                if (it != sessions.end()) {
                    (*it)->saveCommand(command);
                }
            }
        } else if (cmd.find("savenote ") == 0) {
            // Similar for note, data, result
            size_t space = cmd.find(' ', 10);
            if (space != std::string::npos) {
                std::string sess_name = cmd.substr(10, space - 10);
                std::string note = cmd.substr(space + 1);
                auto it = std::find_if(sessions.begin(), sessions.end(), [&](const auto& s) { return s->getName() == sess_name; });
                if (it != sessions.end()) {
                    (*it)->saveNote(note);
                }
            }
        } else if (cmd.find("playback ") == 0) {
            // playback session
            std::string name = cmd.substr(9);
            auto it = std::find_if(sessions.begin(), sessions.end(), [&](const auto& s) { return s->getName() == name; });
            if (it != sessions.end()) {
                (*it)->playback();
            }
        } else if (cmd.find("runproject ") == 0) {
            // runproject session (alias for playback)
            std::string name = cmd.substr(11);
            auto it = std::find_if(sessions.begin(), sessions.end(), [&](const auto& s) { return s->getName() == name; });
            if (it != sessions.end()) {
                (*it)->playback();
            } else {
                std::cout << "Project not found." << std::endl;
            }
        } else {
            std::cout << "Unknown command. Type 'help' for commands." << std::endl;
        }
    }

    void displayHelp() {
        std::cout << "Craifter - AI-Powered Session and Task Management Tool" << std::endl;
        std::cout << "Purpose: Manage tasks, sessions, and commands with AI-assisted organization, persistence, and execution." << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  addtodo <id> <task> [priority]  - Add a new todo item. Priority: low/medium/high (default: medium)." << std::endl;
        std::cout << "                                  Purpose: Track individual tasks. Example: craifter addtodo fix_bug 'Fix login issue' high" << std::endl;
        std::cout << "  updatetodo <id> <status>        - Update a todo's status. Status: pending/in_progress/completed." << std::endl;
        std::cout << "                                  Purpose: Mark task progress. Example: craifter updatetodo fix_bug completed" << std::endl;
        std::cout << "  showtodos                      - Display all current todos." << std::endl;
        std::cout << "                                  Purpose: Review pending/in-progress tasks. Example: craifter showtodos" << std::endl;
        std::cout << "  newsession <name>              - Create a new session (project) for organizing commands and notes." << std::endl;
        std::cout << "                                  Purpose: Start a new workflow container. Example: craifter newsession web_deployment" << std::endl;
        std::cout << "  savecommand <session> <cmd>    - Save a command to a session for later execution." << std::endl;
        std::cout << "                                  Purpose: Store repeatable actions. Example: craifter savecommand web_deployment 'git push origin main'" << std::endl;
        std::cout << "  savenote <session> <note>      - Save a note or description to a session." << std::endl;
        std::cout << "                                  Purpose: Add context or documentation. Example: craifter savenote web_deployment 'Deploy to production server'" << std::endl;
        std::cout << "  playback <session>             - Display saved commands and notes for a session." << std::endl;
        std::cout << "                                  Purpose: Review session contents. Example: craifter playback web_deployment" << std::endl;
        std::cout << "  listsessions                   - List all available sessions." << std::endl;
        std::cout << "                                  Purpose: See active projects. Example: craifter listsessions" << std::endl;
        std::cout << "  runproject <session>           - Execute saved commands for a session (with playback)." << std::endl;
        std::cout << "                                  Purpose: Automate session tasks. Example: craifter runproject web_deployment" << std::endl;
        std::cout << "  exit                           - Exit the interactive mode." << std::endl;
        std::cout << "                                  Purpose: Close the tool. Example: craifter exit" << std::endl;
        std::cout << "Usage: craifter <command> or run interactively." << std::endl;
    }
};



int main(int argc, char* argv[]) {
    AIHelper helper;
    if (argc > 1) {
        // Command-line mode: concatenate args into a single command
        std::string command;
        for (int i = 1; i < argc; ++i) {
            if (i > 1) command += " ";
            command += argv[i];
        }
        helper.executeCommand(command);
    } else {
        // Interactive mode
        helper.run();
    }
    return 0;
}
