## Documentation

### class `CmdLine`

...

### class template `Option<Parser>`

...

#### Base class `OptionBase`

...

#### Creating options

##### `MakeOption(char const* name, char const* descr, Parser&& parser, Flags&&... flags)`

This function creates a new `Option`, which might be attached to a `CmdLine` object (see below).

- `name`

    The name of the new option. This is used to identify the option on the command-line. Multiple
    option names are allowed: `name` can be a list of option names separated by `'|'`.

    A name must not start with a `'-'` and must not be empty.

- `descr`

    A short description of the new option. This is only used when generating help messages.

- `parser`

    The parser is responsible for converting the string representation of the command line argument
    to objects of type `T`.

    This object is copied or moved into the new option.

    Requirements for parsers are described in more detail below.

- `flags...` (optional)

    Flags control how an option might be specified on the command line. The `CmdLine` parser uses
    the flags to split a command line argument into a name and an argument and then calls the
    option parser to convert the string.

The returned option is of type `Option<std::decay_t<Parser>>`.

##### `MakeUniqueOption(char const* name, char const* descr, Parser&& parser, Flags&&... flags)`

Works exactly like `MakeOption` above. But instead of returning a new `Option` by value, this
function returns a `std::unique_ptr` containing a heap-allocated `Option`.

The return value is of type `std::unique_ptr<Option<std::decay_t<Parser>>>`.

#### Flags

The following flags may be specified as additional arguments to `Make[Unique]Option` to customize
how the option (and possibly its argument) may be specified on the command line.

They may be specified in any order and any combination. _But note that not all combinations might
make sense!_

##### `enum class NumOpts`

Controls how often an option may/must be specified.

- `optional`:
    The option may appear at most once. This is the **default**.
    ```c++
    cmd.Add("f", "bool", cl::Assign(f), cl::NumOpts::optional);
        // OK:    ""       ==>  f == <initial value>
        // OK:    "-f"     ==>  f == true
        // Fail:  "-f -f"
    ```

- `required`:
    The option must appear exactly once.
    ```c++
    cmd.Add("f", "bool", cl::Assign(f), cl::NumOpts::required);
        // Fail:  ""
        // OK:    "-f"     ==>  f == true
        // Fail:  "-f -f"
    ```

- `zero_or_more`:
    The option may appear multiple times.
    ```c++
    bool f = false;
    cmd.Add("f", "bool", cl::Assign(f), cl::NumOpts::zero_or_more);
        // OK:  ""       ==>  f == false (i.e., initial value)
        // OK:  "-f"     ==>  f == true
        // OK:  "-f -f"  ==>  f == true
    ```

- `one_or_more`:
    The option must appear at least once.
    ```c++
    cmd.Add("f", "bool", cl::Assign(f), cl::NumOpts::one_or_more);
        // Fail:  ""
        // OK:    "-f"     ==>  f == true
        // OK:    "-f -f"  ==>  f == true
    ```

##### `enum class HasArg`

Controls the number of arguments the option accepts.

- `no`:
    An argument is not allowed. This is the **default**.
    ```c++
    cmd.Add("f", "bool", cl::Assign(f), cl::HasArg::no);
        // OK:    "-f"     ==>  f == true
        // Fail:  "-f=on"
    ```

- `optional`:
    An argument is optional.
    ```c++
    cmd.Add("f", "bool", cl::Assign(f), cl::HasArg::optional);
        // OK:  "-f"        ==>  f == true
        // OK:  "-f=false"  ==>  f == false
    ```

- `required`:
    An argument is required. If this flag is set, the parser may steal arguments from the command
    line if none is provided using the `=` syntax.
    ```c++
    cmd.Add("f", "bool", cl::Assign(f), cl::HasArg::required);
        // Fail:  "-f"
        // OK:    "-f=1"    ==>  f = true
        // OK:    "-f yes"  ==>  f = true
    ```

##### `enum class JoinArg`

Controls whether the option may/must join its argument.

- `no`:
    The option must not join its argument: `"-I dir"` and `"-I=dir"` possible. If the option is
    specified with an equals sign (`"-I=dir"`) the `'='` will _not_ be part of the option argument.
    This is the **default**.
    ```c++
    cmd.Add("I", "Include directory", cl::Assign(I), cl::HasArg::required, cl::JoinArg::no);
        // Fail:  "-I"
        // OK:    "-I dir"  ==>  I == "dir"
        // OK:    "-I=dir"  ==>  I == "dir"
    ```

- `optional`:
    The option may join its argument: `"-I dir"` and `"-Idir"` are possible. If the option is
    specified with an equals sign (`"-I=dir"`) the `'='` will be part of the option argument.
    ```c++
    cmd.Add("I", "Include directory", cl::Assign(I), cl::HasArg::required, cl::JoinArg::optional);
        // Fail:  "-I"
        // OK:    "-I dir"  ==>  I == "dir"
        // OK:    "-I=dir"  ==>  I == "=dir"
    ```

- `yes`:
    The option must join its argument: `"-Idir"` is the only possible format. If the option is
    specified with an equals sign (`"-I=dir"`) the `'='` will be part of the option argument.
    ```c++
    cmd.Add("I", "Include directory", cl::Assign(I), cl::HasArg::required, cl::JoinArg::yes);
        // Fail:  "-I"
        // Fail:  "-I dir"
        // OK:    "-I=dir"  ==>  I == "=dir"
        // OK:    "-Idir"   ==>  I == "dir"
    ```

##### `enum class MayGroup`

May this option group with other options?

- `no`:
    The option may not be grouped with other options (even if the option name consists only of a
    single letter). This is the **default**.
    ```c++
    cmd.Add("x", "", cl::Assign(x), cl::HasArg::no, cl::MayGroup::no);
    cmd.Add("y", "", cl::Assign(y), cl::HasArg::no, cl::MayGroup::no);
        // OK:    "-x -y"  ==>  x == true, y == true
        // Fail:  "-xy"
    ```

- `yes`:
    The option may be grouped with other options. This flag is ignored if the names of the options
    are not a single letter and option groups must be prefixed with a single `'-'`, e.g.
    `"-xvf=file"`.
    ```c++
    cmd.Add("x", "", cl::Assign(x), cl::HasArg::no, cl::MayGroup::yes);
    cmd.Add("y", "", cl::Assign(y), cl::HasArg::no, cl::MayGroup::yes);
        // OK:    "-x -y"  ==>  x == true, y == true
        // OK:    "-xy"    ==>  x == true, y == true
    ```

##### `enum class Positional`

Positional option?

- `no`:
    The option is not a positional option, i.e. requires `'-'` or `'--'` as a prefix when specified.
    This is the **default**.
    ```c++
    cmd.Add("input-file", "", cl::Assign(file), cl::HasArg::required, cl::Positional::no);
        // OK:    "--input-file=FILE"  ==>  file == "FILE"
        // OK:    "--input-file FILE"  ==>  file == "FILE"
        // Fail:  "FILE"
    ```
- `yes`:
    Positional option, no `'-'` required.
    ```c++
    cmd.Add("input-file", "", cl::Assign(file), cl::HasArg::yes, cl::Positional::yes);
        // OK:    "--input-file=FILE"  ==>  file == "FILE"
        // OK:    "--input-file FILE"  ==>  file == "FILE"
        // OK:    "FILE"               ==>  file == "FILE"
    ```

    Multiple positional options are allowed. They are processed in the order as they are attached to
    the parser until they no longer accept any further occurrence. E.g.
    ```c++
    cmd.Add("pos1", "", cl::Assign(pos1), cl::HasArg::yes, cl::NumOpts::optional, cl::Positional::yes);
    cmd.Add("pos2", "", cl::PushBack(pos2), cl::HasArg::yes, cl::NumOpts::zero_or_more, cl::Positional::yes);
        // OK:  "eins"            ==>  pos1 == "eins", pos2 == {}
        // OK:  "eins zwei drei"  ==>  pos1 == "eins", pos2 == {"zwei", "drei"}
    ```

##### `enum class CommaSeparated`

Split the argument between commas?

- `no`:
    Do not split the argument between commas. This is the **default**.
    ```c++
    cmd.Add("i", "ints", cl::PushBack(ints), cl::HasArg::required, cl::NumOpts::zero_or_more, cl::CommaSeparated::no);
        // OK:    "-i=1 -i=2"  ==>  ints = {1,2}
        // Fail:  "-i=1,2"
    ```

- `yes`:
    If this flag is set, the option's argument is split between commas, e.g. `"-i=1,2,,3"` will be
    parsed as `["-i=1", "-i=2", "-i=", "-i=3"]`. Note that each comma-separated argument counts as
    an option occurrence.
    ```c++
    cmd.Add("i", "ints", cl::PushBack(ints), cl::HasArg::required, cl::NumOpts::zero_or_more, cl::CommaSeparated::no);
        // OK:    "-i=1 -i=2"  ==>  ints = {1,2}
        // OK:    "-i=1,2"     ==>  ints = {1,2}
        // Fail:  "-i=1,2,,3"  ==>  ints = {1,2}, but the empty string '' is not a valid integer
    ```

##### `enum class EndsOptions`

Parse all following options as positional options?

- `no`:
    Nothing special. This is the **default**.

- `yes`:
    If an option with this flag is (successfully) parsed, all the remaining options are parsed as
    positional options.
    ```c++
    TODO
    ```

#### Parsers

While `Flags` control how an option may be specified on the command line, the option parser is
responsible for doing something useful with the command line argument.

In general, an option parser is a function or a function object with the following signature

    (ParseContext [const]& ctx) -> bool

The `ParseContext` struct contains the following members:

- `string_view name`

    Name of the option being parsed (only valid in callback!). The `CmdLine` parser uses the options
    flags to decompose a command line argument into an option name and possibly an option argument.

- `string_view arg`

    The option argument (only valid in callback!).

- `int index`

    Current index in the argv array. This might be useful if the meaning of options is
    order-dependent.

- `Cmdline* cmdline`

    The command line parser which currently parses the argument list. This pointer is always valid
    (never null) and can be used, e.g., to emit additional errors and/or warnings.

Given such a `ParseContext` object, the parser is responsible for doing something useful with the
given information. This might include just printing the name and the argument to `stdout` or
converting the argument into objects of type `T`.

The latter seems slightly more useful in general. So the CmdLine library provides some utility
classes and methods to easily convert strings into objects of other types and assigning the result
to user-provided storage.

##### Default parsers

- `class ConvertTo<T>`

    The member function

        bool operator()(string_view in, T& out) [const];

    is responsible for converting strings into objects of type `T`. The function returns `true` on
    success, `false` otherwise.

    The library provides some default specializations:

    - `bool`

        Maps strings to boolean `true` or `false` as defined in the following table:

        - `true` values: `""` (empty argument), `"true"`, `"1"`, `"on"`, `"yes"`
        - `false` values: `"false"`, `"0"`, `"off"`, `"no"`

        Other inputs result in parsing errors.

    - integral types `intN_t, uintN_t`

        Uses `strto[u]ll` to convert the string to an integer and performs a range check using the
        limits of the actual integral type.
        Returns `true` if parsing and the range check succeed, returns `false` otherwise.

    - floating-point types `float`, `double`, `long double`

        Uses `strtof`, `strtod` and `strtold` resp. to parse the floating-point value.
        Returns `false` if the functions set `errno` to `ERANGE`, returns `true` otherwise.

    - `std::pair<T1, T2>`

        Splits the input string at the first occurrence of `':'` into the two substrings `in1` and
        `in2` and uses `ConvertTo<T1>` and `ConvertTo<T2>` to parse the first and second part, resp.

        This allows to parse key-value pairs directly into, e.g., a `std::map`:
        ```c++
        std::map<std::string, int> map;

        cmd.Add("kv", "Key-Value pairs", cl::PushBack(map), cl::NumOpts::zero_or_more, cl::HasArg::required);

            // "-kv eins:1 -kv=zwei:2"  ==>  map == {{"eins", 1}, {"zwei", 2}}
        ```

    The generic implementation uses `std::stringstream` to convert the string into an object of type
    `T`. So if your type has a compatible stream extraction `operator>>` defined, you should be able
    to directly parse options into objects of your custom data type without any additional effort.

    **Note**: Using the default implementation, requires to additionally `#include <sstream>`.

    On the other hand you may want to specialize `ConvertTo` for your custom data type.

- `class ParseValue<T>`

        bool operator()(ParseContext& ctx, T& out) [const];

    Calls `ConvertTo<T>::operator()(ctx.arg, out)` (see below).

    This is the default parser used by all other option parsers defined in the CmdLine library.
    Instead of specializing `ConvertTo<T>` for your custom data type `T` you might as well
    specialize `ParseValue<T>`.

    This might be useful if the value of the option depends on the name of the option as specified
    on the command line. E.g., a flag parser might convert a command line argument to a boolean
    value depending on whether the option was specified as `"--debug"` or `"--no-debug"`.

For convenience, the CmdLine library provides some predefined parsers for the most common use cases.

- `Assign(T& target, UnaryFunctions... fns)`

    Calls `ParseValue` to convert the string into a temporary object `v` of type `T`, then calls
    each `fns` for the result and if all `fns` return `true`, move-assigns `v` to `target`.
    ```c++
    cmd.Add("i", "int", cl::Assign(i), cl::NumOpts::zero_or_more, cl::HasArg::required);
        // "-i=1"       ==>  i == 1
        // "-i 1 -i=2"  ==>  i == 2
    ```

- `PushBack(T& target, UnaryFunctions... fns)`

    Calls `ParseValue` to convert the string into a temporary object `v` of type `T::value_type`,
    then calls each `fns` for the result and if all `fns` return `true`, calls
    `target.insert(target.end(), std::move(v))`.
    ```c++
    cmd.Add("i", "vector<int>", cl::PushBack(i), cl::NumOpts::zero_or_more, cl::HasArg::required);
        // "-i=1"       ==>  i == {1}
        // "-i=1 -i 2"  ==>  i == {1,2}
    ```

- `Map(T& target, std::initializer_list<char const*, T>> map, UnaryFunctions... fns)`

    The returned parser converts strings to objects of type `T` by simply looking them up in the
    given map. This might be useful to parse enum types:
    ```c++
    Simpson { Homer, Marge, Bart, Lisa, Maggie };

    Simpson simpson = Simpson::Homer;

    cmd.Add("simpson", "The name of a Simpson",
        cl::Map(simpson, {{"Homer",    Simpson::Homer },
                          {"Marge",    Simpson::Marge },
                          {"Bart",     Simpson::Bart  },
                          {"El Barto", Simpson::Bart  },
                          {"Lisa",     Simpson::Lisa  },
                          {"Maggie",   Simpson::Maggie}}), cl::NumOpts::optional, cl::HasArg::required);
        // OK:    "-simpson=Bart"      ==>  simpson == Simpson::Bart
        // OK:    "-simpson=El Barto"  ==>  simpson == Simpson::Bart
        // Fail:  "-simpson Granpa"
    ```

All of the functions defined above take an arbitrary number of additional _predicates_, which can be
used to check if a given value satisfies various conditions. These predicates must have the
following signature:

    (ParseContext [const]& ctx, T [const]& value) -> bool

_Note_: The `value` argument might be accepted by non-const reference, in which case the _predicate_
might actually transform the passed value.

The library provides some predicates in the `cl::check` namespace:

- `check::InRange(T lower, U upper)`

    Returns a function object which checks whether a given value is in the range `[lower, upper]`,
    i.e. returns `!(value < lower) && !(upper < value)`.
    ```c++
    cl::Assign(small_int, cl::InRange(0, 15))
        // In addition to converting the input string into an integer, the function object returned
        // from the function call above additionally checks whether the resuling integer is in the
        // range [0,15].
    ```

- `check::GreaterThan(T lower)`

    Returns a function object which checks whether a given value is greater than `lower`, i.e.
    returns `lower < value`.

- `check::GreaterEqual(T lower)`

    Returns a function object which checks whether a given value is greater than or equal to `lower`,
    i.e. returns `!(value < lower)`.

- `check::LessThan(T upper)`

    Returns a function object which checks whether a given value is less than `upper`, i.e. returns
    `value < upper`.

- `check::LessEqual(T upper)`

    Returns a function object which checks whether a given value is less than or equal to `upper`,
    i.e. returns `!(upper < value)`.

- **XXX** (not implemented) `check::FileExists()`

### class `Cmdline`

#### `Add`

The `Add` method can be used to create new options and automatically attach them to the command-line
parser, or attach existing options to the command-line parser.

- `Add(char const* name, char const* descr, Parser&& parser, Args&&... args)`

    This overload creates a new option (using `MakeUniqueOption`) from the given arguments and
    attaches the option to the command-line parser. The command-line parser takes ownership of the
    new option.

- `Add(OptionBase* option)`

    Attaches an existing option to the command-line parser.

#### `Parse`

- `Parse(It first, It last)`

    ...

- `Parse(It first, It last, Sink sink)`

    This is like the `Parse` function shown above. The difference is that instead of generating an
    error for unknown command line arguments, this method calls `sink` with the unknown argument as
    a parameter. This allows the user to ignore unknown arguments and continue parsing.

    Note, however, that ignoring unknown arguments might screw up argument parsing.
    So use with caution.

#### `ParseArgs`

- `ParseArgs(List const& args)`

    Calls `Parse(args.begin(), args.end())`.

- `ParseArgs(List const& args, Sink sink)`

    Calls `Parse(args.begin(), args.end(), std::move(sink))`.

#### `PrintDiag`

...

#### `PrintHelp`

...

### Utility functions

#### `TokenizeUnix(string_view) -> vector<string>`

    Tokenize a string into command line arguments using `bash`-style escaping.

#### `TokenizeWindows(string_view) -> vector<string>`

    Tokenize a string into command line arguments using Windows-style escaping.

    See [CommandLineToArgvW](https://msdn.microsoft.com/en-us/library/windows/desktop/17w5ykft(v=vs.85).aspx)
    for details.

## **TODO**

- Response file

- Wildcard expansion
