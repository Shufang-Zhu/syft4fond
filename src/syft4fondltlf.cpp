#include<sys/stat.h>
#include<cstring>
#include<iostream>
#include<istream>
#include<memory>
// #include<CLI/CLI.hpp>
#include<filesystem>
#include"VarMgr.h"
#include"LTLfFONDSynthesizer.h"
using namespace std;

double sumVec(const std::vector<double>& v) 
{
    double sum = 0;
    for (const auto& d: v) sum += d;
    return sum;
}

struct Config {
    std::string domain_file;
    std::string problem_file;
    std::string goal_file;
    std::string out_file;
    bool interactive = false;
};

void print_usage(const std::string& prog_name) {
    std::cout << "Usage: " << prog_name
              << " -d <domain-file> -p <problem-file> -g <goal-file> [-o <out-file>] [-i]\n\n"
              << "Required options:\n"
              << "  -d, --domain-file   Path to PDDL domain file\n"
              << "  -p, --problem-file  Path to PDDL problem file\n"
              << "  -g, --goal-file     Path to LTLf goal file\n\n"
              << "Optional options:\n"
              << "  -o, --out-file      Path to output .csv file\n"
              << "  -i, --interactive   Run in interactive mode\n";
}

bool file_exists(const std::string& path) {
    return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
}

bool parse_arguments(int argc, char* argv[], Config& config) {
    std::vector<std::string> args(argv + 1, argv + argc);

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];

        if ((arg == "-d" || arg == "--domain-file") && i + 1 < args.size()) {
            config.domain_file = args[++i];
        } else if ((arg == "-p" || arg == "--problem-file") && i + 1 < args.size()) {
            config.problem_file = args[++i];
        } else if ((arg == "-g" || arg == "--goal-file") && i + 1 < args.size()) {
            config.goal_file = args[++i];
        } else if ((arg == "-o" || arg == "--out-file") && i + 1 < args.size()) {
            config.out_file = args[++i];
        } else if (arg == "-i" || arg == "--interactive") {
            config.interactive = true;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            return false;
        }
    }

    // Check required arguments
    if (config.domain_file.empty() || config.problem_file.empty() || config.goal_file.empty()) {
        std::cerr << "Error: missing required arguments.\n";
        return false;
    }

    // Check file existence
    if (!file_exists(config.domain_file)) {
        std::cerr << "Domain file does not exist: " << config.domain_file << "\n";
        return false;
    }
    if (!file_exists(config.problem_file)) {
        std::cerr << "Problem file does not exist: " << config.problem_file << "\n";
        return false;
    }
    if (!file_exists(config.goal_file)) {
        std::cerr << "Goal file does not exist: " << config.goal_file << "\n";
        return false;
    }

    return true;
}

int main(int argc, char** argv) {

    // CLI::App app {
    //     "syft4fond-ltlf: a tool for LTLf reactive synthesis in FOND planning domains"
    // };
    //
    // string domain_file, problem_file, goal_file, out_file;
    // bool interactive = false;
    //
    // CLI::Option* domain_file_opt =
    //     app.add_option("-d,--domain-file", domain_file, "Path to PDDL domain file") ->
    //     required() -> check(CLI::ExistingFile);
    //
    // CLI::Option* problem_file_opt =
    //     app.add_option("-p,--problem-file", problem_file, "Path to PDDL problem file") ->
    //     required() -> check(CLI::ExistingFile);
    //
    // CLI::Option* goal_file_opt =
    //     app.add_option("-g,--goal-file", goal_file, "Path to LTLf goal file") ->
    //     required() -> check(CLI::ExistingFile);
    //
    // // CLI::Option* interactive_opt =
    //     // app.add_option("-i,--interactive", interactive, "Executes the synthesized strategy in interactive mode");
    //
    // CLI::Option* out_file_opt =
    //     app.add_option("-o,--out-file", out_file, "Path to output .csv file. Stores:\n1. PDDL domain file\n2. PDDL problem file\n3. Run time (secs)\n4. PDDL parsing (secs)\n5. PDDL2DFA (secs)\n6. Synthesis (secs)\n7. Realizability (0,1)");
    //
    // CLI11_PARSE(app, argc, argv);

    Config config;

    if (!parse_arguments(argc, argv, config)) {
        print_usage(argv[0]);
        return 1;
    }

    std::shared_ptr<Syft::VarMgr> var_mgr = std::make_shared<Syft::VarMgr>();

    Syft::LTLfFONDSynthesizer synthesizer(
        var_mgr,
        config.domain_file,
        config.problem_file,
        config.goal_file);

    Syft::SynthesisResult result = synthesizer.run();

    auto running_times = synthesizer.get_running_times();
    auto run_time = sumVec(running_times);

    if (result.realizability) std::cout << "[syft4fond] Synthesis is REALIZABLE [" << run_time << " s]" << std::endl;
    else std::cout << "[syft4fond] Synthesis is UNREALIZABLE [" << run_time << " s]" << std::endl;

    if (config.out_file != "") {
        if (!(std::filesystem::exists(config.out_file))) {
            std::ofstream outstream(config.out_file);
            outstream << "PDDL domain,PDDL problem,LTLf goal,PDDL2DFA (s),LTLf2DFA (s),Synthesis (s),Runtime (s)"<<std::endl;
            outstream << config.domain_file << "," << config.problem_file << "," << config.goal_file << "," << running_times[0] << "," << running_times[1] << "," << running_times[2] << "," << sumVec(running_times) << std::endl;
        } else {
            std::ofstream outstream(config.out_file, std::ofstream::app);
            outstream << config.domain_file << "," << config.problem_file << "," << config.goal_file << "," << running_times[0] << "," << running_times[1] << "," << running_times[2] << "," << sumVec(running_times) << std::endl;
        }
    }
    
    return 0;
}