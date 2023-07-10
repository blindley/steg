#include "args.h"
#include "utility.h"

#include <format>
#include <iostream>
#include <unordered_map>
#include <map>
#include <set>
#include <string>
#include <vector>

enum ARGUMENT_TYPE {
    STRING_ARG, INTEGER_ARG, FLOAT_ARG
};

struct ArgSet {
    std::set<std::string> flags;
    std::map<std::string, ARGUMENT_TYPE> value_args;
};

struct ArgParser {
    ArgParser(ArgSet const& arg_set, int argc, char** argv) {
        std::vector<std::string> received_args(argv, argv + argc);

        for (size_t i = 1; i < received_args.size(); i++) {
            auto& arg = received_args[i];

            if (arg_is_present(arg)) {
                auto e = std::format("duplicate argument {}", arg);
                throw std::runtime_error(e);
            }

            if (arg_set.flags.contains(arg)) {
                flags.insert(arg);
                continue;
            }

            if (arg_set.value_args.contains(arg)) {
                ++i;
                if (i == received_args.size() || received_args[i][0] == '-') {
                    auto e = std::format("missing value for argument {}", received_args[i-1]);
                    throw std::runtime_error(e);
                }

                auto arg_type = arg_set.value_args.at(received_args[i-1]);
                auto& value = received_args[i];

                if (arg_type == STRING_ARG) {
                    string_args[arg] = value;
                } else if (arg_type == INTEGER_ARG) {
                    try { integer_args[arg] = std::stoi(value); }
                    catch (...) {
                        auto e = std::format("unexpected argument {}", value);
                        throw std::runtime_error(e);
                    }
                } else if (arg_type == FLOAT_ARG) {
                    try { float_args[arg] = std::stof(value); }
                    catch (...) {
                        auto e = std::format("unexpected argument {}", value);
                        throw std::runtime_error(e);
                    }
                }
                continue;
            }

            auto e = std::format("unexpected argument {}", arg);
            throw std::runtime_error(e);
        }
    }

    /// @brief Check if argument exists 
    bool arg_is_present(std::string const& arg) {
        return flags.contains(arg)
                || string_args.contains(arg)
                || integer_args.contains(arg)
                || float_args.contains(arg);
    }

    /// @brief Count how many members of a list are present in arguments
    ///
    /// Useful for checking for required argument sets or mutually exclusive arguments
    size_t count_present_args_among(std::vector<std::string> const& args) {
        size_t count = 0;
        for (auto& e : args) {
            if (arg_is_present(e))
                count++;
        }
        return count;
    }

    bool none_present(std::vector<std::string> const& args) {
        return count_present_args_among(args) == 0;
    }

    bool all_present(std::vector<std::string> const& args) {
        return count_present_args_among(args) == args.size();
    }

    bool all_or_none(std::vector<std::string> const& args) {
        auto count = count_present_args_among(args);
        return count == 0 || count == args.size();
    }

    bool exactly_one(std::vector<std::string> const& args) {
        return count_present_args_among(args) == 1;
    }

    std::set<std::string> flags;
    std::map<std::string, std::string> string_args;
    std::map<std::string, int> integer_args;
    std::map<std::string, float> float_args;
};


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

