#include <client_shell_ver.h>
#include <serialization/Recompilation.h>

/*translate --help or other explicit flags to their not explicit letter -h
 */
std::string explicitCommandTranslate(const std::string &s) {
    if (s == "help")return "h";
    DEBUG << "The explicit command "<<s<<" was unknown to interpreter!";
    return s;
}

/*
 * find all flags of one kind and delete them since every flag can only be used once
 * function works for one character or template overload for an entire string using lambda construction
 */
template<template<class> class H, typename text>
void flag_delete(H<std::string> &flags, text t) {
    while (std::any_of(flags.begin(), flags.end(),
                       [t](const std::string &c) { return c.find(t) != std::string::npos; })) {
        std::string tmp;
        tmp += t;
        auto itr = std::find(flags.begin(), flags.end(), tmp);
        if (itr != flags.end())[[likely]] {
            flags.erase(itr);
        }
    }
}

/*
 * on error message separate wrong commands with comma
 */
template<template<class> class H>
std::string enlist(H<std::string> &st) {
    std::string enlist;
    unsigned short count = 0;
    for (const auto &item: st) {
        if (count < st.size() - 1)[[likely]] {
            enlist += item + ",";
        } else[[unlikely]] {
            enlist += item;
        }
        count++;
    }
    return enlist;
}

/*
 * help command control
 */
void help_commander(std::vector<std::string> &flags, const std::list<std::string> &input_commands, const std::string &VERSION) {
    //provide help if needed, the flag is disconnected from other files
    if (std::any_of(flags.begin(), flags.end(),
                    [](const std::string &c) { return c.find('h') != std::string::npos; }))[[likely]] {
        auto itr = std::find(flags.begin(), flags.end(), "h");
        if (itr != flags.end())[[likely]] {
            flags.erase(itr);
        }
        std::string write_header = "[PATH1: DATA_SOURCE]... [PATH2 (optional): RECOMPILATION_FILE_TARGET] - Integrate a file or filesystem tree recursively into UltiHash. An empty PATH2 will create a recompilation file in place of or beside the chosen root directory.\n";
        std::string read_header = "[PATH1: RECOMPILATION_FILE_SOURCE]... [PATH2 (optional): TARGET_PUT_FILE_TREE] - Read a recompilation file and write/unzip the file tree either besides the recompilation file or by choice write it into a defined location.\n";
        std::string cd_header = "[PATH1: RECOMPILATION_FILE_SOURCE/SYSTEM_FILE_SOURCE] [PATH2 (optional): COMPARE_TARGET]... - Shows the path of home directory first. It can navigate to any location and also within recompilation files. Virtually it treats a recompilation file as another directory. If PATH2 is set, UltiHash will try to compare file tree changes between the two paths.\n";
        std::string ls_header = "[PATH1: RECOMPILATION_FILE_SOURCE/SYSTEM_FILE_SOURCE]... - Shows the direct content file trees of all selected target paths. Can be extended with the \"-r\" flag to show all contents recursively.\n";

        if (input_commands.empty())[[unlikely]] {
            std::string helpStr = "Help page:\nUltiHash Version: " + VERSION + "\n\n"
                                                                               "Usage:\n\n"
                                                                               "UltiHash [MAIN_COMMANDS/OPTIONS]... [PATH1] [optional: [PATH2]] ...[MAIN_COMMANDS/OPTIONS]\n"
                                                                               "The main commands will be all executed once in the filtered and fixed order: write, read, cd, ls.\n"
                                                                               "The options are pattern matched from biggest to smallest pattern, cutting out a match on each iteration of checking an option flag. \n\n"
                                                                               "Main Commands:\n\n"
                                                                               "write " + write_header +
                                  "read " + read_header +
                                  "cd " + cd_header +
                                  "ls " + ls_header +
                                  "\n\nOptions:\n\n"
                                  "[MAIN_COMMANDS]... -h ...[MAIN_COMMANDS], --help : Help page for general or main command specific use.\n"
                                  "[MAIN_COMMANDS]... -k ...[MAIN_COMMANDS], --keep : The data will be kept at source. On write the source file tree will not be deleted. On read old versions of files will be marked with \".old\" in place.\n"
                                  "[MAIN_COMMANDS]... -s ...[MAIN_COMMANDS], --synchronize : The data will be synchronized so that the newest version of a file remains (time based decision). "
                                  "On write a perhaps existing recompilation file will be read and only new files and folders will be replaced. If it does not exist the option is ignored. "
                                  "On read possibly already existing files and folders with their names will only be updated if the content of the recompilation file is newer than the destination.\n"
                                  "[MAIN_COMMANDS]... -a ...[MAIN_COMMANDS], --analyze : Analyzes two files or file trees, one a system file tree, the other a recompilation file file tree, and only replaces one into the other, if the content has changed (structure and content based decision). "
                                  "On write this means that for a perhaps existing recompilation file the entire sub file tree will be prompted on change. "
                                  "The prompt will contain the decision if the file or file tree should be recursively replaced if there are any changes of new or deleted files. "
                                  "Furthermore changed files at the same spot will be prompted if the content has changed in any way if they should be replaced. "
                                  "With the sub-option -af the checking will only be for files and with -ad checking will only be done for added and deleted files within the file tree without checking file content each. "
                                  "The standard option is equal to -afd if -a is selected.\n"
                                  "[MAIN_COMMANDS]... -r ...[MAIN_COMMANDS], --recursive : Only applicable together with \"ls\" to show file contents recursively.\n";
            std::cout << helpStr;
        } else[[likely]] {
            std::list<std::string> input_commands_help;
            input_commands_help.assign(input_commands.begin(),input_commands.end());
            if (std::any_of(input_commands_help.begin(), input_commands_help.end(),
                            [](auto c) { return c == "write"; }))[[likely]] {
                auto itr1 = std::find(input_commands_help.begin(), input_commands_help.end(), "write");
                if (itr1 != input_commands_help.end())[[likely]] {
                    input_commands_help.erase(itr1);
                }
                std::string helpStr = "Help page \"write\":\n"
                                      + write_header +
                                      "Data will be transferred and on success will be deleted as a standard operation from the source file tree. This is happening as soon as a file or directory was stored back to the database and linked to the recompilation file.\n"
                                      "The Recompilation file holds the references to the analysed.\n"
                                      "The target path can be any filesystem where you have the required permission to write to. Make sure that directories always end with / or \\ and files do not have an ending, except for their extensions!\n\n";
                std::cout << helpStr;
            }
            if (std::any_of(input_commands_help.begin(), input_commands_help.end(),
                            [](auto c) { return c == "read"; }))[[likely]] {
                auto itr1 = std::find(input_commands_help.begin(), input_commands_help.end(), "read");
                if (itr1 != input_commands_help.end())[[likely]] {
                    input_commands_help.erase(itr1);
                }
                std::string helpStr = "Help page \"read\":\n"
                                      + read_header +
                                      "Data will be read from the references of the Recompilation file and is set back together from the database.\n"
                                      "On each successful file recreation UltiHash will force replace or write back files from the recompilation file source to the chosen destination.\n\n";
                std::cout << helpStr;
            }
            if (std::any_of(input_commands_help.begin(), input_commands_help.end(),
                            [](auto c) { return c == "cd"; }))[[likely]] {
                auto itr1 = std::find(input_commands_help.begin(), input_commands_help.end(), "cd");
                if (itr1 != input_commands_help.end())[[likely]] {
                    input_commands_help.erase(itr1);
                }
                std::string helpStr = "Help page \"cd\":\n"
                                      + cd_header +
                                      "UltiHash will keep track of a system path by its own. As it enters a recompilation file (.uh ending) it will navigate on it like within a virtual hard drive\n\n";
                std::cout << helpStr;
            }
            if (std::any_of(input_commands_help.begin(), input_commands_help.end(),
                            [](auto c) { return c == "ls"; }))[[likely]] {
                auto itr1 = std::find(input_commands_help.begin(), input_commands_help.end(), "ls");
                if (itr1 != input_commands_help.end())[[likely]] {
                    input_commands_help.erase(itr1);
                }
                std::string helpStr = "Help page \"ls\":\n"
                                      + ls_header +
                                      "On recursive mode ls can even list file trees and merge them with the sub file trees of recompilation files.\n\n";
                std::cout << helpStr;
            }
            auto pathIt = input_commands_help.begin();
            while (pathIt != input_commands_help.end()) {
                if (std::filesystem::exists(*pathIt))[[likely]] {
                    auto itr1 = std::find(input_commands_help.begin(), input_commands_help.end(), *pathIt);
                    if (itr1 != input_commands_help.end())[[likely]] {
                        input_commands_help.erase(itr1);
                    }
                }
            }
            std::string command_not_applied_help = enlist(input_commands_help);
            std::cout << "The following commands were no main command we could give a manual to and not even path's that could be detected, please debug your request:\n"
                      << command_not_applied_help << std::endl;
        }
    }
    //delete duplicate help flags
    flag_delete(flags, 'h');
}

/*
 * finds available flags in order and deletes them from the flag list that has been found
 * flags are set to RAM on delete
 */
std::map<std::string, bool> flag_checker(std::vector<std::string> &flags) {
    std::map<std::string, bool> flag_check;
    flag_check.emplace("k", std::any_of(flags.begin(), flags.end(),
                                        [](const std::string &c) { return c.find('k') != std::string::npos; }));
    if(flag_check.at("k")){
        flag_delete(flags, 'k');
    }
    flag_check.emplace("s", std::any_of(flags.begin(), flags.end(),
                                        [](const std::string &c) { return c.find('s') != std::string::npos; }));
    if(flag_check.at("s")){
        flag_delete(flags, 's');
    }
    flag_check.emplace("r", std::any_of(flags.begin(), flags.end(),
                                        [](const std::string &c) { return c.find('r') != std::string::npos; }));
    if(flag_check.at("r")){
        flag_delete(flags, 'r');
    }
    if (std::any_of(flags.begin(), flags.end(),[](const std::string &c) {
        return c.find("afd") != std::string::npos; }))[[unlikely]] {
        flag_check.emplace("a", true);
        flag_check.emplace("f", true);
        flag_check.emplace("d", true);
        flag_delete(flags, "afd");
    }
    if (flag_check.size() <= 3 and std::any_of(flags.begin(), flags.end(), [](const std::string &c) {
        return c.find("af") != std::string::npos;
    }))[[unlikely]] {
        flag_check.emplace("a", true);
        flag_check.emplace("f", true);
        flag_check.emplace("d", false);
        flag_delete(flags, "af");
    }
    if (flag_check.size() <= 3 and std::any_of(flags.begin(), flags.end(), [](const std::string &c) {
        return c.find("ad") != std::string::npos;
    }))[[unlikely]] {
        flag_check.emplace("a", true);
        flag_check.emplace("f", false);
        flag_check.emplace("d", true);
        flag_delete(flags, "ad");
    }
    if (flag_check.size() <= 3 and std::any_of(flags.begin(), flags.end(), [](const std::string &c) {
        return c.find('a') != std::string::npos;
    }))[[unlikely]] {
        flag_check.emplace("a", true);
        flag_check.emplace("f", false);
        flag_check.emplace("d", false);
        flag_delete(flags, "a");
    }
    if (flag_check.size() <= 3)[[unlikely]] {
        flag_check.emplace("a", false);
        flag_check.emplace("f", false);
        flag_check.emplace("d", false);
    }
    return flag_check;
}

/*
 * detect available commands
 */
std::map<std::string, bool> command_checker(std::list<std::string> &input_commands) {
    std::map<std::string, bool> commands_todo;
    //detect commands once that have been written down
    commands_todo.emplace("write", std::any_of(input_commands.begin(), input_commands.end(), [](const std::string &c) {
        return c.find("write") != std::string::npos;
    }));
    if (commands_todo.at("write")) {
        flag_delete(input_commands, "write");
    }
    commands_todo.emplace("read", std::any_of(input_commands.begin(), input_commands.end(), [](const std::string &c) {
        return c.find("read") != std::string::npos;
    }));
    if (commands_todo.at("read")) {
        flag_delete(input_commands, "read");
    }
    commands_todo.emplace("ls", std::any_of(input_commands.begin(), input_commands.end(),
                                            [](const std::string &c) { return c.find("ls") != std::string::npos; }));
    if (commands_todo.at("ls")) {
        flag_delete(input_commands, "ls");
    }
    commands_todo.emplace("cd", std::any_of(input_commands.begin(), input_commands.end(),
                                            [](const std::string &c) { return c.find("cd") != std::string::npos; }));
    if (commands_todo.at("cd")) {
        flag_delete(input_commands, "cd");
    }
    return commands_todo;
}


//TODO: reconfigure logging from external module source
/*
 * TODO: everything should move out of main as far as possible to a file called main_base_2_Integration_Project.h which can be called
 *  from the test main and the release main of both projects, _test and normal;
 *  in the next step all test projects take the release projects as submodules
 */
int main(int argc, char *argv[]) {
    /*
     * input: write /path1 /path3 /path2/ -<flag> --<flag_string>
     *
     * flags: --<flag_string> on read are converted to -<flag> like --help is translated in main.cpp to -h on the line
     * then after filter and conversion -h is called
     * multiple flags: -c -a -m are transformed to -cam and are equal, so every flag is one character
     *
     * this means that path1 and path3 and path2 are written to a recompilation file
     * named like path2 which converts to path2.uh
     * if path2.xx is a file everything is going to path2.uh
     * if we have the situation write /path1 /path3 /path2.uh then only path1 and path3 are going to recompilation file path2.uh
     *
     * read /path1.uh /path2.uh /path3
     * this means that recompilation file path1.uh and path2.uh are written and processed to /path3
     *
     * multiple different commands ls write /path1 /path3 /path2/ -<flag> --<flag_string> will write first and enlist file content later
     * example "ls read -<flag> /path1.uh /path2.uh" same as "read /path1.uh -<flag> /path2.uh ls" and same as "-<flag> /path1.uh read /path2.uh ls"
     *
     * help section for example write -h calls write specific help and read -h calls read specific help and so on
     *
     * cd and ls input are adapters to recompilation file structure:
     * ls/cd /path1/path2.uh/Videos one is actually checking /path1/path2.uh on the operating system and is checking
     * the recompilation file if /Videos can be enlisted from inside
     * WARNING: after cd ./path2.uh we should be able to call ls ./Videos right away
     */
    //read basic input pipe info
    std::string exeRun(argv[0]);
    auto root_work_path = new std::filesystem::path{exeRun};
    root_work_path->assign(root_work_path->parent_path());
    //logging system
    init_logging(std::filesystem::path{*root_work_path}.append("./UltiHash.log").string());
    //first log that program started
    INFO << "UltiHash started running..." << std::endl;
    const std::string VERSION = std::to_string(CLIENT_SHELL_VERSION_MAJOR) + "." + std::to_string(CLIENT_SHELL_VERSION_MINOR);
    std::list<std::string> input_commands;
    std::vector<std::string> flags;
    for (unsigned int i = 1; i < static_cast<unsigned int>(argc); i++) {
        input_commands.emplace_back(argv[i]);
    }
    //read and filter flags
    auto begCom = input_commands.begin();
    while (begCom != input_commands.end()) {
        if (begCom->starts_with("--") or begCom->starts_with("-"))[[unlikely]] {
            if (begCom->starts_with("-"))[[likely]] {
                *begCom = begCom->substr(1, begCom->size());
                flags.push_back((*begCom));
            } else[[unlikely]] {
                *begCom = begCom->substr(2, begCom->size());
                flags.push_back(explicitCommandTranslate(*begCom));
            }
            auto toDel = begCom;
            begCom++;
            input_commands.erase(toDel);
        } else[[likely]] {
            begCom++;
        }
    }

    help_commander(flags, input_commands, VERSION);

    std::map<std::string, bool> flag_check = flag_checker(flags);

    std::map<std::string, bool> commands_todo = command_checker(input_commands);

    decltype(input_commands) filteredExistingPaths;
    std::ranges::copy_if(input_commands.begin(),input_commands.end(),std::back_inserter(filteredExistingPaths),
                         [input_commands](auto in){
                             return in!=input_commands.back() and !std::filesystem::exists(in);
                         });

    std::string command_not_applied = enlist(filteredExistingPaths);
    std::string flag_not_applied = enlist(flags);
    if (not command_not_applied.empty())[[unlikely]] {
        std::cout << "The following commands and paths could not be detected and applied:\n" << command_not_applied << "\n";
    }
    if (not flag_not_applied.empty())[[unlikely]] {
        std::cout << "The following flags could not be detected and applied:\n" << command_not_applied << "\n";
    }
    if (not command_not_applied.empty() or not flag_not_applied.empty())[[unlikely]] {
        std::cout << "Due to falsified request no action was performed.\n";
    } else[[likely]] {
        //perform actions
        if (commands_todo.empty())[[unlikely]] {
            std::cout << "Nothing to do! No main commands selected!\n";
        } else[[likely]] {
            if(input_commands.empty())[[unlikely]]{
                std::cout << "Nothing to do! No paths for the main commands selected!\n";
            }
            else[[likely]]{
                std::string db_config_file_path{};
                if(commands_todo.at("write") or commands_todo.at("read")){
                    std::string decideOption;
                    if(commands_todo.at("read")){
                        decideOption+="read";
                    }
                    if(commands_todo.at("write")){
                        if(commands_todo.at("read")){
                            decideOption+=" and ";
                        }
                        decideOption+="write";
                    }
                    //TODO: support multiple databases
                    std::string db_request="Since you want to "+decideOption+" you need to specify the postgresql databases you want to use for the operation in the following form:\n"
                                                                             "1. Target database server ip adress.\n"
                                                                             "2. Target database server port.\n"
                                                                             "3. Target database server username.\n"
                                                                             "4. Target database server user password.\n"
                                                                             "5. Target database server internal database name to write on.\n"
                                                                             "6. Target database server internal database scheme to write on.\n"
                                                                             "7. Target database server internal database table to write on.\n"
                                                                             "If not existent in write mode, the schemes and tables will be created on the chosen internal database. Add steps 1 to 7 recursively under the first database to connect more databases.\n"
                                                                             "\nNow enter the path to the database configuration file:\n";
                    //print help text for database setup
                    std::printf("%s", db_request.c_str());

                    std::string read_db_config_str {};
                    //user's custom config file
                    std::getline(std::cin,read_db_config_str);

                    if(not (std::filesystem::exists(read_db_config_str))){
                        std::printf("%s", "The suggested file does not exist! Aborting!\n");
                        return EXIT_FAILURE;
                    }
                    if(not (std::filesystem::is_regular_file(read_db_config_str))){
                        std::printf("%s", "The suggested path is no file that we can read! Aborting!\n");
                        return EXIT_FAILURE;
                    }
                    db_config_file_path = read_db_config_str;
                }
                if (commands_todo.at("write")) {
                    //write first
                    std::cout << "writing...\n";
                    Recompilation(db_config_file_path,true,input_commands,flag_check);
                }
                if (commands_todo.at("read")) {
                    //read second
                    std::cout << "reading...\n";
                    Recompilation(db_config_file_path,false,input_commands,flag_check);
                }
                if (commands_todo.at("cd")) {
                    //cd third
                    std::cout << "cd...\n";
                    std::cout << "Function is not implemented yet!" << std::endl;
                    /*
                     * Recompilation r=Recompilation(db_config_file_path,true,input_commands,flag_check);
                    r.cd();
                     */
                }
                if (commands_todo.at("ls")) {
                    //ls fourth
                    std::cout << "ls...\n";
                    std::cout << "Function is not implemented yet!" << std::endl;
                    /*
                     * Recompilation r=Recompilation(db_config_file_path,true,input_commands,flag_check);
                    r.ls();
                     */
                }
            }
        }
    }
    return EXIT_SUCCESS;
}
