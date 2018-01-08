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
{
    // Create a command line parser.
    cl::Cmdline cmd;

    // The parser needs to know about the valid options (and the syntax which
    // might be used to specifiy the options on the command line).
    // Options can be created and attached to the parser using the Cmdline::Add
    // method.

    // This adds an option to enable (or disable) debug output.
    // The value of the option will be assigned to the variable `debug`.

    bool debug = false;

    cmd.Add(
        // The first three parameters are mandatory.

        // The 1st parameter is the name of the option. Multiple option names
        // may be separated by `|`. So the `debug` option might be specified as
        // `-d` or `--debug`.
        "d|debug",
        // The 2nd parameter provides a short description of the option which
        // will be displayed in the automatically generated help message.
        "Enable debug output",
        // The 3rd parameter is the actual option parser which effectively takes
        // the string representation of the command line argument and is used to
        // convert the option (or the options' argument - if present) into the
        // target object.
        // For now it should be sufficient to note that Assign(T& target) is a
        // convenience function which converts the string as given on the
        // command line into an object of type T and assigns the result to the
        // given target variable.
        // More advanced usages of the parser will be described later.
        cl::Assign(debug),

        // The following parameters are optional and specifiy how and how often
        // the option might be specified on the command line and whether the
        // option takes an argument or not. These flags might be specified in
        // any order.

        // The `NumOpts` enum class can be used to tell the parser how often the
        // option may or must occur on the command line. The `zero_or_more`
        // value means the this option is optional and may be specified as many
        // times as the user likes.
        cl::NumOpts::zero_or_more,
        // The `HasArg` enum class can used to tell the parser whether the
        // option accepts an argument.
        // The `optional` value means that an argument is optional, i.e. both
        // `--debug` and `--debug=on` or `--debug=false` are valid.
        cl::HasArg::optional

        // There are more options which can be used to customize the syntax for
        // options. These are described later.
        );

    // The next command adds an option which may be specified multiple times
    // and the argument of each occurence will be added to the
    // `include_directories` array defined below.

    std::vector<std::string> include_directories;

    cmd.Add("I", "Include directories",
        // Like Assign(), PushBack() is a convenience function which returns a
        // parser which in turn used a containers' `push_back` method to append
        // arguments to an STL container.
        cl::PushBack(include_directories),

        // A value of `zero_or_more` allows to specify the option multiple times
        // on the command line. Each occurrence will add a string to the
        // `include_directories` array.
        cl::NumOpts::zero_or_more,
        // The `HasArg::required` flag tells the parser the an "-I" must have an
        // argument specified. Either using the "-I=dir" syntax or as an
        // additional argument like "-I dir".
        cl::HasArg::required,
        );

    // The `cmd` object now knows everything it needs to know to parse the
    // command line argument array.

    // The Cmdline::Parse method takes a pair of iterators, parses the command
    // line arguments and calls the options' parser to convert the strings into
    // some objects of another type.
    // Note that the Parse method does not know anything about the program name
    // which is stored in argv[0], so we skip that.
    bool const ok = cmd.Parse(argv + 1, argv + argc);
    if (!ok)
    {
        // If the method returns false, the command line was invalid, i.e.
        // the options were specified with an invalid syntax or there were
        // unrecognized options.

        // In this case the parser has generated more detailed error messages
        // which may be retrieved using the `Cmdline::diag()` method which
        // returns an array with error (and/or warning messages).
        // For convenience the `Cmdline::PrintDiag()` method prints these
        // messages to `stderr`.
        cmd.PrintDiag();

        // To provide the user with some information how to avoid these mistakes
        // the Cmdling objects provides a `FormatHelp` methods which returns
        // a string containing a description of all the options which the
        // program accepts.
        // Again, for convenience, the `PrintHelp` method prints this help
        // message to `stderr`.
        // Here we need to tell the CmdLine library the program name in order
        // to print some nicer help message...
        cmd.PrintHelp("Example");

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

## API

### Basic usage

...

### The `string_view` class

Alias for `std::string_view` if available, incomplete fallback otherwise.

...

### The `Cmdline` class

#### `Add(Args&&... args)`

...

#### `Parse(It first, It last)`

...

#### `Parse(It first, It last, Sink sink)`

This is like the `Parse` function shown above. The difference is that instead of
generating an error for unknown command line arguments, this method calls `sink`
with the unknown argument as a parameter. This allows the user to ignore unknown
arguments and continue parsing.

Note, however, that ignoring unknown arguments might screw up argument parsing.
So use with caution.

#### `ParseArgs(List const& args)`

...

#### `ParseArgs(List const& args, Sink sink)`

Like the `ParseArgs` function shown above, but unknown arguments will be passed
to `sink`.

#### `PrintDiag`

...

#### `FormatHelp`

...

#### `PrintHelp`

...

### The `Option` class

...

#### Creating options

##### `MakeOption -> Option`

...

##### `MakeUniqueOption -> std::unique_ptr<Option>`

...

##### `Option`

Available when class template argument deduction is supported (C++17).

...

##### `Cmdline::Add -> Option`

...

#### Flags

The following flags may be specified as additional arguments to `Cmdline::Add`
and `Make[Unique]Option` to customize how the option (and possibly its argument)
may be specified on the command line.

They may be specified in any order and any combinations. _But note that not all
combinations might make sense!_

##### `enum class NumOpts`

Controls how often an option may/must be specified.

- `optional`:
    The option may appear at most once. This is the **default**.
    ```c++
    cmd.Add("i", "int", cl::Assign(i), cl::NumOpts::optional);
        // OK:   {}
        // OK:   {"-i"}
        // Fail: {"-i", "-i"}
    ```

- `required`:
    The option must appear exactly once.
    ```c++
    cmd.Add("i", "int", cl::Assign(i), cl::NumOpts::required);
        // Fail: {}
        // OK:   {"-i"}
        // Fail: {"-i", "-i"}
    ```

- `zero_or_more`:
    The option may appear multiple times.
    ```c++
    cmd.Add("i", "int", cl::Assign(i), cl::NumOpts::zero_or_more);
        // OK:   {}
        // OK:   {"-i"}
        // OK:   {"-i", "-i"}
    ```

- `one_or_more`:
    The option must appear at least once.
    ```c++
    cmd.Add("i", "int", cl::Assign(i), cl::NumOpts::one_or_more);
        // Fail: {}
        // OK:   {"-i"}
        // OK:   {"-i", "-i"}
    ```

##### `enum class HasArg`

Controls the number of arguments the option accepts.

- `no`:
    An argument is not allowed. This is the **default**.
    ```c++
    cmd.Add("i", "int", cl::Assign(i), cl::HasArg::no);
        // OK:   {"-i"}
        // Fail: {"-i=1"}
    ```

- `optional`:
    An argument is optional.
    ```c++
    cmd.Add("i", "int", cl::Assign(i), cl::HasArg::optional);
        // OK:   {"-i"}
        // OK:   {"-i=1"}
    ```

- `required`:
    An argument is required. If this flag is set, the parser may steal arguments
    from the command line if none is provided using the `=` syntax.
    ```c++
    cmd.Add("i", "int", cl::Assign(i), cl::HasArg::required);
        // Fail: {"-i"}
        // OK:   {"-i=1"}
        // OK:   {"-i", "1"}
    ```

##### `enum class JoinArg`

Controls whether the option may/must join its argument.

- `no`:
    The option must not join its argument: `"-I dir"` and `"-I=dir"` possible.
    If the option is specified with an equals sign (`"-I=dir"`) the `'='` will
    _not_ be part of the option argument. This is the **default**.
    ```c++
    cmd.Add("I", "Include directory", cl::Assign(I), cl::HasArg::required,
        cl::JoinArg::no);
        // Fail: {"-I"}
        // OK:   {"-I=dir"}     ==>  I = "dir"
        // OK:   {"-I", "dir"}  ==>  I = "dir"
    ```

- `optional`:
    The option may join its argument: `"-I dir"` and `"-Idir"` are possible. If
    the option is specified with an equals sign (`"-I=dir"`) the `'='` will be
    part of the option argument.
    ```c++
    cmd.Add("I", "Include directory", cl::Assign(I), cl::HasArg::required,
        cl::JoinArg::optional);
        // Fail: {"-I"}
        // OK:   {"-I=dir"}     ==>  I = "=dir"
        // OK:   {"-I", "dir"}  ==>  I = "dir"
    ```

- `yes`:
    The option must join its argument: `"-Idir"` is the only possible format.
    If the option is specified with an equals sign (`"-I=dir"`) the `'='` will
    be part of the option argument.
    ```c++
    cmd.Add("I", "Include directory", cl::Assign(I), cl::HasArg::required,
        cl::JoinArg::yes);
        // Fail: {"-I"}
        // OK:   {"-I=dir"}     ==>  I = "=dir"
        // OK:   {"-Idir"}      ==>  I = "dir"
        // Fail: {"-I", "dir"}
    ```

##### `enum class MayGroup`

May this option group with other options?

- `no`:
    The option may not be grouped with other options (even if the option name
    consists only of a single letter). This is the **default**.
    ```c++
    cmd.Add("x", "", cl::Assign(x), cl::HasArg::no, cl::MayGroup::no);
    cmd.Add("y", "", cl::Assign(y), cl::HasArg::no, cl::MayGroup::no);
        // {"-x", "-y"}  ==>  OK.
        // {"-xy}        ==>  Fail.
    ```

- `yes`:
    The option may be grouped with other options. This flag is ignored if the
    names of the options are not a single letter and option groups must be
    prefixed with a single `'-'`, e.g. `"-xvf=file"`.
    ```c++
    cmd.Add("x", "", cl::Assign(x), cl::HasArg::no, cl::MayGroup::yes);
    cmd.Add("y", "", cl::Assign(y), cl::HasArg::no, cl::MayGroup::yes);
        // {"-x", "-y"}  ==>  OK.
        // {"-xy}        ==>  OK.
    ```

##### `enum class Positional`

Positional option?

- `no`:
    The option is not a positional option, i.e. requires `'-'` or `'--'` as a
    prefix when specified. This is the **default**.
    ```c++
    cmd.Add("input-file", "", cl::Assign(file), cl::HasArg::yes,
        cl::Positional::no);
        // {"--input-file", "<file>"}  ==>  OK. (file == "<file>")
        // {"<file>"}                  ==>  Fail.
    ```
- `yes`:
    Positional option, no `'-'` required.
    ```c++
    cmd.Add("input-file", "", cl::Assign(file), cl::HasArg::yes,
        cl::Positional::yes);
        // {"--input-file", "<file>"}  ==>  OK. (file == "<file>")
        // {"<file>"}                  ==>  OK. (file == "<file>")
    ```

    _Note_: Positional options must have the `HasArg::required` flag set.

    _Note_: Multiple positional options are allowed. They are processed in the
    order as they are attached to the parser until they no longer accept any
    further occurrence. E.g.
    ```c++
    cmd.Add("pos1", "", cl::Assign(pos1), cl::HasArg::yes,
        cl::Positional::yes, cl::NumOpts::optional);
    cmd.Add("pos2", "", cl::PushBack(pos2), cl::HasArg::yes,
        cl::Positional::yes, cl::NumOpts::zero_or_more);
        // {"eins"}                 ==> pos1 = "eins", pos2 = {}
        // {"eins", "zwei", "drei"} ==> pos1 = "eins", pos2 = {"zwei", "drei"}
    ```

##### `enum class CommaSeparated`

Split the argument between commas?

- `no`:
    Do not split the argument between commas. This is the **default**.
    ```c++
    cmd.Add("i", "ints", cl::PushBack(ints), cl::HasArg::required,
        cl::NumOpts::zero_or_more, cl::CommaSeparated::no);
        // {"-i=1", "-i=2"}     ==> OK. (ints = {1,2})
        // {"-i=1,2"}           ==> Fail.
    ```

- `yes`:
    If this flag is set, the option's argument is split between commas, e.g.
    `"-i=1,2,,3"` will be parsed as `["-i=1", "-i=2", "-i=", "-i=3"]`. Note that
    each comma-separated argument counts as an option occurrence.
    ```c++
    cmd.Add("i", "ints", cl::PushBack(ints), cl::HasArg::required,
        cl::NumOpts::zero_or_more, cl::CommaSeparated::no);
        // {"-i=1", "-i=2"}     ==> OK. (ints = {1,2})
        // {"-i=1,2"}           ==> OK. (ints = {1,2})
    ```

##### `enum class EndsOptions`

Parse all following options as positional options?

- `no`:
    Nothing special. This is the **default**.
    ```c++
    ```

- `yes`:
    If an option with this flag is (successfully) parsed, all the remaining
    options are parsed as positional options.
    ```c++
    ```

#### Parsers

While `Flags` control how an option may be specified on the command line, the
option parser is responsible for doing somethingn useful with the command line
argument.

In general, an option parser is a function or a function object with the
following signature

    (ParseContext [const]& ctx) -> bool

(the `const` qualifier is optional). The `ParseContext` class contains the
following members:

- `string_view name`

    Name of the option being parsed (only valid in callback!). The `CmdLine`
    parser uses the options flags to decompose a command line argument into an
    option name and possibly an option argument.

- `string_view arg`

    The option argument (only valid in callback!).

- `int index`

    Current index in the argv array. This might be useful if the meaning of
    options is order-dependent.

- `Cmdline* cmdline`

    The command line parser which currently parses the argument list. This
    pointer is always valid (never null) and can be used, e.g., to emit
    additional errors and/or warnings.

Given such a `ParseContext` object, the parser is responsible for doing
something useful with the given information. This might include just printing
the name and the argument to `stdout` or converting the argument into objects
of type `T`.

The latter seems slightly more useful in general. So the CmdLine library
provides some utility classes and methods to easily convert strings into objects
of other types and assigning the result to user-provided storage.

##### Default parsers

-   `class ConvertTo<T>`

    The member function

        bool operator()(string_view in, T& out) const;

    is responsible for converting strings into objects of type `T`. The function
    returns `true` on success, `false` otherwise.
    The library provides some useful default specializations:

    - `bool`

        Maps strings to boolean `true` or `false` as defined in the following
        table:

        - `true` values: `""` (empty string), `"true"`, `"1"`, `"on"`
        - `false` values: `"false"`, `"0"`, `"off"`

        Other inputs result in parsing errors.

    - integral types `intN_t, uintN_t`

        Uses `strto[u]ll` to convert the string to an integer and performs a
        range check for the limits for the actual integral type. Returns `true`
        if parsing and the range check succeed, returns `false` otherwise.

    - floating-point types `float`, `double`, `long double`

        Uses `strtof`, `strtod` and `strtold` resp. to parse the floating-point
        value. Returns `false` if the functions set `errno` to `ERANGE`, returns
        `true` otherwise.

    The generic implementation uses `std::stringstream` to convert the string
    into an object of type `T`. So if your type has a compatible stream
    extraction `operator>>` defined, you should be able to directly parse
    options into objects of your custom data type without any additional effort.

    **Note**: Using the default implementation, requires to additionally
    `#include <sstream>`.

    On the other hand you may want to specialize `ConvertTo` for your custom
    data type.

-   `class ParseValue<T>`

        bool operator()(ParseContext& ctx, T& out) const;

    Calls `ConvertTo<T>::operator()(ctx.arg, out)` (see below).

    This is the default parser used by all other option parsers defined in the
    CmdLine library. Instead of specializing `ConvertTo<T>` for your custom data
    type `T` you might as well specialize `ParseValue<T>`.

    This might be useful if the value of the option depends on the name of the
    option as specified on the command line. E.g., a flag parser might convert
    a command line argument to a boolean value depending on whether the option
    was specified as `"--debug"` or `"--no-debug"`.

For convenience, the CmdLine library provides some predefined parsers for the
most common use cases.

- `Assign(T& target, UnaryFunctions... fns)`

    Calls `ParseValue` to convert the string into a temporary object `v` of type
    `T`, then calls each `fns` for the result and if all `fns` return
    `true`, move-assigns `v` to `target`.
    ```c++
    int i = 0;

    cmd.Add("i", "int", cl::Assign(i), cl::NumOpts::zero_or_more, cl::HasArg::required);
        // {"-i=1"}             ==> i = 1
        // {"-i", "1", "-i=2"}  ==> i = 2
    ```

- `PushBack(T& target, UnaryFunctions... fns)`

    Calls `ParseValue` to convert the string into a temporary object `v` of type
    `T::value_type`, then calls each `fns` for the result and if all `fns`
    return `true`, calls `target.insert(target.end(), std::move(v))`.
    ```c++
    std::vector<int> ints;

    cmd.Add("i", "int", cl::PushBack(ints), cl::NumOpts::zero_or_more, cl::HasArg::required);
        // {"-i=1"}             ==> i = {1}
        // {"-i=1", "-i", "2"}  ==> i = {1,2}
    ```

- `Map(T& target, std::initializer_list<char const*, T>> map, UnaryFunctions... fns)`

    The returned parser converts strings to objects of type `T` by simply
    looking them up in the given map. This might be useful to parse enum types:
    ```c++
    enum class Simpson { Homer, Marge, Bart, Lisa, Maggie };

    Simpson simpson = Simpson::Homer;

    cmd.Add("simpson", "The name of a Simpson",
        cl::Map(simpson, {{"Homer",    Simpson::Homer },
                          {"Marge",    Simpson::Marge },
                          {"Bart",     Simpson::Bart  },
                          {"El Barto", Simpson::Bart  },
                          {"Lisa",     Simpson::Lisa  },
                          {"Maggie",   Simpson::Maggie}})
        cl::NumOpts::optional,
        cl::HasArg::required);
        // {"-simpson=Bart"}       ==>  simpson == Simpson::Bart
        // {"-simpson=El Barto"}   ==>  simpson == Simpson::Bart
        // {"-simpson", "Granpa"}  ==>  Fail.
    ```

All of the functions defined above take an arbitrary number of additional
_predicates_, which can be used to check if a given value satisfies various
conditions. These predicates must have the following signature:

    (ParseContext [const]& ctx, T [const]& value) -> bool

Note: The `value` argument might be accepted by mutable reference, in which case
the _predicate_ is might actually transform the passed value.

The library provides some predicates in the `cl::check` namespace:

- `check::InRange(T lower, U upper)`

    Returns a function object which checks whether a given value is in the range
    `[lower, upper]`, i.e. returns `!(value < lower) && !(upper < value)`.
    ```c++
    cl::Assign(small_int, cl::InRange(0, 15))
        // In addition to converting the input string into an integer, the
        // function object returned from the function call above additionally
        // checks whether the resuling integer is in the range [0,15].
    ```

- `check::GreaterThan(T lower)`

    Returns a function object which checks whether a given value is greater than
    `lower`, i.e. returns `lower < value`.

- `check::GreaterEqual(T lower)`

    Returns a function object which checks whether a given value is greater than
    or equal to `lower`, i.e. returns `!(value < lower)`.

- `check::LessThan(T upper)`

    Returns a function object which checks whether a given value is less than
    `upper`, i.e. returns `value < upper`.

- `check::LessEqual(T upper)`

    Returns a function object which checks whether a given value is less than or
    equal to `upper`, i.e. returns `!(upper < value)`.

- **XXX** (not implemented) `check::FileExists()`

### Utility functions

- `TokenizeUnix(string_view) -> vector<string>`

    Tokenize a string into command line arguments using `bash`-style escaping.

- `TokenizeWindows(string_view) -> vector<string>`

    Tokenize a string into command line arguments using Windows-style escaping.

    See [CommandLineToArgvW](https://msdn.microsoft.com/en-us/library/windows/desktop/17w5ykft(v=vs.85).aspx)
    for details.

## **TODO**

- Response file

- Wildcard expansion
