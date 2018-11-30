# Cmdline

[![Build Status](https://travis-ci.org/abolz/CmdLine2.svg?branch=master)](https://travis-ci.org/abolz/CmdLine2)
[![Build status](https://ci.appveyor.com/api/projects/status/4sgf2oay90gxbc06/branch/master?svg=true)](https://ci.appveyor.com/project/abolz/cmdline2/branch/master)
[![codecov](https://codecov.io/gh/abolz/CmdLine2/branch/master/graph/badge.svg)](https://codecov.io/gh/abolz/CmdLine2)

Command line argument parser (C++14)

## Quick start

The following example shows how to use the library to parse command line
arguments from the `argv` array.

```c++
// Include the CmdLine library.
// It's header-only.
#include "Cmdline.h"

int main(int argc, char* argv[])
// or:
// int wmain(int argc, wchar_t* argv[])
{
    // Create a command line parser.
    cl::Cmdline cli("Example", "A short description");

    // The parser needs to know about the valid options (and the syntax which
    // might be used to specifiy the options on the command line).
    // Options can be created and attached to the parser using the Cmdline::Add
    // method.

    // This adds an option to enable (or disable) debug output.
    // The value of the option will be assigned to the variable `debug`.

    bool debug = false;

    cli.Add(
        // The first three parameters are mandatory.

        // 1)
        //
        // The 1st parameter is the name of the option. Multiple option names
        // may be separated by `|`. So the `debug` option might be specified as
        // `-d` or `--debug`.
        "d|debug",

        // 2)
        //
        // The 2nd parameter provides a short description of the option which
        // will be displayed in the automatically generated help message.
        "Enable debug output",

        // 3)
        //
        // The 3rd parameter is the actual option parser which effectively takes
        // the string representation of the command line argument and is used to
        // convert the option (or the options' argument - if present) into the
        // target object.
        // For now it should be sufficient to note that Var(T& target) is a
        // convenience function which converts the string as given on the
        // command line into an object of type `T` and assigns the result to the
        // given target variable.
        // More advanced usages of the parser will be described later.
        cl::Var(debug),

        // 4)
        //
        // The following parameters are optional and specifiy how and how often
        // the option might be specified on the command line and whether the
        // option takes an argument or not. These flags might be specified in
        // any order.

        // The `Multiple` flag can be used to tell the parser that an option
        // may appear multiple times on the command line.
        // The default value for `Multiple` is `no`, i.e. the option may
        // appear at most once on the command line.
        cl::Multiple::yes,

        // The `Arg` flag can be used to tell the parser whether the option
        // accepts an argument. The `optional` value means that an argument is
        // optional, i.e. both `--debug` and `--debug=on` or `--debug=false` are
        // valid.
        // The default value is `Arg::no`, i.e. an argument is not allowed.
        cl::Arg::optional

        // There are more options which can be used to customize the syntax for
        // options. These are described later.
        );

    // The next command adds an option which may be specified multiple times and
    // the argument of each occurence will be added to the `include_directories`
    // array defined below.

    std::vector<std::string> include_directories;
    // or:
    // std::vector<std::wstring> include_directories;

    cli.Add("I", "Include directories",

        // `Var` works with STL containers, too.
        cl::Var(include_directories),

        // A value of `yes` allows to specify the option multiple times
        // on the command line. Each occurrence will add a string to the
        // `include_directories` array.
        cl::Multiple::yes,

        // The `Arg::required` flag tells the parser that an "-I" must have an
        // argument specified. Either using the "-I=dir" syntax or as an
        // additional argument like "-I dir".
        cl::Arg::required
        );

    // The `cli` object now knows everything it needs to know to parse the
    // command line argument array.

    // The Cmdline::Parse method takes a pair of iterators, parses the command
    // line arguments and calls the options' parser to convert the strings into
    // some objects of another type. Note that the Parse method does not know
    // anything about the program name which is stored in argv[0], so we skip
    // that.
    auto const result = cli.Parse(argv + 1, argv + argc);
    if (!result)
    {
        // If the method returns false, the command line was invalid, i.e. the
        // options were specified with an invalid syntax or there were
        // unrecognized options.

        // In this case the parser has generated more detailed error messages
        // which may be retrieved using the `Cmdline::diag()` method which
        // returns an array with error (and/or warning messages). For
        // convenience the `Cmdline::PrintDiag()` method can be used to print
        // these messages to `stderr`.
        cli.PrintDiag();

        // To provide the user with some information how to avoid these mistakes
        // the Cmdling objects provides a `FormatHelp` methods which returns a
        // string containing a description of all the options which the program
        // accepts.
        // Again, for convenience, the `PrintHelp` method prints this help
        // message to `stderr`. Here we need to tell the CmdLine library the
        // program name in order to print some nicer help message...
        cli.PrintHelp();

        return -1;
    }

    // The command line has been successfully parsed.
    // Now do something useful.

    if (debug)
        printf("Debug output enabled");
    else
        printf("Debug output disabled");

    for (auto const& s : include_directories) {
        printf("I=%s\n", s.c_str());
    }

    return 0;
}
```

## Documentation

More detailed information can be found in the [API-Reference](/doc/API.md).
