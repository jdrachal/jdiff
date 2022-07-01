#include <iostream>
#include "rhash.hpp"
#include "diff.hpp"
#include <map>
#include "cxxopts.hpp"
#include "file_reader.hpp"

static bool overwritePrompt(const std::string &file_path) {
    std::string input;
    std::cout << "Do you want to overwrite " << file_path << "?\n[Y]es\t[N]o" << std::endl;
    std::cin >> input;

    if ((input == "Y") || (input == "y") || (input == "Yes")) {
        return true;
    }
    return false;
}


int main(int argc, char* argv[]) {

    bool force = false;
    bool sha = false;
    std::string output_path;
    std::string file_path;
    uint16_t block_size = 0;

    cxxopts::Options options(argv[0], "Application for diffing files - cli options:");
    options
        .custom_help("target <arg>")
        .positional_help("{required args} [options]")
        .show_positional_help();


    options
        .set_tab_expansion()
        .add_options("target")
            ("s,signature", "Create signature file", cxxopts::value<std::string>(), "<base_file_path> {-o <out>} [-x | -f]")
            ("d,delta", "Create delta file", cxxopts::value<std::string>(), "<signature_path> {-i <new> | -o <out>} [-x | -f]")
            ("p,patch", "Patch file",cxxopts::value<std::string>(), "<base_file_path> {-i <delta> | -o <out>} [-x | -f]")
            ("h,help", "Print help");

    options
        .set_tab_expansion()
        .add_options("arg")
            ("i,input", "Additional input file", cxxopts::value<std::string>(), "<file_path>")
            ("o,output", "Output file", cxxopts::value<std::string>(), "<output_path>");

    options
        .set_tab_expansion()
        .add_options("more")
            ("x,sha", "Enable sha hashing")
            ("f,force", "Force output overwrite")
            ("b,block-size", "Block size to hash (not recommended!)", cxxopts::value<uint16_t>(),
                    "<decimal>");

    options.parse_positional({"input", "output"});
    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
        std::cerr << options.help({"target", "arg", "more"}) << std::endl;
        exit(0);
    }


    if (result.count("sha")){
        sha = true;
    }

    if (result.count("force")){
        force = true;
    }

    if (result.count("block-size")){
        block_size = result["block-size"].as<uint16_t>();
    }

    if (result.count("output")){
        output_path = result["output"].as<std::string>();
        if(!force && io::FileReader::doesFileExist(output_path) && !overwritePrompt(output_path)){
            exit(0);
        }
    } else {
        goto FinishHelp;
    }

    if (result.count("input")){
        file_path = result["input"].as<std::string>();
    }

    try {
        if (result.count("patch")) {
            diff::Diff d;
            if (file_path.empty()) {
                goto FinishHelp;
            }
            std::string base_file_path = result["patch"].as<std::string>();
            d.getDeltaFromFile(file_path);
            io::FileReader reader(base_file_path, d.delta().block_size);
            io::FileWriter writer(output_path);
            diff::Diff::patchFile(d.delta(), reader, writer, sha);
        } else if (result.count("delta")) {
            diff::Diff d;
            if (file_path.empty()) {
                goto FinishHelp;
            }
            std::string signature_file = result["delta"].as<std::string>();
            d.getSignatureFromFile(signature_file);
            io::FileReader reader(file_path, d.signature().block_size);
            d.prepareDelta(d.signature(), reader, sha);
            d.generateDeltaFile(output_path);

        } else if (result.count("signature")) {
            std::string base_file_path = result["signature"].as<std::string>();
            diff::Diff d;
            io::FileReader reader(base_file_path, block_size);
            d.prepareSignatures(reader, sha);
            d.generateSignatureFile(output_path);
        } else {
            goto FinishHelp;
        }
    } catch (std::invalid_argument &e){
        std::cerr << "Error: " << e.what() << std::endl;
    }


    return 0;

FinishHelp:
    std::cerr << options.help({"target", "arg", "more"}) << std::endl;
    exit(0);
}
