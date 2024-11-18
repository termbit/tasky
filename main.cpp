#include "json/single_include/nlohmann/json.hpp"
#include "TaskyConfig.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <string_view>

using namespace std::string_view_literals;
using json = nlohmann::json;

std::string data_json_path = "";
std::string issued_ID_path = "";
std::string temp_issued_ID_path = "";

void create_config_paths() {
    const char* xdg_config_dir = getenv("XDG_CONFIG_DIR");
    const char* home_dir = getenv("HOME");
    if(xdg_config_dir) {
        data_json_path.append(xdg_config_dir);
        data_json_path.append("/tasky/data.json");
        issued_ID_path.append(xdg_config_dir);
        issued_ID_path.append("/tasky/issued_ID.txt");
        temp_issued_ID_path.append(xdg_config_dir);
        temp_issued_ID_path.append("/temp_issued_ID.txt");
    }
    else {
        data_json_path.append(home_dir);
        data_json_path.append("/.config/tasky/data.json");
        issued_ID_path.append(home_dir);
        issued_ID_path.append("/.config/tasky/issued_ID.txt");
        temp_issued_ID_path.append(home_dir);
        temp_issued_ID_path.append("/.config/tasky/temp_issued_ID.txt");
    }
}

void check_file_openable(std::string name) {
    std::fstream file(name);
    if(!file.is_open()) {
        std::cerr << "-- \"" << name << "\" file reading error\n"
                  << "Check the file or tasks for existence\n";
        exit(0);
    }
    file.close();
}

auto is_file_empty(std::string name) {
    std::fstream file(name);
    file.seekg(0, std::ios::end);
    auto file_size = file.tellg();
    file.close();
    return file_size;
}

bool check_ID_existence(const std::string_view task_id) {
    bool is_ID_exist = false;
    std::fstream issued_ID(issued_ID_path, std::ios::in);
    std::string temp_string;
    while (std::getline(issued_ID, temp_string)) {
        if(temp_string == task_id)
            is_ID_exist = true;
    }
    issued_ID.close();
    return is_ID_exist;
}

void initialize_srand() {
    srand(time(0));
}

int task_id_generate() {
    std::fstream issued_ID(issued_ID_path);
    int id = 0;
    std::string temp_string = "";

    while (true) {
        id = rand() % 1000;
        bool is_unique = true;
    
        while (std::getline(issued_ID, temp_string)) {
            if (temp_string == std::to_string(id)) {
                is_unique = false;
                break;
            }
        }

        if(is_unique) {
            issued_ID.clear();
            issued_ID << id << std::endl;
            return id;
        }
    }

    issued_ID.close();
    
    std::cerr << " - ID generation failure\n";
    exit(1);
}

void help() {
    std::cout << "© Copyright 2024 termbit"
              << "Tasky - fast and simple task tracker\n"
              << "Version " << TASKY_VERSION_MAJOR << '.' << TASKY_VERSION_MINOR << "\n\n"
              << "Usage: tasky [OPTIONS]\n\n"
              << "Options:\n"
              << "  help                           - Display this help message and exit\n"
              << "  list FILTER_TYPE               - Show all tasks\n"
              << "  add \"DESCRIPTION\"              - Add a new task in list\n"
              << "  delete ID                      - Delete a task from list\n"
              << "  delete all                     - Delete all tasks (be careful)\n"
              << "  edit ID \"NEW_DESCRIPTION\"      - Edit task description by its ID\n"
              << "  mark-done ID                   - Change task status to \"done\"\n"
              << "  mark-in-progress ID            - Change task status to \"in-progress\"\n\n"
    
    << "Tasky allows you to create, delete, edit and view tasks\n"
    << "Control is carried out using the commands above and other parameters.\n"
    << "Tasks have their own status, creation date, description (name) and ID\n"
    << "By default all tasks have status \"in-progress\"\n\n" 
    << "\"list\" command have filter types \"done\" and \"in-progress\"\n";             
}

void add_task(const char* task_description) {
    if(!task_description) {
        std::cout << "Please, enter a task name\n";
        return;
    }

    auto createdAt_time_t = time(0);
    std::string createdAt = std::ctime(&createdAt_time_t);
    createdAt.erase(createdAt.end() - 1);

    json existing_data;

    std::ifstream input(data_json_path);

    auto file_size = is_file_empty(data_json_path);

    if (file_size) {
        input >> existing_data;
        input.close();
    }

    if (!existing_data.contains("tasks")) {
        existing_data["tasks"] = json::array();
    }
    
    int current_task_id = task_id_generate();

    json new_task;
    new_task = {
        {"id", current_task_id},
        {"description", task_description},
        {"created-at", createdAt},
        {"status", "in-progress"}
    };

    existing_data["tasks"].push_back(new_task);
    
    std::ofstream output(data_json_path);
    output << existing_data.dump(4);
    output.close();

    std::cout << "Task has been created (ID: " << current_task_id << " )\n";
}

void list_tasks(const char* filter_by) {  
    json obj_to_list;
    std::fstream data_file(data_json_path);

    auto file_size = is_file_empty(data_json_path); // file closed here

    if (file_size) {
        data_file >> obj_to_list;
        data_file.close();
    }
    else {
        std::cout << "No tasks\n"
                  << "Add new tasks by command \"tasky add TASK_NAME\"\n";
        exit(0);
    }

    if (!obj_to_list.contains("tasks")) {
        obj_to_list["tasks"] = json::array();
    }

    std::cout << "+-------+------------------------------+---------------+------------------+" << std::endl;
    std::cout << "| ID    | Creating date                | Status        | Description      |" << std::endl;
    std::cout << "+-------+------------------------------+---------------+------------------+" << std::endl;

    if(filter_by == NULL) {
        for (const auto& x : obj_to_list["tasks"]) {
            std::cout << "| " << "  " << x["id"] 
                    << "\t| " << "  " << x["created-at"] 
                    << " | " << x["status"]
                    << " | " << x["description"] << std::endl;
        }
    }
    else if (filter_by == "done"sv) {
        
        for (const auto& x : obj_to_list["tasks"]) {
            if(x["status"] == "done"sv)
                std::cout << "| " << "  " << x["id"] 
                        << "\t| " << "  " << x["created-at"] 
                        << " | " << x["status"]
                        << " | " << x["description"] << std::endl;
        }        
    }
    else if (filter_by == "in-progress"sv) {
        for (const auto& x : obj_to_list["tasks"]) {
            if(x["status"] == "in-progress"sv)
                std::cout << "| " << "  " << x["id"] 
                        << "\t| " << "  " << x["created-at"] 
                        << " | " << x["status"]
                        << " | " << x["description"] << std::endl;
        }          
    }
    else {
        std::cerr << "\nUnknow type of list filtering\n"
                  << "See available types by \"tasky help\"\n";
        return;
    }
}

void delete_task(const std::string& task_id) {  

    if(task_id.empty()) {
        std::cerr << "Please, enter a task id\n";
        return;
    }

    bool is_ID_exist = check_ID_existence(task_id);

    if(!is_ID_exist) {
        std::cerr << "Task with id ( " << task_id << " ) doesn't exist\n";
        exit(0); 
    }

    std::fstream data_file(data_json_path);

    auto file_size = is_file_empty(data_json_path);

    json obj_from_delete;

    if(file_size) {
        data_file >> obj_from_delete;
        data_file.close();


        bool is_task_with_id_exist = false;
        for (auto& elem : obj_from_delete["tasks"]) {
            if(std::to_string(elem["id"].get<int>()) == task_id) {
                is_task_with_id_exist = true;
                break;
            }
        }
    
        if (!is_task_with_id_exist) {
            std::cout << "This id was issued, but task with that id doesn't exist\n"
                      << "Please delete it from \'issued_ID.txt\'\n";
            exit(0);
        }

    }
    else {
        obj_from_delete["tasks"] = json::array();


        std::cout << "This id was issued, but tasks no\n"
                  << "Please delete it from \'issued_ID.txt\'\n\n"
                  << "Add new task with command \"tasky add TASK_NAME\"\n";
        exit(0);
    }

    obj_from_delete["tasks"].erase(std::remove_if(obj_from_delete["tasks"].begin(), obj_from_delete["tasks"].end(),
            [&task_id](auto& elem) {
                return std::to_string(int(elem["id"])) == task_id;
            }
        )
    );



    std::fstream issued_ID(issued_ID_path, std::fstream::in);
    std::fstream temp_issued_ID(temp_issued_ID_path, std::fstream::out);
    std::string current_string;
    while (std::getline(issued_ID, current_string)) {
        if(current_string != task_id)
            temp_issued_ID << current_string << "\n";
    }
    issued_ID.close(); temp_issued_ID.close();

    remove(issued_ID_path.c_str());
    rename(temp_issued_ID_path.c_str(), issued_ID_path.c_str());


    std::fstream output(data_json_path, std::fstream::out | std::fstream::trunc);
    output << obj_from_delete.dump(4);
    output.close();

}

void delete_all_tasks() {
    std::fstream data_file(data_json_path, std::fstream::out | std::fstream::trunc);
    data_file.close();
    

    std::fstream issued_ID(issued_ID_path, std::fstream::out | std::fstream::trunc);
    issued_ID.close();
}

void edit(const std::string& task_id, const std::string& new_description) {

    if(task_id.empty()) {
        std::cerr << "Please, enter a task id\n";
        return;
    }

    if(new_description.empty()) {
        std::cerr << "Please, enter a new description for task\n";
        return;
    }

    std::fstream data_file(data_json_path);

    auto data_file_size = is_file_empty(data_json_path);

    json obj_to_edit_task;

    if(data_file_size) {
        data_file >> obj_to_edit_task;
        data_file.close();

        bool is_task_with_id_exist = false;
        for (auto& elem : obj_to_edit_task["tasks"]) {
            if(std::to_string(elem["id"].get<int>()) == task_id) {
                is_task_with_id_exist = true;                
            }
        }

        if (!is_task_with_id_exist) {
            std::cerr << "Task with ID: ( " << task_id << " ) doesn't exist\n";
            return;
        }        

        for (auto& elem : obj_to_edit_task["tasks"]) {
            if(std::to_string(elem["id"].get<int>()) == task_id) {
                elem["description"] = new_description;
                break;
            }
        }
    }
    else {
        std::cerr << "error file reading\n"
                  << "Check tasks for existence\n";
        return;
    }

    data_file.open(data_json_path, std::fstream::out | std::fstream::trunc);
    data_file << obj_to_edit_task.dump(4);
    data_file.close();

}

void mark_done(const std::string& task_id) {
    if(task_id.empty()) {
        std::cerr << "Please, enter a task id\n";
        return;
    }

    bool is_ID_exist = check_ID_existence(task_id);

    if(!is_ID_exist) {
        std::cerr << "Task with id ( " << task_id << " ) doesn't exist\n";
        exit(0); 
    }
    
    auto file_size = is_file_empty(data_json_path);
    json obj_to_mark_done;

    if (file_size) {
        std::fstream data_file(data_json_path, std::ios::in);
        data_file >> obj_to_mark_done;
        data_file.close();
    }            

    for (auto& elem : obj_to_mark_done["tasks"]) {
        if (std::to_string(elem["id"].get<int>()) == task_id){
            elem["status"] = "done";
            break;
        }
    }    

    std::fstream output_file(data_json_path, std::ios::out);
    output_file << obj_to_mark_done.dump(4);
    output_file.close();
}

void mark_in_progress(const std::string& task_id) {
    if(task_id.empty()) {
        std::cerr << "Please, enter a task id\n";
        return;
    }

    bool is_ID_exist = check_ID_existence(task_id);

    if(!is_ID_exist) {
        std::cerr << "Task with id ( " << task_id << " ) doesn't exist\n";
        exit(0); 
    }

    auto file_size = is_file_empty(data_json_path);
    json obj_to_mark_in_progress;


    if (file_size) {
        std::fstream data_file(data_json_path, std::ios::in);
        data_file >> obj_to_mark_in_progress;
        data_file.close();
    }

    for (auto& elem : obj_to_mark_in_progress["tasks"]) {
        if (std::to_string(elem["id"].get<int>()) == task_id){
            elem["status"] = "in-progress";
            break;
        }
    }

    std::fstream output_file(data_json_path, std::ios::out);
    output_file << obj_to_mark_in_progress.dump(4);
    output_file.close();
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::cout << "Option not selected\n" << "To see options, use \"tasky help\"\n"; 
        return 0;
    }

    create_config_paths();

    check_file_openable(data_json_path);
    check_file_openable(issued_ID_path);

    initialize_srand();


    if (argv[1] == "help"sv)                  help();
    else if (argv[1] == "mark-done"sv) {
        if (argv[2] == NULL)
            mark_done("");
        else
            mark_done(argv[2]);
    }
    else if (argv[1] == "mark-in-progress"sv) {
        if (argv[2] == NULL)
            mark_in_progress("");
        else
            mark_in_progress(argv[2]);
    }
    else if (argv[1] == "add"sv)              add_task(argv[2]);
    else if (argv[1] == "list"sv)             list_tasks(argv[2]);
    else if (argv[1] == "edit"sv) {
        if(argv[2] == NULL)
            edit("", argv[3]);
        else if (argv[3] == NULL)
            edit(argv[2], "");
        else if (argv[2] == NULL && argv[3] == NULL)
            edit("", "");
        else
            edit(argv[2], argv[3]);
    }

    else if (argv[1] == "delete"sv) {
        if (argv[2] == NULL)
            delete_task("");
        else if (argv[2] == "all"sv)
            delete_all_tasks();
        else
            delete_task(argv[2]);
    }
    else 
        std::cout << "Unknown option.\n use \"tasky help\" to see supported options\n";
}