#include "utils.h"
#include "cxxopts.hpp"

// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
// and compile with -DPSAPI_VERSION=1

Options g_options;

void ParseOption(int argc, char** argv) {
    cxxopts::Options options("dll_monitor.exe", "dll monitor");
    options.custom_help("");
    options.add_options()
        ("process_name", "print all the loaded dlls of this running process", cxxopts::value<std::string>())
        ("dll_name", "print all the processes loading this dll", cxxopts::value<std::string>())
        ("show_running_process", "show all the currently running process", cxxopts::value<bool>())
        ("remove_duplicate_name", "remove duplicate process name loading the specific dll, using with option --dll_name", cxxopts::value<bool>())
        ("kill_all", "kill all the processes of loading a specific dll, using with option --dll_name", cxxopts::value<bool>())
        ("h,help", "Print usage")
        ;

    options.allow_unrecognised_options();
    auto result = options.parse(argc, argv);

    //unmatched arguments
    if (result.unmatched().size() > 0) {
        std::cout << "Unmatched args: ";
        for (auto& arg : result.unmatched())
            std::cout << arg << "\t";
        exit(0);
    }

    //print help strings
    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }
    else {
        //handle each argument
        if (result.count("process_name")) {
            g_options.processName = result["process_name"].as<std::string>();
            if (g_options.processName.find(".exe") == std::string::npos)
                g_options.processName += ".exe";
        }
        else if (result.count("dll_name")) {
            g_options.dllName = result["dll_name"].as<std::string>();
            if (g_options.dllName.find(".dll") == std::string::npos)
                g_options.dllName += ".dll";
            if (result.count("remove_duplicate_name")) {
                g_options.removeDuplicateName = result["remove_duplicate_name"].as<bool>();
            }
            if (result.count("kill_all")) {
                g_options.killAll = result["kill_all"].as<bool>();
            }
        }
        else if (result.count("show_running_process")) {
            g_options.showRunningProcess = result["show_running_process"].as<bool>();
        }
        else {
            std::cout << options.help() << std::endl;
            exit(0);
        }
    }
}

int PrintModules(DWORD processID) {
#if 0 //debug purpose
    std::wcout << "\n==================================================" << "Process name: " << cutils::GetProcessNameFromID(processID) << "\tProcess id: " << processID << std::endl;
#endif
    HMODULE hMods[1024];
    HANDLE hProcess;
    DWORD cbNeeded;

    // Get a handle to the process.
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |PROCESS_VM_READ, FALSE, processID);
    if (NULL == hProcess) {
        std::cout << "System process, can't get the process handle, process id: " <<processID << std::endl;
        return 1;
    }

    // Get a list of all the modules in this process.
    if( EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        for ( int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++ )
        {
            TCHAR szModName[MAX_PATH];
            // Get the full path to the module's file.
            if ( GetModuleFileNameEx( hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR)))
            {
                // Print the module name and handle value.
                fs::path modulePath = szModName;
                if (!g_options.dllName.empty()) {
                    if (g_options.removeDuplicateName) { //remove duplicated process name
						std::string processName;
						cutils::WString2String(cutils::GetProcessNameFromID(processID), processName);
						if (processName.empty())
							processName = "System process(access denied)";
						static std::set<std::string> processSet;
                        if (g_options.dllName.compare(modulePath.filename().string()) == 0 && processSet.find(processName) == processSet.end()) {
                            processSet.insert(processName);
                            //print process name
                            fmt::print("\t {} {: >50}\n", processName, processID);
                            break;
                        }
                    }
                    else {
						if (g_options.dllName.compare(modulePath.filename().string()) == 0) {
							//print process name
							std::string processName;
							cutils::WString2String(cutils::GetProcessNameFromID(processID), processName);
							if (processName.empty())
								processName = "System process(access denied)";
							fmt::print("\t {} {: >50}\n", processName, processID);
                            if (g_options.killAll) {
                                cutils::KillProcess(processID);
                            }
							break;
						}
                    }
                }
                
                if (!g_options.processName.empty()) {
                    fmt::print("\t {} {: >100}\n", modulePath.filename().string().c_str(), modulePath.string().c_str());
                }
            }
        }
    }
    else {
        std::wcout << "EnumProcessModules failed, Process name: " << cutils::GetProcessNameFromID(processID) << "\tProcess id: " << processID << std::endl;
    }
    // Release the handle to the process.
    CloseHandle( hProcess );

    return 0;
}

int main(int argc, char** argv) {
    ParseOption(argc, argv);
    DWORD aProcesses[1024]; 
    DWORD cbNeeded; 
    DWORD cProcesses;
    // Get the list of process identifiers.
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
        std::cout << "Enum processes failed" << std::endl;
        return -1;
    }
    // Calculate how many process identifiers were returned.
    cProcesses = cbNeeded / sizeof(DWORD);
    std::cout << "\nTotal running processes number: " << cProcesses << std::endl;
    
    if (!g_options.processName.empty()) {
		// Print the names of the modules for each process.
		for (int i = 0; i < cProcesses; i++)
		{
            std::string processName;
            cutils::WString2String(cutils::GetProcessNameFromID(aProcesses[i]), processName);
            if (g_options.processName.compare(processName) == 0) {
                std::cout << "\nAll loaded DLL of process: " << processName << "\tID: " << aProcesses[i] << std::endl;
                PrintModules(aProcesses[i]);
            }
		}
    }

	if (!g_options.dllName.empty()) {
		// Print the names of the modules for each process.
		std::cout << "\nAll processes loaded the dll: " << g_options.dllName << std::endl;
		for (int i = 0; i < cProcesses; i++)
		{
			PrintModules(aProcesses[i]);
		}
	}

    if (g_options.showRunningProcess) {
        // Print the names of the modules for each process.
        for (int i = 0; i < cProcesses; i++)
        {
            std::string processName;
            cutils::WString2String(cutils::GetProcessNameFromID(aProcesses[i]), processName);
            if (processName.empty())
                processName = "System process(access denied)";
            std::cout << "Process Name: " << processName << "\t\t\tProcess ID: " << aProcesses[i] << std::endl;
        }
    }

    std::cout << "Done!" << std::endl;
    return 0;
}