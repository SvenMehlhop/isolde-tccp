#include "tccp_co.hpp"
#include "save_summary.hpp"
#include <iostream>
#include <string>

bool g_debug_mode = false;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <project_name> [--debug]\n";
        return 1;
    }
    std::string project_name = argv[1];
    
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--debug" || arg == "-d") {
            g_debug_mode = true;
        }
    }

    std::cout << "Opening Project: " << project_name << "\n";

    MainConfig main_cfg = loadMainConfig("config.toml");
    
    std::string input_dir = main_cfg.input;
    if (!input_dir.empty() && input_dir[0] == '/') {
        input_dir = input_dir.substr(1);
    }
    
    std::string project_dir = input_dir + "/" + project_name;
    ProjectConfig proj_cfg = loadProjectConfig(project_dir + "/config.toml");
    
    std::vector<Contract> contracts;
    int id_counter = 1;
    for (const auto& c_file : proj_cfg.contracts) {
        if (!c_file.empty()) {
            loadContractFile(project_dir + "/" + c_file, proj_cfg, contracts, id_counter, "");
        }
    }
    for (const auto& obs : proj_cfg.observers) {
        for (const auto& src : obs.sources) {
            if (!src.contract.empty()) {
                std::string resolved_dump = "";
                if (!src.dump.empty()) {
                    resolved_dump = project_dir + "/" + src.dump;
                }
                loadContractFile(project_dir + "/" + src.contract, proj_cfg, contracts, id_counter, resolved_dump);
            }
        }
    }
    
    if (!contracts.empty()) {
        std::string output_dir = main_cfg.output;
        if (!output_dir.empty() && output_dir[0] == '/') {
            output_dir = output_dir.substr(1);
        }
        std::string project_out_dir = output_dir + "/" + proj_cfg.output;
        saveContracts(contracts, project_out_dir, "contracts.csv");
        saveSummary(contracts, proj_cfg, project_out_dir, "summary.md");
    }

    return 0;
}