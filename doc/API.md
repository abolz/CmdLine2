# class `Option<P>`

## Base class `OptionBase`

...

## Creating options

```c++
Option<std::decay_t<Parser>> MakeOption(char const* name, char const* descr, Parser parser, Flags... flags)
```

This function creates a new `Option`, which might be attached to a `CmdLine`
object (see below).

* `name`
    <br/> The name of the new option. This is used to identify the option on the
    command-line. Multiple option names are allowed: `name` can be a list of
    option names separated by `'|'`.

    A name must not start with a `'-'` and must not be empty.

* `descr`
    <br/> A short description of the new option. This is only used when
    generating help messages.

* `parser`
    <br/> The parser is responsible for converting the string representation of
    the command line argument to objects of type `T`. This object is copied or
    moved into the new option.

    Requirements for parsers are described in more detail below.

* `flags...` (optional)
    <br/> Flags control how an option might be specified on the command line.
    The `CmdLine` parser uses the flags to split a command line argument into a
    name and an argument and then calls the option parser to convert the string.

```c++
std::unique_ptr<Option<std::decay_t<Parser>>> MakeUniqueOption(char const* name, char const* descr, Parser parser, Flags... flags)
```

Works exactly like `MakeOption` above. But instead of returning a new `Option`
by value, this function returns a `std::unique_ptr` containing a heap-allocated
`Option`.

```c++
// Using class template argument deduction (C++17)
Option(char const* name, char const* descr, Parser&& parser, Flags&&... flags)
  -> Option<std::decay_t<Parser>>
```

## Flags

The following flags may be specified as additional arguments to
`Make[Unique]Option` to customize how the option (and possibly its argument) may
be specified on the command line.

They may be specified in any order and any combination. _But note that not all
combinations might make sense!_

### enum class `NumOpts`

Controls how often an option may/must be specified.

* `optional`
    <br/> The option may appear at most once. This is the **default**.
    ```c++
    cmd.Add("f", "bool", cl::Assign(f), cl::NumOpts::optional);
    // ""       ==>  f = <initial value>
    // "-f"     ==>  f = true
    // "-f -f"  ==>  Error
    ```

* `required`
    <br/> The option must appear exactly once.
    ```c++
    cmd.Add("f", "bool", cl::Assign(f), cl::NumOpts::required);
    // ""       ==>  Error
    // "-f"     ==>  f = true
    // "-f -f"  ==>  Error
    ```

* `zero_or_more`
    <br/> The option may appear multiple times.
    ```c++
    bool f = false;
    cmd.Add("f", "bool", cl::Assign(f), cl::NumOpts::zero_or_more);
    // ""       ==>  f = false (i.e., initial value)
    // "-f"     ==>  f = true
    // "-f -f"  ==>  f = true
    ```

* `one_or_more`
    <br/> The option must appear at least once.
    ```c++
    cmd.Add("f", "bool", cl::Assign(f), cl::NumOpts::one_or_more);
    // ""       ==>  Error
    // "-f"     ==>  f = true
    // "-f -f"  ==>  f = true
    ```

### enum class `HasArg`

Controls the number of arguments the option accepts.

* `no`
    <br/> An argument is not allowed. This is the **default**.
    ```c++
    cmd.Add("f", "bool", cl::Assign(f), cl::HasArg::no);
    // "-f"     ==>  f = true
    // "-f=on"  ==>  Error
    ```

* `optional`
    <br/> An argument is optional.
    ```c++
    cmd.Add("f", "bool", cl::Assign(f), cl::HasArg::optional);
    // "-f"        ==>  f = true
    // "-f=false"  ==>  f = false
    ```

* `required`
    <br/> An argument is required. If this flag is set, the parser may steal
    arguments from the command line if none is provided using the `=` syntax.
    ```c++
    cmd.Add("f", "bool", cl::Assign(f), cl::HasArg::required);
    // "-f"      ==>  Error
    // "-f=1"    ==>  f = true
    // "-f yes"  ==>  f = true
    ```

### enum class `JoinArg`

Controls whether the option may/must join its argument.

* `no`
    <br/> The option must not join its argument: `"-I dir"` and `"-I=dir"`
    possible. If the option is specified with an equals sign (`"-I=dir"`) the
    `'='` will _not_ be part of the option argument. This is the **default**.
    ```c++
    cmd.Add("I", "Include directory", cl::Assign(I), cl::HasArg::required, cl::JoinArg::no);
    // "-I"      ==>  Error
    // "-I dir"  ==>  I = "dir"
    // "-I=dir"  ==>  I = "dir"
    ```

* `optional`
    <br/> The option may join its argument: `"-I dir"` and `"-Idir"` are
    possible. If the option is specified with an equals sign (`"-I=dir"`) the
    `'='` will be part of the option argument.
    ```c++
    cmd.Add("I", "Include directory", cl::Assign(I), cl::HasArg::required, cl::JoinArg::optional);
    // "-I"      ==>  Error
    // "-I dir"  ==>  I = "dir"
    // "-I=dir"  ==>  I = "=dir"
    ```

* `yes`
    <br/> The option must join its argument: `"-Idir"` is the only possible
    format. If the option is specified with an equals sign (`"-I=dir"`) the
    `'='` will be part of the option argument.
    ```c++
    cmd.Add("I", "Include directory", cl::Assign(I), cl::HasArg::required, cl::JoinArg::yes);
    // "-I"      ==>  Error
    // "-I dir"  ==>  Error
    // "-I=dir"  ==>  I = "=dir"
    // "-Idir"   ==>  I = "dir"
    ```

### enum class `MayGroup`

May this option group with other options?

* `no`
    <br/> The option may not be grouped with other options (even if the option
    name consists only of a single letter). This is the **default**.
    ```c++
    cmd.Add("x", "", cl::Assign(x), cl::HasArg::no, cl::MayGroup::no);
    cmd.Add("y", "", cl::Assign(y), cl::HasArg::no, cl::MayGroup::no);
    // "-x -y"  ==>  x = true, y = true
    // "-xy"    ==>  Error
    ```

* `yes`
    <br/> The option may be grouped with other options. This flag is ignored if
    the name of the option is not a single letter and option groups must be
    prefixed with a single `'-'`, e.g. `"-xvf=file"`.
    ```c++
    cmd.Add("x", "", cl::Assign(x), cl::HasArg::no, cl::MayGroup::yes);
    cmd.Add("y", "", cl::Assign(y), cl::HasArg::no, cl::MayGroup::yes);
    // "-x -y"  ==>  x = true, y = true
    // "-xy"    ==>  x = true, y = true
    // "--xy"   ==>  Error
    ```

### enum class `Positional`

Positional option?

* `no`
    <br/> The option is not a positional option, i.e. requires `'-'` or `'--'`
    as a prefix when specified. This is the **default**.
    ```c++
    cmd.Add("input-file", "", cl::Assign(file), cl::HasArg::required, cl::Positional::no);
    // "--input-file FILE"  ==>  file = "FILE"
    // "FILE"               ==>  Error
    ```

* `yes`
    <br/> Positional option, no `'-'` required.
    ```c++
    cmd.Add("input-file", "", cl::Assign(file), cl::HasArg::yes, cl::Positional::yes);
    // "--input-file=FILE"  ==>  file = "FILE"
    // "FILE"               ==>  file = "FILE"
    ```
    Multiple positional options are allowed. They are processed in the order as
    they are attached to the parser until they no longer accept any further
    occurrence. E.g.
    ```c++
    cmd.Add("pos1", "", cl::Assign(pos1), cl::HasArg::required, cl::NumOpts::optional, cl::Positional::yes);
    cmd.Add("pos2", "", cl::PushBack(pos2), cl::HasArg::required, cl::NumOpts::zero_or_more, cl::Positional::yes);
    // "eins"            ==>  pos1 = "eins", pos2 = {}
    // "eins zwei drei"  ==>  pos1 = "eins", pos2 = {"zwei", "drei"}
    ```

### enum class `CommaSeparated`

Split the argument between commas?

* `no`
    <br/> Do not split the argument between commas. This is the **default**.
    ```c++
    cmd.Add("i", "ints", cl::PushBack(ints), cl::HasArg::required, cl::NumOpts::zero_or_more, cl::CommaSeparated::no);
    // "-i=1 -i=2"  ==>  ints = {1,2}
    // "-i=1,2"     ==>  Error
    ```

* `yes`
    <br/> If this flag is set, the option's argument is split between commas,
    e.g. `"-i=1,2,,3"` will be parsed as `["-i=1", "-i=2", "-i=", "-i=3"]`. Note
    that each comma-separated argument counts as an option occurrence.
    ```c++
    cmd.Add("i", "ints", cl::PushBack(ints), cl::HasArg::required, cl::NumOpts::zero_or_more,  cl::CommaSeparated::no);
    // "-i=1 -i=2"  ==>  ints = {1,2}
    // "-i=1,2"     ==>  ints = {1,2}
    // "-i=1,2,,3"  ==>  Error; i.e. ints = {1,2}, but the empty string '' is not a valid integer
    ```

### enum class `StopParsing`

Stop parsing early?

* `no`
    <br/> Nothing special. This is the **default**.

* `yes`
    <br/> If an option with this flag is (successfully) parsed, all the
    remaining command line arguments are ignored and the parser returns
    immediately.
    ```c++
    cmd.Add("a", "", cl::Var(a), cl::HasArg::optional);
    cmd.Add("command", "", cl::Var(command), cl::Positional::yes, cl::HasArg::yes, cl::StopParsing::yes, cl::NumOpts::required);
    // argv[] = {"-a=123", "add", "-b", "-c"};
    // result = cmd.Parse(begin(argv), end(argv));
    //  ==> a           = 123
    //  ==> command     = "add"
    //  ==> res.success = true
    //  ==> res.next    = arg.begin() + 1
    ```

## Parsers

While `Flags` control how an option may be specified on the command line, the
option parser is responsible for doing something useful with the command line
argument.

In general, an option parser is a function or a function object with the
following signature
```c++
bool (ParseContext [const]& ctx)
```
which gets called whenever an option is parsed from the command line.

The `ParseContext` struct contains the following members:

* `string_view name`
    <br/> Name of the option being parsed (only valid in callback!). The
    `CmdLine` parser uses the options flags to decompose a command line argument
    into an option name and possibly an option argument.

* `string_view arg`
    <br/> The option argument (only valid in callback!).

* `int index`
    <br/> Current index in the argv array. This might be useful if the meaning
    of options is order-dependent.

* `Cmdline* cmdline`
    <br/> The command line parser which currently parses the argument list. This
    pointer is always valid (never null) and can be used, e.g., to emit
    additional errors and/or warnings.

Given such a `ParseContext` object, the parser is responsible for doing
something useful with the given information. This might include just printing
the name and the argument to `stdout` or converting the argument into objects of
type `T`.

The latter seems slightly more useful in general. So the CmdLine library
provides some utility classes and methods to easily convert strings into objects
of other types and assigning the result to user-provided storage.

---
```c++
template <typename T, typename = void>
struct ConvertTo {
    bool operator()(string_view in, T& out) const;
};
```

is responsible for converting strings into objects of type `T`. The function
returns `true` on success, `false` otherwise.

The library provides some specializations:

* `bool`

    Maps strings to boolean `true` or `false` as defined in the following table:
    * `true` values: `""` (empty string), `"true"`, `"1"`, `"on"`, `"yes"`
    * `false` values: `"false"`, `"0"`, `"off"`, `"no"`
    Other inputs result in parsing errors.

* `intN_t, uintN_t` (integral types)

    Uses `strto[u]ll` to convert the string to an integer and performs a range
    check using the limits of the actual integral type. Returns `true` if
    parsing and the range check succeed, returns `false` otherwise.

* `float`, `double`, `long double`

    Use the `strtod` family of functions to parse the floating-point value.
    Returns `false` if the functions set `errno` to `ERANGE`, returns `true`
    otherwise.

The default implementation uses `std::stringstream` to convert the string into
an object of type `T`. So if your type has a compatible stream extraction
`operator>>` defined, you should be able to directly parse options into objects
of your custom data type without any additional effort.

**Note**: Using the default implementation, requires to additionally
`#include <sstream>`.

---
```c++
template <typename T, typename = void>
struct ParseValue {
    bool operator()(ParseContext& ctx, T& out) const
    {
        ConvertTo<T> conv;
        return conv(ctx.arg, out);
    }
};
```

This is the default parser used by all other option parsers defined in the
CmdLine library.

Instead of specializing `ConvertTo<T>` for your data type `T`, you might as well
specialize `ParseValue<T>`.

This might be useful if the value of the option depends on the name of the
option as specified on the command line. E.g., a flag parser might convert a
command line argument to a boolean value depending on whether the option was
specified as `"--debug"` or `"--no-debug"`.

---
```c++
ParseFn Assign(T& target)
```

Calls `ParseValue` to convert the string into a temporary object `v` of type
`T`, then calls each `fns` for the result and if all `fns` return `true`,
move-assigns `v` to `target`.
```c++
cmd.Add("i", "int", cl::Assign(i), cl::NumOpts::zero_or_more, cl::HasArg::required);
// "-i=1"       ==>  i == 1
// "-i 1 -i=2"  ==>  i == 2
```

---
```c++
ParseFn PushBack(T& target)
```

Calls `ParseValue` to convert the string into a temporary object `v` of type
`T::value_type`, then calls each `fns` for the result and if all `fns` return
`true`, calls `target.insert(target.end(), std::move(v))`.
```c++
cmd.Add("i", "int list", cl::PushBack(i), cl::NumOpts::zero_or_more, cl::HasArg::required);
// "-i=1"       ==>  i == {1}
// "-i=1 -i 2"  ==>  i == {1,2}
```

---
```c++
ParseFn Map(T& target, std::initializer_list<char const*, T>> map)
```

The returned parser converts strings to objects of type `T` by simply looking
them up in the given map. This might be useful to parse enum types:
```c++
enum class Simpson { Homer, Marge, Bart, Lisa, Maggie };

Simpson simpson = Simpson::Homer;

cmd.Add("simpson", "The name of a Simpson",
    cl::Map(simpson, {{"Homer",    Simpson::Homer },
                      {"Marge",    Simpson::Marge },
                      {"Bart",     Simpson::Bart  },
                      {"El Barto", Simpson::Bart  },
                      {"Lisa",     Simpson::Lisa  },
                      {"Maggie",   Simpson::Maggie}}), cl::NumOpts::optional, cl::HasArg::required);
// "-simpson=Bart"      ==>  simpson == Simpson::Bart
// "-simpson=El Barto"  ==>  simpson == Simpson::Bart
// "-simpson Granpa"    ==>  Error
```

### Predicates

All of the functions defined above take an arbitrary number of additional
functions which can be used to check if a given value satisfies various
conditions.

These functions must have the following signature:
```c++
bool (ParseContext const& ctx, T [const]& value)
```

**Note**: The `value` argument might be accepted by non-const reference, in
which case the function might actually transform the passed value.

The library provides some predicates in the `cl::check` namespace:

* `check::InRange(T lower, U upper)`
    <br/> Returns a function object which checks whether a given value is in the
    range `[lower, upper]`.

* `check::GreaterThan(T lower)`
    <br/> Returns a function object which checks whether a given value is
    greater than `lower`, i.e. returns `lower < value`.

* `check::GreaterEqual(T lower)`
    <br/> Returns a function object which checks whether a given value is
    greater than or equal to `lower`, i.e. returns `!(value < lower)`.

* `check::LessThan(T upper)`
    <br/> Returns a function object which checks whether a given value is less
    than `upper`, i.e. returns `value < upper`.

* `check::LessEqual(T upper)`
    <br/> Returns a function object which checks whether a given value is less
    than or equal to `upper`, i.e. returns `!(upper < value)`.

```c++
cl::Assign(small_int, cl::check::InRange(0, 15))
// In addition to converting the input string into an integer, the function
// object returned from the function call above additionally checks whether the
// resuling integer is in the range [0,15].

cl::Assign(small_int, cl::check::GreaterEqual(0), cl::check::LessThan(16))
// The same as the example above using InRange
```

# class `Cmdline`

## Add options

The `Add` method can be used to create new options and automatically attach them
to the command-line parser, or attach existing options to the command-line
parser.

To create a new option, use
```c++
OptionBase* Add(char const* name, char const* descr, Parser parser, Flags... flags)
```
This overload creates a new option (using `MakeUniqueOption`) from the given
arguments and attaches the option to the command-line parser. The
command-line parser takes ownership of the new option.

The function returns a pointer to the newly created option of type
`Option<std::decay_t<Parser>>*`.

To attach an existing option to the command line parser, use
```c++
OptionBase* Add(std::unique_option<OptionBase> option)
```
or
```c++
OptionBase* Add(OptionBase* option)
```

**Note**: The life-time of `option` must exceed the life-time of the `CmdLine`
object.

## Parse command line arguments

```c++
struct ParseResult {
    InputIterator next;
    bool success;
};

ParseResult Parse(InputIterator first, InputIterator last, CheckMissingOptions check_missing = yes)
```

...

```c++
bool ParseArgs(Range const& args, CheckMissingOptions check_missing = yes)
```

...

## Diagnostics and help messages

...

# Utility functions

#### `TokenizeUnix(string_view str) -> vector<string>`

Tokenize a string into command line arguments using `bash`-style escaping.

#### `TokenizeWindows(string_view str) -> vector<string>`

Tokenize a string into command line arguments using Windows-style escaping. See
[CommandLineToArgvW](https://msdn.microsoft.com/en-us/library/windows/desktop/17w5ykft(v=vs.85).aspx)
for details.

#### `CommandLineToArgvUTF8(wchar_t const* command_line) -> vector<string>`

(This function is only available when compiling for the Windows platform.)

Like `TokenizeWindows`, but the null-terminated UTF-16 encoded input string is
first converted to UTF-8. This is intended to be used with the
[GetCommandLine](https://msdn.microsoft.com/en-us/library/windows/desktop/ms683156(v=vs.85).aspx) function

# Subcommands
