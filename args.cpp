#include "args.h"

#include <format>
#include <iostream>
#include <unordered_map>
#include <string>

template<typename C, typename T>
bool contains(C const& container, T const& item) {
    auto b = std::begin(container);
    auto e = std::end(container);
    auto found = std::find(b, e, item) != e;
    return found;
}

void debug_print(std::ostream& ostr, Args const& args) {
    ostr << "Args {\n";
    ostr << std::format("    extract = {}", args.extract) << '\n';
    ostr << std::format("    hide = {}", args.hide) << '\n';
    ostr << std::format("    message_file = {}", args.message_file) << '\n';
    ostr << std::format("    cover_file = {}", args.cover_file) << '\n';
    ostr << std::format("    stego_file = {}", args.stego_file) << '\n';
    ostr << std::format("    output_file = {}", args.output_file) << '\n';
    ostr << std::format("    error = {}", args.error) << '\n';
    ostr << "}\n";
}

Args parse_args(int argc, char** argv) {
    Args args = {};

    std::unordered_map<std::string, std::string> args_map;
    char const* boolean_arg_list[] = {"--hide", "--extract"};
    char const* args_with_args[] = {"-m", "-c", "-s", "-o"};

    for (size_t i = 1; i < argc; i++) {
        if (contains(boolean_arg_list, argv[i])) {
            args_map[argv[i]] = "";
        } else if (contains(args_with_args, argv[i])) {
            if (args_map.contains(argv[i])) {
                args.error = std::format("Duplicate '{}' argument", argv[i]);
                return args;
            }

            i++;
            if (i == argc) {
                args.error = std::format("Unexpected end of argument list after '{}'", argv[i-1]);
                return args;
            }

            args_map[argv[i-1]] = argv[i];
        } else {
            args.error = std::format("Unexpected argument '{}'", argv[i]);
            return args;
        }
    }

    args.hide = args_map.contains("--hide");
    args.extract = args_map.contains("--extract");
    args.message_file = args_map["-m"];
    args.cover_file = args_map["-c"];
    args.stego_file = args_map["-s"];
    args.output_file = args_map["-o"];

    if (!args.hide && !args.extract) {
        args.error = "No mode selected";
        return args;
    }

    if (args.hide && args.extract) {
        args.error = "Mode --hide not compatible with mode --extract";
        return args;
    }

    if (args.hide) {
        if (args.message_file.empty()) {
            args.error = "Missing '-m' argument";
            return args;
        }

        if (args.cover_file.empty()) {
            args.error = "Missing '-c'";
            return args;
        }

        if (!args.stego_file.empty()) {
            args.error = "Unexpected argument '-s' in hide mode";
            return args;
        }
    }

    if (args.extract) {
        if (args.stego_file.empty()) {
            args.error = "Missing '-s' argument";
            return args;
        }

        if (!args.message_file.empty()) {
            args.error = "Unexpected argument '-m' in extract mode";
            return args;
        }

        if (!args.cover_file.empty()) {
            args.error = "Unexpected argument '-c' in extract mode";
        }
    }

    return args;
}

