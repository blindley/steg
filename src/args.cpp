#include "args.h"
#include "utility.h"

#include <format>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <iostream>

std::string get_exe_short_name(char const* argv0) {
    char const* after_last_slash = argv0;
    for (size_t i = 0; argv0[i] != 0; i++) {
        if (argv0[i] == '/' || argv0[i] == '\\') {
            after_last_slash = argv0 + i + 1;
        }
    }
    return after_last_slash;
}

void print_usage(char const* exe_name) {
    auto exe_short_name = get_exe_short_name(exe_name);

    std::cout << "Usage:\n";
    std::cout << "    " << exe_short_name
        << " --hide -m <message file> -c <coverfile> -o <stego file> "
        << "[--rmax <n>] [--gmax <n>] [--bmax <n>] [--amax <n>]\n";
    std::cout << "    " << exe_short_name
        << " --hide --random <count> -c <coverfile> -o <stego file> "
        << "[--rmax <n>] [--gmax <n>] [--bmax <n>] [--amax <n>]\n";    
    std::cout << "    " << exe_short_name << " --extract -s <stego file> -o <message file>\n";
    std::cout << "    " << exe_short_name << " --measure -c <cover file> -t <threshold>\n";
    std::cout << "    " << exe_short_name << " --help\n";

    std::cout << "\n(try --help for more details)\n";
}

void print_help(char const* argv0) {
    print_usage(argv0);

    char const* help_lines[] = {
        "",
        "Modes:",
        "  --hide              Hide message in cover image",
        "  --extract           Extract hidden message",
        "  --measure           Measure hiding capacity of an image",
        "  --help              Display this help message",
        "",
        "Hide Mode Options:",
        "  -c <coverfile>      Cover image to hide message in",
        "  -m <message file>   Message file to hide",
        "  --random <count>    Fill cover file with <count> random bytes",
        "  -o <stego file>     Name of output stego image file",
        "  --rmax <n>          Max red bitplanes to use ([0,8], default=8)",
        "  --gmax <n>          Max green bitplanes to use ([0,8], default=8)",
        "  --bmax <n>          Max blue bitplanes to use ([0,8], default=8)",
        "  --amax <n>          Max alpha bitplanes to use ([0,8], default=8)",
        "",
        "Extract Mode Options:",
        "  -s <stego file>     Stego file to extract hidden message from",
        "  -o <message file>   Name of output message file",
        "",
        "Measure Mode Options:",
        "  -c <cover file>     Cover image to measure for capacity",
        "  -t <threshold>      Complexity threshold to measure for [0,0.5]",
    };

    for (auto& line : help_lines) {
        std::cout << line << '\n';
    }
}

struct RawArgs {
    std::set<std::string> flags;
    std::map<std::string, std::string> value_args;

    bool arg_is_present(std::string const& arg_name) const {
        return flags.contains(arg_name) || value_args.contains(arg_name);
    }

    std::string get_value_or_throw(std::string const& arg_name) const {
        try {
            return value_args.at(arg_name);
        } catch(...) {
            auto err = std::format("missing argument {}", arg_name);
            throw std::runtime_error(err);
        }
    }

    int get_integer_or_default_with_range(std::string const& arg_name,
        int default_, int low, int high) const
    {
        if (!value_args.contains(arg_name)) {
            return default_;
        }

        try {
            auto value = std::stoi(value_args.at(arg_name));
            if (value >= low && value <= high) {
                return value;
            }
        } catch(...) {}
        auto err = std::format("{} should be integer in range [{}, {}]", arg_name, low, high);
        throw std::runtime_error(err);
    }

    float get_float_or_default_with_range(std::string const& arg_name,
        float default_, float low, float high) const
    {
        if (!value_args.contains(arg_name)) {
            return default_;
        }

        try {
            auto value = std::stof(value_args.at(arg_name));
            if (value >= low && value <= high) {
                return value;
            }
        } catch(...) {}
        auto err = std::format("{} should be real number in range [{}, {}]", arg_name, low, high);
        throw std::runtime_error(err);
    }
};

RawArgs collect_raw_args(int argc, char** _argv,
    std::set<std::string> const& flag_names, std::set<std::string> value_arg_names)
{
    std::set<std::string> flags;
    std::map<std::string, std::string> value_args;

    std::vector<std::string> argv(_argv, _argv + argc);

    for (size_t i = 1; i < argv.size(); i++) {
        if (flags.contains(argv[i]) || value_args.contains(argv[i])) {
            auto err = std::format("duplicate argument {}", argv[i]);
            throw std::runtime_error(err);
        }

        if (flag_names.contains(argv[i])) {
            flags.insert(argv[i]);
            continue;
        }

        if (value_arg_names.contains(argv[i])) {
            ++i;
            if (i == argv.size()) {
                auto err = std::format("unexpected end of argument list. missing value for {}", argv[i-1]);
                throw std::runtime_error(err);
            }

            value_args[argv[i-1]] = argv[i];
            continue;
        }

        auto err = std::format("unexpected argument {}", argv[i]);
        throw std::runtime_error(err);
    }

    RawArgs collection;
    collection.flags = std::move(flags);
    collection.value_args = std::move(value_args);

    return collection;
}

Args parse_args(int argc, char** argv) {
    auto raw_args = collect_raw_args(argc, argv,
        {"--hide", "--extract", "--measure", "--help"},
        {"-m", "-c", "-o", "-s", "-t", "--rmax", "--gmax", "--bmax", "--amax", "--random"}
    );

    Args args = {};

    if (raw_args.arg_is_present("--help")) {
        // if help is present, we don't care about anything else
        args.help = true;
        return args;
    }

    args.hide = raw_args.arg_is_present("--hide");
    args.extract = raw_args.arg_is_present("--extract");
    args.measure = raw_args.arg_is_present("--measure");
    bool random_message = raw_args.arg_is_present("--random");

    int num_modes = (int)args.hide + (int)args.extract + (int)args.measure;
    if (num_modes == 0) {
        auto err = std::format("no mode selected (--hide, --extract or --measure)");
        throw std::runtime_error(err);
    }

    if (num_modes > 1) {
        auto err = "multiple modes selected (choose one of --hide, --extract or --measure)";
        throw std::runtime_error(err);
    }

    std::set<std::string> required_args;
    std::set<std::string> allowed_args;

    if (args.hide) {
        if (random_message) {
            required_args = {"--hide", "--random", "-c", "-o"};
            allowed_args = {"--rmax", "--gmax", "--bmax", "--amax"};
        } else {
            required_args = {"--hide", "-m", "-c", "-o"};
            allowed_args = {"--rmax", "--gmax", "--bmax", "--amax"};
        }
    } else if (args.extract) {
        required_args = {"--extract", "-s", "-o"};
    } else if (args.measure) {
        required_args = {"--measure", "-c", "-t"};
        allowed_args = {"--rmax", "--gmax", "--bmax", "--amax"};
    }

    allowed_args.insert(required_args.begin(), required_args.end());

    for (auto& arg : required_args) {
        if (!raw_args.arg_is_present(arg)) {
            auto err = std::format("missing argument {}", arg);
            throw std::runtime_error(err);
        }
    }

    auto all_args = raw_args.flags;
    for (auto& kv : raw_args.value_args) {
        all_args.insert(kv.first);
    }

    for (auto& arg : all_args) {
        if (!allowed_args.contains(arg)) {
            auto err = std::format("unexpected argument {}", arg);
            throw std::runtime_error(err);
        }
    }

    if (args.hide) {
        if (random_message) {
            args.random_count = raw_args.get_integer_or_default_with_range("--random", -1, 0, 2000000000);
        } else {
            args.message_file = raw_args.get_value_or_throw("-m");
            args.random_count = -1;
        }
        args.cover_file = raw_args.get_value_or_throw("-c");
        args.output_file = raw_args.get_value_or_throw("-o");
        args.rmax = raw_args.get_integer_or_default_with_range("--rmax", 8, 0, 8);
        args.gmax = raw_args.get_integer_or_default_with_range("--gmax", 8, 0, 8);
        args.bmax = raw_args.get_integer_or_default_with_range("--bmax", 8, 0, 8);
        args.amax = raw_args.get_integer_or_default_with_range("--amax", 8, 0, 8);

        auto ext = get_file_extension(args.output_file);
        if (ext != "bmp" && ext != "png" && ext != "tga") {
            auto err = std::format("output file extension must be one of bmp, png or tga");
            throw std::runtime_error(err);
        }
    } else if (args.extract) {
        args.stego_file = raw_args.get_value_or_throw("-s");
        args.output_file = raw_args.get_value_or_throw("-o");
    } else if (args.measure) {
        args.cover_file = raw_args.get_value_or_throw("-c");
        args.threshold = raw_args.get_float_or_default_with_range("-t", 0.3f, 0.0f, 0.5f);
        args.rmax = raw_args.get_integer_or_default_with_range("--rmax", 8, 0, 8);
        args.gmax = raw_args.get_integer_or_default_with_range("--gmax", 8, 0, 8);
        args.bmax = raw_args.get_integer_or_default_with_range("--bmax", 8, 0, 8);
        args.amax = raw_args.get_integer_or_default_with_range("--amax", 8, 0, 8);
    }

    return args;
}
