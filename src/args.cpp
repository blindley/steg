// Benjamin Lindley, Vanessa Martinez
//
// args.cpp
//
// Handles the parsing of command line arguments, and displaying documentation on usage to the user.

#include <map>
#include <set>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include "declarations.h"

// Replaces <pattern> with <replacement> wherever it occurs in <str>
template<typename R>
void replace_substring(std::string& str, std::string pattern, R replacement) {
    std::ostringstream oss;
    oss << replacement;
    auto replacement_str = oss.str();
    
    size_t i = 0;
    while ((i = str.find(pattern, i)) != std::string::npos) {
        str.replace(i, pattern.size(), replacement_str);
        i += replacement_str.size();
    }
}

// Strips executable path down to just the filename + extension, removing the directory
//
// Pass argv[0] from main to this function. This is used to keep the usage documentation more brief
// in case the full path to the executable is long.
std::string get_exe_short_name(char const* argv0) {
    char const* after_last_slash = argv0;
    for (size_t i = 0; argv0[i] != 0; i++) {
        if (argv0[i] == '/' || argv0[i] == '\\') {
            after_last_slash = argv0 + i + 1;
        }
    }
    return after_last_slash;
}

// Prints the basic usage parameters, but not the full help
void print_usage(char const* exe_name) {
    auto exe_short_name = get_exe_short_name(exe_name);

    std::cout << "version: 08/12/2023\n";
    std::cout << "Usage:\n";
    std::cout << "    " << exe_short_name
        << " --hide -m <message file> -c <coverfile> -o <stego file> [-t <threshold>]\n"
        << "        [--rmax <n>] [--gmax <n>] [--bmax <n>] [--amax <n>]\n";
    std::cout << "    " << exe_short_name
        << " --hide --random <count> -c <coverfile> -o <stego file> [-t <threshold>]\n"
        << "        [--rmax <n>] [--gmax <n>] [--bmax <n>] [--amax <n>]\n";
    std::cout << "    " << exe_short_name
        << " --extract -s <stego file> -o <message file>\n";
    std::cout << "    " << exe_short_name
        << " --measure -c <cover file> -t <threshold>\n"
        << "        [--rmax <n>] [--gmax <n>] [--bmax <n>] [--amax <n>]\n";
    std::cout << "    " << exe_short_name << " --help\n";

    std::cout << "\n(try --help for more details)\n";
}

// Prints the full help, including basic usage, parameter descriptions and examples
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
        "  -m <message file>   Message file to hide. Exclusive with --random.",
        "  --random <count>    Fill cover file with <count> random bytes. Exclusive with -m.",
        "  -o <stego file>     Name of output stego image file",
        "  -t <threshold>      Complexity threshold [0, 0.5]. default=dynamic threshold",
        "  --rmax <n>          Max red bitplanes to use ([0,8], default={BP})",
        "  --gmax <n>          Max green bitplanes to use ([0,8], default={BP})",
        "  --bmax <n>          Max blue bitplanes to use ([0,8], default={BP})",
        "  --amax <n>          Max alpha bitplanes to use ([0,8], default={BP})",
        "",
        "Extract Mode Options:",
        "  -s <stego file>     Stego file to extract hidden message from",
        "  -o <message file>   Name of output message file",
        "",
        "Measure Mode Options:",
        "  -c <cover file>     Cover image to measure for capacity",
        "  -t <threshold>      Complexity threshold to measure for [0,0.5]",
        "  --rmax <n>          Max red bitplanes to use ([0,8], default={BP})",
        "  --gmax <n>          Max green bitplanes to use ([0,8], default={BP})",
        "  --bmax <n>          Max blue bitplanes to use ([0,8], default={BP})",
        "  --amax <n>          Max alpha bitplanes to use ([0,8], default={BP})",
        "",
        "Examples:",
        "  {steg.exe} --hide -c cover.jpg -m message.txt --amax 0 -o hidden.png",
        "       Hide message.txt in cover.jpg. Do not use any bitplanes from the",
        "       alpha channel. Output to hidden.png",
        "",
        "  {steg.exe} --extract -s hidden.png -o extracted.txt",
        "       Extract a hidden message from hidden.png. Output to extracted.txt",
        "",
        "  {steg.exe} --measure -s cover.bmp -t 0.3 --rmax 4 --gmax 4 --bmax 4 --amax 2",
        "       Measure the hiding capacty for cover.bmp at complexity threshold",
        "       = 0.3, using 4 bitplanes each for the red, green and blue channels,",
        "       and 2 bitplanes for the alpha channel.",
        "",
        "  {steg.exe} --hide -c cover.png -m - -o hidden.tga",
        "       Read a message from standard input (note the '-' in place of a",
        "       filename), hide it in cover.png, output to hidden.tga. The",
        "       message can also be piped in this way.",
        "",
        "  {steg.exe} --extract -s hidden.tga -o -",
        "       Extract hidden message and output to standard output. Not",
        "       recommended on Windows unless you know for sure that the",
        "       hidden message is text.",
        "",
        "  {steg.exe} --hide -c cover.jpg --random 10000 -o hidden.png",
        "       Hide 10000 random bytes in cover.jpg. Output to hidden.png.",
    };

    auto exe_short_name = get_exe_short_name(argv0);

    for (auto& line : help_lines) {
        std::string formatted = line;
        // The executable might be named something besides steg.exe, so we replace it in the help
        // examples with the actual executable name.
        replace_substring(formatted, "{steg.exe}", exe_short_name);
        replace_substring(formatted, "{BP}", DEFAULT_BITPLANE_USAGE);
        std::cout << formatted << '\n';
    }
}

// Consists of a dictionary (map) mapping argument names to their string values which were passed on
// the command line, and a set containing boolean flags which were present.
struct RawArgs {
    std::set<std::string> flags;
    std::map<std::string, std::string> value_args;

    // Checks if an argument or flag is present
    bool arg_is_present(std::string const& arg_name) const {
        return flags.contains(arg_name) || value_args.contains(arg_name);
    }

    // Gets the value of an argument. Throws an exception if the argument was not passed.
    std::string get_value_or_throw(std::string const& arg_name) const {
        try {
            return value_args.at(arg_name);
        } catch(...) {
            std::ostringstream oss;
            oss << "missing argument " << arg_name;
            auto err = oss.str();
            throw std::runtime_error(err);
        }
    }

    // Gets an integer argument. Throws exception if the argument was not an integer, or was not in
    // the specified range. If the argument was not present, then supplies a default value.
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

        std::ostringstream oss;
        oss << arg_name << " should be integer in range [" << low << ", " << high << "]";
        auto err = oss.str();
        throw std::runtime_error(err);
    }

    // Gets an floating point argument. Throws exception if the argument can not be parsed as a
    // flaot, or was not in the specified range. If the argument was not present, then supplies a
    // default value.
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
        std::ostringstream oss;
        oss << arg_name << " should be real number in range [" << low << ", " << high << "]";
        auto err = oss.str();
        throw std::runtime_error(err);
    }
};

// Parses the command line arguments. <flag_names> are the arguments which are flags, that is, they
// are either present or not, they do not take a value. <value_arg_names> are arguments which take a
// value. There are no arguments which take multiple values in this program, so that case is not
// handled. The types of the arguments are not regarded here. Everything is a string.
RawArgs collect_raw_args(int argc, char** _argv,
    std::set<std::string> const& flag_names, std::set<std::string> value_arg_names)
{
    std::set<std::string> flags;
    std::map<std::string, std::string> value_args;

    // I've been burned a few times forgetting to use strcmp for string comparisons, so now I just
    // convert the command line arguments to std::string up front to keep things simple.
    std::vector<std::string> argv(_argv, _argv + argc);

    for (size_t i = 1; i < argv.size(); i++) {
        // Throw an error on duplicate arguments
        if (flags.contains(argv[i]) || value_args.contains(argv[i])) {
            std::ostringstream oss;
            oss << "duplicate argument " << argv[i];
            auto err = oss.str();
            throw std::runtime_error(err);
        }

        if (flag_names.contains(argv[i])) {
            flags.insert(argv[i]);
            continue;
        }

        if (value_arg_names.contains(argv[i])) {
            ++i; // go to the next argument, which should be the value for this argument

            // reached end of argument list prematurely, when we were expecting a value for another
            // argument
            if (i == argv.size()) {
                std::ostringstream oss;
                oss << "unexpected end of argument list. missing value for" << argv[i-1];
                auto err = oss.str();
                throw std::runtime_error(err);
            }

            value_args[argv[i-1]] = argv[i];
            continue;
        }

        // if we're here, then the argument was not in <flags_names> or <value_arg_names>
        std::ostringstream oss;
        oss << "unexpected argument " << argv[i];
        auto err = oss.str();
        throw std::runtime_error(err);
    }

    RawArgs collection;
    collection.flags = std::move(flags);
    collection.value_args = std::move(value_args);

    return collection;
}

// Parses the command line, returning an Args object containing the properly typed arguments
Args parse_args(int argc, char** argv) {
    auto raw_args = collect_raw_args(argc, argv,
        // flags
        {"--hide", "--extract", "--measure", "--help"},

        // value args
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
    bool message_is_random = raw_args.arg_is_present("--random");

    // here we're using the conversion of boolean false -> 0, true -> 1 to count selected modes
    int num_modes = (int)args.hide + (int)args.extract + (int)args.measure;

    // At least one mode (hide, extract or measure) must be selected
    if (num_modes == 0) {
        std::ostringstream oss;
        oss << "no mode selected (--hide, --extract or --measure)";
        auto err = oss.str();
        throw std::runtime_error(err);
    }

    // No more than one mode can be selected
    if (num_modes > 1) {
        std::ostringstream oss;
        oss << "multiple modes selected (choose one of --hide, --extract or --measure)";
        auto err = oss.str();
        throw std::runtime_error(err);
    }

    std::set<std::string> required_args;
    std::set<std::string> allowed_args;

    if (args.hide) {
        // --random is exlusive with -m. If one is present, the other is not allowed.
        if (message_is_random) {
            required_args = {"--hide", "--random", "-c", "-o"};
            allowed_args = {"-t", "--rmax", "--gmax", "--bmax", "--amax"};
        } else {
            required_args = {"--hide", "-m", "-c", "-o"};
            allowed_args = {"-t", "--rmax", "--gmax", "--bmax", "--amax"};
        }
    } else if (args.extract) {
        required_args = {"--extract", "-s", "-o"};
    } else if (args.measure) {
        required_args = {"--measure", "-c", "-t"};
        allowed_args = {"--rmax", "--gmax", "--bmax", "--amax"};
    }

    // required args are also allowed args, obviously
    allowed_args.insert(required_args.begin(), required_args.end());

    // throw an error if any of the required args are not present
    for (auto& arg : required_args) {
        if (!raw_args.arg_is_present(arg)) {
            std::ostringstream oss;
            oss << "missing argument " << arg;
            auto err = oss.str();
            throw std::runtime_error(err);
        }
    }

    // a single set containing the flags and value args (but with the values removed), for the
    // purpose of checking the presence of the argument, whatever its type
    auto all_args = raw_args.flags;
    for (auto& kv : raw_args.value_args) {
        all_args.insert(kv.first);
    }

    // throw an error if an argument is present which is not in the set of allowed arguments
    for (auto& arg : all_args) {
        if (!allowed_args.contains(arg)) {
            std::ostringstream oss;
            oss << "unexpected argument " << arg;
            auto err = oss.str();
            throw std::runtime_error(err);
        }
    }

    if (args.hide) {
        if (message_is_random) {
            args.random_count = raw_args.get_integer_or_default_with_range("--random", -1, 0, 2000000000);
        } else {
            args.message_file = raw_args.get_value_or_throw("-m");
            args.random_count = -1;
        }
        args.cover_file = raw_args.get_value_or_throw("-c");
        args.output_file = raw_args.get_value_or_throw("-o");

        args.threshold = raw_args.get_float_or_default_with_range("-t", -1.0f, 0.0f, 0.5f);

        args.rmax = (u8)raw_args.get_integer_or_default_with_range("--rmax", DEFAULT_BITPLANE_USAGE, 0, 8);
        args.gmax = (u8)raw_args.get_integer_or_default_with_range("--gmax", DEFAULT_BITPLANE_USAGE, 0, 8);
        args.bmax = (u8)raw_args.get_integer_or_default_with_range("--bmax", DEFAULT_BITPLANE_USAGE, 0, 8);
        args.amax = (u8)raw_args.get_integer_or_default_with_range("--amax", DEFAULT_BITPLANE_USAGE, 0, 8);

        auto ext = get_file_extension(args.output_file);
        if (ext != "bmp" && ext != "png" && ext != "tga") {
            std::ostringstream oss;
            oss << "output file extension must be one of bmp, png or tga";
            auto err = oss.str();
            throw std::runtime_error(err);
        }
    } else if (args.extract) {
        args.stego_file = raw_args.get_value_or_throw("-s");
        args.output_file = raw_args.get_value_or_throw("-o");
    } else if (args.measure) {
        args.cover_file = raw_args.get_value_or_throw("-c");
        args.threshold = raw_args.get_float_or_default_with_range("-t", 0.3f, 0.0f, 0.5f);
        args.rmax = (u8)raw_args.get_integer_or_default_with_range("--rmax", DEFAULT_BITPLANE_USAGE, 0, 8);
        args.gmax = (u8)raw_args.get_integer_or_default_with_range("--gmax", DEFAULT_BITPLANE_USAGE, 0, 8);
        args.bmax = (u8)raw_args.get_integer_or_default_with_range("--bmax", DEFAULT_BITPLANE_USAGE, 0, 8);
        args.amax = (u8)raw_args.get_integer_or_default_with_range("--amax", DEFAULT_BITPLANE_USAGE, 0, 8);
    }

    return args;
}
