#include "args.h"
#include "utility.h"

#include <format>
#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>

void debug_print(std::ostream& ostr, Args const& args) {
    ostr << "Args { ";
    ostr << std::format("extract={} ", args.extract);
    ostr << std::format("hide={} ", args.hide);
    ostr << std::format("measure={} ", args.measure);
    ostr << std::format("message_file=\"{}\" ", args.message_file);
    ostr << std::format("cover_file=\"{}\" ", args.cover_file);
    ostr << std::format("stego_file=\"{}\" ", args.stego_file);
    ostr << std::format("output_file=\"{}\" ", args.output_file);
    ostr << "}\n";
}

float parse_threshold(std::string s) {
    float result = -1.0f;
    try {
        result = std::stof(s);
    } catch (...) {}

    if (!(result >= 0.0f) || !(result <= 0.5f)) {
        throw std::runtime_error("threshold should be a value in [0, 0.5]");
    }

    return result;
}

u8 parse_channel_max_bitplane(std::string max_str) {
    if (max_str.empty())
        return 8;
    try {
        int result = std::stoi(max_str);
        if (result < 0 || result > 8)
            throw std::runtime_error("r/g/b/amax must be integer in [0,8]");
        return (u8)result;
    } catch (...) {
        throw std::runtime_error("r/g/b/amax must be integer in [0,8]");
    }
}

Args parse_args(int argc, char** argv) {
    Args args = {};

    std::vector<std::string> arg_vec(argv, argv + argc);

    std::unordered_map<std::string, std::string> args_map;
    std::string boolean_arg_list[] = {"--hide", "--extract", "--measure", "--help"};
    std::string args_with_args[] = {
        "-m", "-c", "-s", "-o", "-t",
        "--rmax", "--gmax", "--bmax", "--amax",
    };

    for (size_t i = 1; i < argc; i++) {
        if (contains(boolean_arg_list, arg_vec[i])) {
            args_map[arg_vec[i]] = "";
        } else if (contains(args_with_args, arg_vec[i])) {
            if (args_map.contains(arg_vec[i])) {
                throw std::format("Duplicate '{}' argument", arg_vec[i]);
            }

            i++;
            if (i == argc) {
                throw std::format("Unexpected end of argument list after '{}'", arg_vec[i-1]);
            }

            args_map[arg_vec[i-1]] = arg_vec[i];
        } else {
            auto error = std::format("Unexpected argument '{}'", arg_vec[i]);
            throw error;
        }
    }

    args.help = args_map.contains("--help");
    args.hide = args_map.contains("--hide");
    args.extract = args_map.contains("--extract");
    args.measure = args_map.contains("--measure");
    args.message_file = args_map["-m"];
    args.cover_file = args_map["-c"];
    args.stego_file = args_map["-s"];
    args.output_file = args_map["-o"];

    auto threshold_str = args_map["-t"];

    auto rmax_str = args_map["--rmax"];
    auto gmax_str = args_map["--gmax"];
    auto bmax_str = args_map["--bmax"];
    auto amax_str = args_map["--amax"];

    if (!args.help && !args.hide && !args.extract && !args.measure) {
        throw "No mode selected (try --help)";
    }

    if (args.help) {
        // ignore all other arguments
        args = {};
        args.help = true;
        return args;
    }

    if (args.hide) {
        if (args.extract)
            throw "Mode --hide not compatible with mode --extract";
        if (args.measure)
            throw "Mode --hide not compatible with mode --measure";
    }

    if (args.extract) {
        if (args.measure)
            throw "Mode --extract not compatible with mode --measure";
    }

    if (args.hide) {
        if (args.message_file.empty())
            throw "Missing '-m' argument";

        if (args.cover_file.empty())
            throw "Missing '-c'";

        if (!args.stego_file.empty())
            throw "Unexpected argument '-s' in hide mode";

        if (!threshold_str.empty())
            throw "Unexpected argument '-t' in hide mode";

        args.rmax = parse_channel_max_bitplane(rmax_str);
        args.gmax = parse_channel_max_bitplane(gmax_str);
        args.bmax = parse_channel_max_bitplane(bmax_str);
        args.amax = parse_channel_max_bitplane(amax_str);
    }

    if (args.extract) {
        if (args.stego_file.empty())
            throw "Missing '-s' argument";

        if (!args.message_file.empty())
            throw "Unexpected argument '-m' in extract mode";

        if (!args.cover_file.empty())
            throw "Unexpected argument '-c' in extract mode";

        if (!threshold_str.empty())
            throw "Unexpected argument '-t' in hide mode";
    }

    if (args.measure) {
        if (args.cover_file.empty())
            throw "Missing '-c' argument";

        if (threshold_str.empty())
            throw "Missing '-t' argument";

        if (!args.message_file.empty())
            throw "Unexpected argument '-m' in measure mode";

        if (!args.stego_file.empty())
            throw "Unexpected argument '-s' in measure mode";

        if (!args.output_file.empty())
            throw "Unexpected argument '-o' in measure mode";

        args.threshold = parse_threshold(threshold_str);
    }

    return args;
}

