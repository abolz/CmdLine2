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
        // 1)
        //
        // The 1st parameter is the name of the option. Multiple option names
        // may be separated by `|`. So the `debug` option might be specified as
        // `-d` or `--debug`.
        //
        "d|debug",

        // 2)
        //
        // The 2nd parameter provides a short description of the option which
        // will be displayed in the automatically generated help message.
        //
        "Enable debug output",

        // 3)
        //
        // The 3rd parameter is a combination of flags, which specify ow and how
        // often the option might be specified on the command line and whether
        // the option takes an argument or not.
        //
        // The `Multiple` flag can be used to tell the parser that an option
        // may appear multiple times on the command line.
        // The default value for `Multiple` is `no`, i.e. the option may
        // appear at most once on the command line.
        //
        cl::Multiple::yes
        //
        // The `Arg` flag can be used to tell the parser whether the option
        // accepts an argument. The `optional` value means that an argument is
        // optional, i.e. both `--debug` and `--debug=on` or `--debug=false` are
        // valid.
        // The default value is `Arg::no`, i.e. an argument is not allowed.
        //
        | cl::Arg::optional,
        //
        // There are more options which can be used to customize the syntax for
        // options. These are described later.
        //

        // 4)
        //
        // The 4th parameter is the actual option parser which effectively takes
        // the string representation of the command line argument and is used to
        // convert the option (or the options' argument - if present) into the
        // target object.
        //
        // For now it should be sufficient to note that Var(T& target) is a
        // convenience function which converts the string as given on the
        // command line into an object of type `T` and assigns the result to the
        // given target variable.
        // More advanced usages of the parser will be described later.
        //
        cl::Var(debug)
    );

    // The next command adds an option which may be specified multiple times and
    // the argument of each occurence will be added to the `include_directories`
    // array defined below.

    std::vector<std::string> include_directories;
    // or:
    // std::vector<std::wstring> include_directories;

    cli.Add("I", "Include directories",

        // The `Arg::required` flag tells the parser that an "-I" must have an
        // argument specified. Either using the "-I=dir" syntax or as an
        // additional argument like "-I dir".
        cl::Multiple::yes | cl::Arg::required,

        // `Var` works with STL containers, too.
        cl::Var(include_directories)
        );

    // The `cli` object now knows everything it needs to know to parse the
    // command line argument array.

    // The Cmdline::Parse method takes a pair of iterators, parses the command
    // line arguments and calls the options' parser to convert the strings into
    // some objects of another type. Note that the Parse method does not know
    // anything about the program name which is stored in argv[0], so we skip
    // that.
    auto const result = cli.Parse(argv + 1, argv + argc);

    // The parser may generate warning and/or error messages. These messages
    // may be obtained with `Cmdline::diag()`. For convenience the `PrintDiag()`
    // method prints the diagnostic messages to `stderr`.
    cli.PrintDiag();

    if (!result) {
        // The result of `Cmdline::Parse` can be explicitly converted to `bool`
        // to indicate success or failure. If the options were specified with
        // an invalid syntax or a conversion error has occurred, the result will
        // be `false`.
        //
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
    // Now do something useful...

    return 0;
}
```

## Option flags

As noted above, the `flags` argument (3rd) controls how the options must/may be
specified on the command line. Here is a list of the options flags the library
supports.

In the examples below, the `descr` parameter is used to indicate the type of the
variable.

> There are (currently) no restrictions on the combinations of flags. You may
> combine any flag with any other, but the syntax may become odd or ambiguous.
> So be careful.

> **TODO**
>
> Implement `Cmdline::SyntaxCheck()` to check for odd or ambiguous option
> combinations. Or perform this check (in debug-mode only) in `Cmdline::Add()`?

### `enum class Required`

Controls whether an option must appear on the command line.

* `no` <br/>
    The option is not required to appear on the command line.
    This is the **default**.
    ```c++
    cli.Add("f", "bool", Required::no, Var(f));

    // ""       ==>  f = <initial value>
    // "-f"     ==>  f = true
    // "-f -f"  ==>  Error
    ```

* `yes` <br/>
    The option is required to appear on the command line.
    ```c++
    cli.Add("f", "bool", Required::yes, Var(f));

    // ""       ==>  Error
    // "-f"     ==>  f = true
    // "-f -f"  ==>  Error
    ```

### `enum class Multiple`

Determines whether an option may appear multiple times on the command line.

* `no` <br/>
    The option may appear (at most) once on the command line.
    This is the **default**.

* `yes` <br/>
    The option may appear multiple times on the command line.
    ```c++
    cli.Add("f", "bool", Multiple::yes, Var(f));

    // ""       ==>  f = <initial value>
    // "-f"     ==>  f = true
    // "-f -f"  ==>  f = true

    cli.Add("f", "bool", Multiple::yes | Required::yes, Var(f));

    // ""       ==>  Error
    // "-f"     ==>  f = true
    // "-f -f"  ==>  f = true
    ```

### `enum class Arg`

Controls the number of arguments the option accepts.

* `no` <br/>
    An argument is not allowed. This is the **default**.
    ```c++
    cli.Add("f", "bool", Arg::no, Var(f));

    // "-f"     ==>  f = true
    // "-f=on"  ==>  Error
    ```

* `optional` <br/>
    An argument is optional.
    ```c++
    cli.Add("f", "bool", Arg::optional, Var(f));

    // "-f"        ==>  f = true
    // "-f=false"  ==>  f = false
    ```

* `yes` <br/>
    An argument is required. If this flag is set, the parser may steal
    arguments from the command line if none is provided using the `=` syntax.
    ```c++
    cli.Add("f", "bool", Arg::required, Var(f));

    // "-f"      ==>  Error
    // "-f=1"    ==>  f = true
    // "-f yes"  ==>  f = true
    ```

### `enum class MayJoin`

Controls whether the option may/must join its argument.

* `no` <br/>
    The option must not join its argument: `"-I dir"` and `"-I=dir"`
    possible. If the option is specified with an equals sign (`"-I=dir"`) the
    `'='` will _not_ be part of the option argument. This is the **default**.
    ```c++
    cli.Add("I", "Include directory", Arg::yes | MayJoin::no, Var(I));

    // "-I"      ==>  Error
    // "-Idir"   ==>  Error
    // "-I dir"  ==>  I = "dir"
    // "-I=dir"  ==>  I = "dir"
    ```

* `yes` <br/>
    The option may join its argument: `"-I dir"` and `"-Idir"` are
    possible. If the option is specified with an equals sign (`"-I=dir"`) the
    `'='` will be part of the option argument.
    ```c++
    cli.Add("I", "Include directory", Arg::yes | MayJoin::yes, Var(I));

    // "-I"      ==>  Error
    // "-Idir"   ==>  I = "dir"
    // "-I dir"  ==>  I = "dir"
    // "-I=dir"  ==>  I = "=dir"
    ```

### `enum class MayGroup`

May this option group with other options?

* `no` <br/>
    The option may not be grouped with other options (even if the option
    name consists only of a single letter). This is the **default**.
    ```c++
    cli.Add("x", "bool", Arg::no | MayGroup::no, Var(x));
    cli.Add("v", "bool", Arg::no | MayGroup::no, Var(v));

    // "-x -v"  ==>  x = true, v = true
    // "-xv"    ==>  Error
    ```

* `yes` <br/>
    The option may be grouped with other options. This flag is ignored if
    the name of the option is not a single letter and option groups _must_ be
    prefixed with a single `'-'`, e.g. `"-xvf=file"`.
    ```c++
    cli.Add("x", "bool", Arg::no | MayGroup::yes, Var(x));
    cli.Add("v", "bool", Arg::no | MayGroup::yes, Var(v));

    // "-x -v"  ==>  x = true, v = true
    // "-xv"    ==>  x = true, v = true
    // "--xv"   ==>  Error (not an option group)
    ```
    Options which require an argument may be grouped, too. But the first option
    in a group, which requires an argument, terminates the option group, e.g.,
    ```c++
    cli.Add("x", "bool", MayGroup::yes | Arg::no, Var(x));
    cli.Add("v", "bool", MayGroup::yes | Arg::no, Var(v));
    cli.Add("f", "str", MayGroup::yes | Arg::yes, Var(v));

    // "-x -v -f file"  ==>  x = true, v = true, f = "file"
    // "-xvf file"      ==>  x = true, v = true, f = "file"
    // "-xvf=file"      ==>  x = true, v = true, f = "file"
    // "-xf"            ==>  Error (option 'f' requires an argument)
    // "-xfv=file"      ==>  Error (option 'f' must not join its argument)
    // "-xfv"           ==>  Error (option 'f' must not join its argument)
    // "--xfv"          ==>  Error (unknown option '--xvf')
    ```
    If option `f` may join its argument, the syntax would become
    ```c++
    cli.Add("x", "bool", MayGroup::yes | Arg::no, Var(x));
    cli.Add("v", "bool", MayGroup::yes | Arg::no, Var(v));
    cli.Add("f", "str", MayJoin::yes | MayGroup::yes | Arg::yes, Var(v));

    // "-x -v -f file"  ==>  as before
    // "-xvf file"      ==>  as before
    // "-xvf=file"      ==>  x = true, v = true, f = "=file"
    // "-xf"            ==>  Error (option 'f' requires an argument)
    // "-xfv=file"      ==>  x = true, v = false, f = "v=file"
    // "-xfv"           ==>  x = true, v = false, f = "v"
    // "--xfv"          ==>  Error (unknown option '--xvf')
    ```
    As noted above, there are no restrictions on the combinations of the flags,
    so be careful.

### `enum class Positional`

Positional option?

* `no` <br/>
    The option is not a positional option, i.e. it requires `'-'` or `'--'`
    as a prefix when specified. This is the **default**.
    ```c++
    cli.Add("f", "str", Arg::yes | Positional::no, Var(f));

    // "--f file"  ==>  f = "file"
    // "file"      ==>  Error
    ```

* `yes` <br/>
    Positional option, no `'-'` required.
    ```c++
    cli.Add("f", "str", Arg::yes | Positional::yes, Var(f));

    // "--f=file"  ==>  f = "file"
    // "file"      ==>  f = "file"
    ```
    Multiple positional options are allowed. They are processed in the order as
    they are attached to the parser until they no longer accept any further
    occurrence. E.g.
    ```c++
    cli.Add("f1", "str", Positional::yes, Var(f1));
    cli.Add("f2", "vec<str>", Multiple::yes | Positional::yes, Var(f2));

    // "eins"            ==>  f1 = "eins", f2 = {}
    // "eins zwei drei"  ==>  f1 = "eins", f2 = {"zwei", "drei"}
    ```
    As can be seen above, positional options still can be specified using the
    `-f1=eins` syntax. In particular, if the `Arg::yes` has been set, the syntax
    may even be `-f1 eins`.

    Again this may lead to ambiguities:
    ```c++
    cli.Add("f1", "str", Positional::yes, Var(f1));
    cli.Add("f2", "str", Positional::yes | Arg::yes, Var(f2));

    // "eins zwei"     ==>  f1 = "eins", f2 = "zwei"
    // "-f1 eins"      ==>  f1 = "", f2 = "zwei"
    // "-f1 -f2 zwei"  ==>  f1 = "", f2 = "zwei"
    ```
    compared to:
    ```c++
    cli.Add("f1", "str", Positional::yes | Arg::yes, Var(f1));
    cli.Add("f2", "str", Positional::yes, Var(f2));

    // "eins zwei"          ==>  f1 = "eins", f2 = "zwei"
    // "-f1 eins"           ==>  f1 = "eins", f2 = ""
    // "-f1 -f2=zwei"       ==>  f1 = "-f2=zwei", f2 = ""
    // "-f1 -f2=zwei zwei"  ==>  f1 = "-f2=zwei", f2 = "zwei"
    ```
    Note that the last two examples emit a warning:

    > warning: option `'f2'` is used as an argument for option `'f1'` <br/>
    > note: use `'--f1=-f2=zwei'` to suppress this warning

    which indicates a possible missing argument for `f1`. As noted,
    ```c++
    // "-f1=-f2=zwei zwei"  ==>  f1 = "-f2=zwei", f2 = "zwei"
    ```
    does not emit a warning.

### `enum class CommaSeparated`

> **TODO**

### `enum class StopParsing`

> **TODO**

## Option parser

> **TODO**

### General

```c++
bool parse(ParseContext const& ctx); // 'const' is required here
```

```c++
ctx.name     // option name
ctx.arg      // option argument
ctx.index    // index in argv[]
ctx.cmdline  // for warning and/or error messages
```

```c++
auto parse = [](ParseContext const& ctx) {
    std::cout << "Index " << ctx.index << ":"
        << " --" << ctx.name << " " << ctx.arg << "\n";
    if (ctx.name == "b") {
        ctx.cmdline->EmitDiag(
            Diagnostic::warning, ctx.index, "option 'b' is deprecated");
    }
    return true;
};

cli.Add("a|b", "custom", Arg::optional | Multiple::yes, parse);

// "-a -b=eins -a=zwei" ==>
//      Index 0: --a
//      Index 1: --b eins
//      Index 2: --a zwei
```
and emits a warning:

> warning: option 'b' is deprecated

### Predefined

#### `Assign`

* `bool`
* integers
* floating-point
* strings
* stream extraction operator `operator<<(ostream, T)`

#### `Append`

#### `Var`

#### `Map`

### Predicates / Filters / Transforms

```c++
bool predicate(ParseContext const&, auto /*[const]*/& value);
```

## Command-line parser

> **TODO**

## Unicode support

> **TODO**

Yes. UTF conversions built-in.

## Tokenize

> **TODO**

```c++
const auto vec = TokenizeWindows("-a -b -c")
// vec = {"-a", "-b", "-c"}
```

```c++
const auto vec = TokenizeUnix("-a -b -c");
// vec = {"-a", "-b", "-c"}
```

## Subcommands

> **TODO**

No, not really. [Example](/test/Example_subcommands.cc).

## Console colors

> **TODO**

```c++
#define CL_ANSI_CONSOLE_COLORS 1 // Windows/Unix (if supported)
#define CL_WINDOWS_CONSOLE_COLORS 1 // Windows only
```
