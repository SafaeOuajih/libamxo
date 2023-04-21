# Object Definition Language

[[_TOC_]]

# Introduction

The Object Definition Language provides a simple mean to define a data model.

A data model is a hierarchical tree of objects where each object can contain 0, 1 or more parameters and 0, 1 or more functions.

## Syntax Notation

Throughout this document odl syntax is provided in railroad [syntax diagrams](https://en.wikipedia.org/wiki/Syntax_diagram). In these diagrams you start reading from left to right and just follow the "rails".

Blocks with rounded corners are called "terminals", what is in that block just needs to be copied, except when it starts with `<` and ends with `>` . Blocks with straight corners are called "non-terminals" (note that the names of these are always in capitals), there will be another diagram for that block.

In some "terminals" the content in the block is between `<` and `>` characters. Read the correct topic in [Appendix A - Syntax Overview](#appendix-a-syntax-overview) for more information about these blocks.

Whitespaces must be put between each block, except when the block only contains a single symbol like `%`, `!`, `&`, `#`, `?`.

### Example Of Railroad Diagram

![Include Syntax](doc/railroad/include.svg "include")<br>

According to this diagram all of the following are valid `include` syntaxes:

```
include "my-file.odl";
include 'my-file.odl';
#include 'my-file.odl';
&include "my-file.odl";
?include 'my-file.odl':"backup-file.odl";
```

***
> **NOTE**<br>
> In this example diagram some blocks mention `<FILE>` as content of that block. So any valid file system path ending with a valid file name can be used here. For more information read [\<FILE\>](#file) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview)
***

## Comments

In an odl file it is possible to write comments.

- Single line comments start with `//` and span till the end of the line (newline)

- Multi-line comments start with `/*` and end with `*/`

Comments can be used at any place in the odl file.

## Using Configuration Options and Environment Variables.

In an odl file it is allowed to use the defined config options at many places. Instead of using a fixed string, it can be replaced by `${<NAME>}`, in this notation the name references a configuration option. If no configuration option exists with the given name it is replaced with an empty string.

The same can be done with environment variables, using the notation `$(<NAME>)`, in this notation the name references a environment variable. If no environment variable exists with the given name it is replaced with an empty string.

Configuration options or environment variables can be used within a string to make it partially configurable, e.g.: `${prefix_}LocalData`, when the configuration option `prefix_` is defined and has the value `X_RPL_COM_` the result string will be `X_PRPL_COM_LocalData`.

>**Example**
> ```
> %config {
>     prefix_ = "X_PRPL_COM_";
>     myinclude = "test.odl";
> }
>
> include "${myinclude}";
>
> %define {
>     object MQTT {
>         object "${prefix_}Extra" {
>
>         }
>     }
> }
> ```

In above example the file `test.odl` will be included.  The object `MQTT.X_PRPL_COM_Extra` will be defined.

***
>**NOTE**  
>Using undefined configuration options or environment variables in the odl file could lead to some failures during parsing of the odl file. 
>Undefined configuration options or environment variables will be replaced with an empty string.
>
>It is also recommended to put fields using configuration options or environment variables between quotes (single or double)
***

# ODL Files

ODL (Object Definition Language) files are files that can define a data model and bind function implementations to these definitions. These definitions make it easy to define [BBF TR181](https://usp-data-models.broadband-forum.org/tr-181-2-15-1-usp.html) like data models in just minutes. When using the ambiorix tools and libraries it can be registered to different bus systems or use different protocols to make the defined data model(s) accessible to other applications (on the same host or remote).

Ambiorix uses a [Bus Agnostic API](https://gitlab.com/prpl-foundation/components/ambiorix/libraries/libamxb) to make it possible to create services and applications on top of different bus systems and protocols. Besides the Bus Agnostic Library for each supported bus system or protocol a specific back-end must be available.

At the moment of writing the following bus systems and protocols can be used:

- [ubus](https://gitlab.com/prpl-foundation/components/ambiorix/modules/amxb_backends/amxb_ubus).
- [pcb](https://gitlab.com/prpl-foundation/components/ambiorix/modules/amxb_backends/amxb_pcb) (SoftAtHome proprietary bus system).
- [USP](https://gitlab.com/prpl-foundation/components/ambiorix/modules/amxb_backends/amxb_usp)/IMTP - [USP](https://gitlab.com/prpl-foundation/components/ambiorix/modules/amxb_backends/amxb_usp)/MQTT.
- [RBus](https://gitlab.com/prpl-foundation/components/ambiorix/modules/amxb_backends/amxb_rbus) (in development, check the gitlab repository for the current state).
- [DBus](https://gitlab.com/prpl-foundation/components/ambiorix/modules/amxb_backends/amxb_dbus) (in development, chech the gitlab repository for the current state).
- [JSONRPC](https://gitlab.com/prpl-foundation/components/ambiorix/modules/amxb_backends/amxb_jsonrpc) (currently not opensourced).

An ODL file is a structured text file. At the top level the following elements can be used:

- [include](#include) 
- [import](#import)
- [requires](#requires)
- [config](#config)
- [define](#define)
- [populate](#populate)

![ODL Syntax](doc/railroad/odl.svg "odl")<br>

# Include

With `include`, `#include`, `&include` or `?include` other odl files or directories containing odl files can be included. Parsing of the include file is done first - except when using `&include` - before continuing the current odl file (that contains the include).

Mandatory includes are specified with `include` or `&include`, optional includes with `#include`. When an optional include file or directory is not available, parsing of the current odl continues. When a mandatory include file or directory is not available, parsing stops with an error.

The conditional include `?include` takes two include files or directories separated with a `:`. When the first include file or directory is not found, the second file or directory will be loaded. If none of the files or directories exists, parsing stops with an error. If the first file is found, but is not a valid odl file, parsing stops with an error. If the first is a directory and contains an invalid odl file, parsing stops with an error.

When using `&include`, a check is done to verify that the file or directory exists, if the file or directory is not found, parsing stops with an error, otherwise the file or directory is added to a list of include files that need to be loaded. The file or directory included with `&include` will not be loaded immediately.

When post includes `&include` are loaded is depending on the application. When using the ambiorix run-time (`amxrt`) all post include files or directory (`&include`) are parsed after the `entry-points` with reason 0 (AMXO_START) are invoked. When the `entry-points` are called and all successful, all post include files and directories are loaded. After the post include files are loaded, the `entry-points` are called with reason 2 (AMXO_ODL_LOADED). This is mainly used when some initialization needs to be done before loading the default values. Typically a post include only contains a `%populate` section.

Includes can be done anywhere at top level in the odl file, outside a section (`%config`, `%define`, `%populate`).

The name of the file or directory must be put between quotes (single or double) and can contain an absolute path or relative path. When a relative path is specified (not starting with `/`), the file or directory is searched in the include dirs. (by default the current working directory). The include directories can be configured in the config option `include-dirs`.

The name of the odl file or directory may be fully or partially replaced with a configuration option or an environment variable.

Each `include` statement must be terminated with a `;`.

***
>**NOTE**<br>
>Recursive includes are not allowed.
>If file `A` includes file `B`, file `B` includes file `C` and file `C` includes `A` then the last include is invalid as it creates a recursive include.<br>
>When recursive includes are detected, parsing will stop with an error.
***

## Include Syntax

![Include Syntax](doc/railroad/include.svg "include")

For more information about `<FILE>` read [\<FILE\>](#file) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).
## Include Example

> ```odl
> %config {
>     name = "tr181-mqtt";    
> }
>
> #include "mod_sahtrace.odl";
> #include "${name}_extra.odl";
> include "${name}_definition.odl";
> ?include "${name}-save.odl:${name}-defaults.odl";
> ```

# Import

Throughout an odl file references to functions can be provided. The odl parser will try to resolve these function names to a function implementation. The parser gets help of function resolvers. Extra function resolvers can be registered to the odl parser. The odl parser already has 3 function resolvers, `auto` resolver (the default), the `function table` (ftab) resolver and the `import` resolver. The last one uses loaded shared objects (loaded with [dlopen](https://man7.org/linux/man-pages/man3/dlopen.3.html)) and tries to resolve the function names using [dlsym](https://man7.org/linux/man-pages/man3/dlsym.3.html).

With the `import` shared objects can be loaded. The loaded shared objects will be used by the `import resolver` to resolve function symbols in the loaded shared objects. An absolute or relative path may be provided together with the name of the shared object. When no path or a relative path is specified the shared object is searched in the import directories as specified by the `import-dirs` configuration section.

The `import` can be used anywhere at top level, between any sections. Make sure that shared objects are imported before using symbols from the shared object.

An alias for a shared object can be provided using `as "<NAME>"`

The `import` uses [`dlopen`](https://www.man7.org/linux/man-pages/man3/dlopen.3.html) to load the shared object, some of the attributes of `dlopen` can be specified:

- `RTLD_NOW` (from the linux man pages)
    > If this value is specified, or the environment variable LD_BIND_NOW is set to a nonempty string, all undefined symbols in the shared object are resolved before dlopen() returns.  If this cannot be done, an error is returned.

- `RTLD_GLOBAL` (from the linux man pages)
    > The symbols defined by this shared object will be made available for symbol resolution of subsequently loaded shared objects.

The default behavior, if no attributes are specified, is `RTLD_LAZY` (from the linux man pages)<br>
> Perform lazy binding.  Resolve symbols only as the code that references them is executed.  If the symbol is never referenced, then it is never resolved.  (Lazy binding is performed only for function references; references to variables are always immediately bound when the shared object is loaded.)

An extra attribute is defined `RTLD_NODELETE` which will not unload the loaded shared object when no symbols are used.

***
> NOTE
>
> - Shared objects of which no symbols are used, are unloaded after parsing the odl files, unless the flag `RTLD_NODELETE` is set on the import instruction.
> - Importing the same shared object multiple times (with different attributes), will have no effect. A shared object will be loaded only one time, all other imports of the same shared object are ignored.
***

An `alias` can be specified to make it easier to refer to the shared object. The `alias` can be used when defining the [entry-points](#define-entry-points) or can be used in [resolver instructions](#resolver-instructions).

The attributes are optional. Zero one or more of these attributes can be specified with the `import`.

The `import` must be terminated with a `;`.

Instead of specifying the shared object file name, the name may be replaced with a configuration option or an environment variable.

## Import Syntax

![Import Syntax](doc/railroad/import.svg "import")

For more information about `<FILE>` read [\<FILE\>](#file) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>
For more information about `<NAME>` read [\<NAME\>](#name) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

## Import Example

> ```odl
> import "greeter_plugin.so" as "greeter";
>
> %config {
>     name = "tr181-mqtt"; 
> }
>
> import "${name}.so" as "${name}";
> ```

# Print

Using the `print` instruction messages can be printed to `stdout`. The print instruction can be used anywhere at top level, between sections.

It can be used to print the values of configuration options, which can be helpfull in finding out what the current value of a configuration options is at that moment.

The `print` instruction must be terminated with a `;`.

## Print Syntax

![Print Syntax](doc/railroad/print.svg "print")

For more information about `<TEXT>` read [\<TEXT\>](#text) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

## Print Example

> **Example:**
> ```
> %config{
>     Option1 = "Value1";
> }
>
> print "Option1 = ${Option1}";
> ```

# Requires

Using the `requires` keyword a dependency to another part of the data model can be specified. The parser will not check if the required object is available, the object path will be put in a list of required object paths.

It is up to the application that uses the odl parser to check if all required objects are available and wait for them if needed.

When using the ambiorix runtime application (`amxrt`), it will check if the required objects are available and wait for them if they are not available. The ambiorix run time will register (build the loaded data model) only when all required objects are available, amxrt will only call the defined entry-points when all required objects are available. 

The `requires` instruction must be terminated with a `;`.

Only one object path can be specified with the `requires` instruction, multiple `requires` instructions may be used.

## Requires Sytax

![Requires Syntax](doc/railroad/requires.svg "requires")

The \<PATH\> refers to data model objects. For more information about `<PATH>` read [\<PATH\>](#path) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

## Requires Example

> ```
> requires "NetModel.Intf."; 
> ```

# Sections

There are 3 kinds of sections available in an odl file:

- `%config` - see [Section %config](#section-%25config)
- `%define` - see [Section %define](#section-%25define)
- `%populate`

Section rules:

- All sections are optional, so an odl file without any section is valid.
- Each section can be used multiple times in a single odl file.
- The different kind of sections can appear in any order.

***
> NOTE  
> - Although the order of the sections doesn't matter, it matters that objects and parameters are defined in a `%define` section, before they are used in the `%populate` section.
> - When using configuration options in \<NAME\> or \<TEXT\> also make sure they are defined before using them.
>
> Typically the first section used is the `%config` section.
***

## Section %config

In the config section values for configuration options can be specified.
There is no restriction on which configuration options are set. Which configuration options will be used all depends on the application or plug-in.

Each config section starts with `%config {` and ends with `}`.

Everything between the curly braces `{` and `}` is considered as the body of the config section.

Each `%config` section has a scope, the values of the `options` defined in a `%config` section are only kept for that scope.

The scope of a `%config` section starts where it is defined and ends at the end of the file where it was defined. When an `include` is encountered the current configuration is passed to the include file.

In other words, changes in the configuration options are only visible in the current file, and in its included files but never in `parent` files (an odl file that included this one).

If a configuration option must be passed to the top level the attribute `%global` can be put before the name of the option. This will make the new value globally visible.

Each configuration option must be terminated with `;`.

>Example of configuration option scope<br>
>Assume these two odl files, named `main.odl` and `included.odl`
> 
> **main.odl**:
> ```
> print "Start - Option1 = ${Option1}";
>
> %config{
>     Option1 = "Value1";
> }
>
> print "Before include - Option1 = ${Option1}";
>
> include "included.odl";
>
> print "After include - Option1 = ${Option1}";
> ```
> 
> **included.odl**:
>
> ```
> print "In included - Option1 = ${Option1}";
>
> %config {
>     Option1 = "ChangeValue";
> }
>
> print "End of included - Option1 = ${Option1}";
> ```

When parsing the `main.odl` file using an application that uses the odl parser you should get this output (if printing to stdout is available), here `amxrt` (AMbioriX RunTime) is used to parse the odl file.

```
$ amxrt main.odl
Start - Option1 =  (/home/sah4009/development/experiments/test_odls/main.odl@1)
Before include - Option1 = Value1 (/home/sah4009/development/experiments/test_odls/main.odl@7)
In included - Option1 = Value1 (/home/sah4009/development/experiments/test_odls/included.odl@1)
End of included - Option1 = ChangeValue (/home/sah4009/development/experiments/test_odls/included.odl@7)
After include - Option1 = Value1 (/home/sah4009/development/experiments/test_odls/main.odl@11)
```

Just before the end of `included.odl` the value of `Option1` is printed and is at that moment `ChangeValue`. When back in the `main.odl` after the include of `included.odl` the value of `Option` is back to the same value as before the include.


### Configuration Option Names

The names of configuration options can contain any character except space characters, but it is recommanded to only use 0 - 9, a - z, A - Z, _ and -. When using other characters it is recommended to put the name between quotes (single or double), to avoid conflicts or misinterpretation by the parser.

If a name contains dots (`.`), it referes to an element which is in a table or array. If the dots are part of the name it must be put between double and single quotes.

A configuration option name can not use references to other configuration options or environment variables.

>**Examples of configuration option names**
> ```
> %config {
>     Option1 = "MyValue"; 
>     "$Option2" = "MyValue";
>     "Option3.Sub1" = "MyValue";
>     "Option3.Sub2" = "MyValue";
>     "'Option.With.Dots'" = "MyValue";
> }
> ```

The above example defines 4 configuration options at top level:
- `Option1`
- `$Option2`
- `Option3` - which is a table
- `Option.With.Dots`

The table `Option3` will have to sub-values `Sub1` and `Sub2`. An alternative way to declare the table `Option3` is:

> ```
> %config {
>    Option3 = {
>        Sub1 = "MyValue",
>        Sub2 = "MyValue"
>    };
> }
> ```

### Configuration Option Values

A value can be any of the values as defined in [\<VALUE\>](#value).

The values in a table or an array can be again one of these types. In other words it is possible to add a table in an array or define complex configuration options. In tables and arrays each individual value must be terminated with a `,`, except the last one which is terminated with `}` or `]`.

***
> **NOTE**<br>
> When redefining a table or an array the full table or array is replaced.
>
> **Consider this example:**
> ```
> %config {
>     MyTable = {
>         MySubTable = {
>             MyOption1 = "Value1",
>             MyOption2 = "Value2"
>         },
>         MySubArray = [ 1,2,3 ]
>     };
> }
>
> %config {
>     MyTable = {
>         MySubTable2 = {
>             Option1 = true,
>             Option2 = false
>         },
>     };
> }
> ```
>
> The second `MyTable` definition will overide the first definition of `MyTable`. In other words the configuration option `MyTable.MySubTable` will not exist anymore, including all sub-values if any. Also the array `MySubArray` will not exist anymore.
>
> It is possible to change or add values in tables or arrays, by using the dot notation in the name.
>
> ```
> %config {
>     MyTable.MySubTable.MyOption2 = "ChangeValue";
>     MyTable.MySubArray.0 = 0;
>     MyTable.MySubArray.2 = 2;
>     MyTable.MySubTable.MyOption3 = "NewValue";
> }
> ```
>
> This will change the value of `MyOption2` in table `MySubTable` which is in `MyTable` to `ChangeValue`, and change the values at index `0` and `2` of array `MySubArray` which is in `MyTable`.
> The last line will add a new key to `MySubTable`.
***

### %config Syntax

![Config Syntax](doc/railroad/config.svg "config")<br>
**\<VALUE\>**<br>
![Value Syntax](doc/railroad/value.svg "value")<br>

For more information about `<NAME>` read [\<NAME\>](#name) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>
For more information about `<PATH>` read [\<PATH\>](#path) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>
For more information about `<VALUE>` read [\<VALUE\>](#value) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>
For more information about `<TEXT>` read [\<TEXT\>](#text) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>
For more information about `<NUMBER>` read [\<NUMBER\>](#number) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>
For more information about `<DATETIME>` read [\<DATETIME\>](#datetime) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

### %config Example

> **Example:**
>
> ```odl
> %config {
>     import-dbg = true;
>     message = "My welcome message";
>     %global auto-resolver-order = [ "import", "ftab", "*" ];
>     personal-message = "${message} to everyone who reads this";
> }
> ```

### ODL Parser Configuration Options

The parser itself uses the following configuration options:

- `include-dirs` - an array of include directories (quoted strings). Include files specified in the odl file with no path or a relative path are searched in these directories.
- `import-dirs` - an array of of import directories (quoted strings). Import files (plug-in shared objects) specified in the odl file with no path or a relative path are searched in these directories.
- `import-dbg` - boolean (true or false). Makes the import function resolver print more information to stderr.
- `silent` - boolean (true or false). When set to true no `import` errors or messages are printed, the `print` command that can be used between sections will also be silenced.
- `odl-import` - boolean (true or false). When set to true (when not defined, the default is true), the `import` will load the specified shared object file, when set to false no shared object specified with `import` are loaded.
- `auto-resolver-order` - an array of resolver names (quoted strings). The order specified in this list is the order the auto resolver invokes them to resolve a function symbols. This list can end with a `*` to indicate all other resolvers in no specified order. When the list is empty, the auto resolver will not resolve any symbol.
- `define-behavior` - is a key - value map, which can be used to change the behavior of the ODL parser when reading `%define` sections. The available keys are:
    - `existing-object` - possible values are "error" or "update", default behavior is "error"
    - `existing-parameter` - possible values are "error" or "update", default behavior is "error"
- `populate-behavior` - is a key - value map, which can be used to change the behavior of the ODL parser when reading `%populate` sections. The available keys are:
    - `unknown-parameter` - possible values are "error", "warning", "add", default behavior is "error". When set to "add" the parser will add the parameter to the object, even if it is not defined.
    - `duplicate-instance` - possible values are "error", "update", default behavior is "error". When set to "update" the parser will update the already added instance.
- `odl`
    - `buffer-size` - when saving in odl format (config or data model) a buffer is used to write in chunks of at least the specified buffer-size. When not defined a buffer size of 16K is used. The buffer is used to reduce the number of writes on the file system.

*****
> **NOTE**
>
> By default the parser will throw an error when defining multiple-times the same object or parameter. This behavior can be changed by setting `define-behavior.existing-object` and `define-behavior.existing-parameter` respectively to `update`. In that case it will update the object or parameter. It is possible to change the parameter `type` but it is not possible to change the object `type` (from singleton to multi-instance or the other way arround).
>
> By default the parser will throw an error when setting a parameter value in the `%populate` section for a non-defined parameter. This behavior can be changed by setting the `populate-behavior.unknown-parameter` to `add` which will then add the parameter using the value `type` as parameter type, no parameter attributes will be added, this behavior is not `recommended`. The setting can be changed to `warning` as well, the parser will then give a warning, but continues.
>
> By default the parser will throw an error when creating a duplicate instance (same index, name or key values). This behavior can be changed by setting `populate-behavior.duplicate-instance` to `update`. In this case the parser will update the instance. Updating an instance can also be done by `selecting` the object and changing the `parameter` values.
***

## Section %define

In the `%define` section `mibs` (modular information blocks), the data model `objects` and `entry points` are defined.
Objects can contain parameters, functions or other objects.

Each define section starts with `%define {` and ends with `}`.

Everything between the curly braces `{` and `}` is considered as the body of the define section.

In a `%define` section a hierarchical object tree can be defined. Such a hierarchical object tree is also known as a `data model`. It is possible to define a part of the TR181 data model as described in the [tr181 BBF data model](https://usp-data-models.broadband-forum.org/tr-181-2-15-1-usp.html).

Using `select` it is possilble to extend already define objects.

***
> **NOTE**<br>
>
> MIBs are used to extend data model objects at run-time (or in the `%define` section), and are not compatible with definitions in [tr181 BBF data model](https://usp-data-models.broadband-forum.org/tr-181-2-15-1-usp.html). The data models defined by BBF are very static in nature, while the MIBs make the data model more dynamic, as extra parameters, objects and functions can be added and removed at any time.
***

An `%define` body may be empty.
### %define Syntax

![Define Syntax](doc/railroad/define.svg "define")<br>

### %define Example

> ```
> %define {
> }
> ```

### Ownership Of Data Model

The process (service or application) that loads the odl file where objects are defined, is the owner of the data model. Such a process is known as a `data model server` or `data model plug-in`. All other processes that query this data model are known as `data model consumers` or `data model clients`. 

The owner of the data model has full control of the part of the data model that it defines. No restrictions apply on the owner. The owner can always change any parameter or object in its data model.

The data model owned by an application is also referred to as the `local data model`, data models and objects not owned by the application are referred to as the `remote data model`.

### Defining Objects and Hierarchical Object Tree

Objects can be defined in the body of a define section or in the body of an object definition.

The minimum requirement to define an object is the keyword `object` and a name. All other parts of an object definition are optional.

A name of an object must comply with these rule:
- must start with a letter or underscore.
- subsequent characters must be letters, digits, underscores or hyphens.

In other words object names must match this regular expression `^[a-zA-Z_][a-zA-Z_\-0-9]*$`

It is a good idea to name objects conform to [General Notation](https://data-model-template.broadband-forum.org/index.htm#sec:general-notation) from BBF specifications.

Before the keyword `object` attributes can be specified. More information about object attributes can be found in [Appendix B - Attributes](#object-attributes)

An object definition can have an object body, which starts with `{` and ends with `}`. In this body other objects can be defined. Objects defined in the body of another object are child objects.

An object body may be empty. An empty object body is the same as terminating the object with a `;`.

To define a template (aka multi-instance or table) object put `[` and `]` after the object name. Optionally the maximum number of possible instances can be put between the square brackets `[` and `]`, this number must be a positive integer. When specifying 0 as maximum number of instances, the number of instances is unlimited (which is the same as specifying no maximum between `[` and `]`).

The object name may be put between quotes (single or double) and may contain a configuration option or environment variable reference.

When the object name is the same as a reserved odl keyword, you can put it between quotes (single or double) so it will not be interpreted as a keyword.

#### Object Body

In an object body following elements can be defined:

- Object action callback handlers - See [Object and Parameter Action Callback Handlers](#object-and-parameter-action-callback-handlers)
- Child objects - See [Defining Objects and Hierarchical Object Tree](#defining-objects-and-hierarchical-object-tree) and [Extend object definitions](#extend-object-definitions)
- Parameters - See [Define Parameters](#define-parameters) 
- Methods (functions) - See [Define Functions](#define-functions)
- Events - See [Define Events](#define-events)
- Event callback handlers - See [Object Event Callback Handlers](#object-event-callback-handlers)

For more information about one of these, please read the related topic in this document.

#### Object syntax

![Object Syntax](doc/railroad/object.svg "object")<br>

For more information about `<NAME>` read [\<NAME\>](#name) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>
For more information about `<NUMBER>` read [\<NUMBER\>](#number) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

#### Object Tree Example

> MQTT object definition from [TR-181 2-15-usp](https://usp-data-models.broadband-forum.org/tr-181-2-15-1-usp.html#T.D.Device:2.Device.MQTT.)
> ```
> %define {
>     %persistent object MQTT {
>         object Capabilitites {
>    
>         }
>         %persistent object Client[] {
>             %persistent Subscription[] {
> 
>             }
>             %persistent UserProperty[] {
> 
>             }
>         }
>     }    
> }
> ```

### Extend object Definitions

#### Extend Using MIB

##### Defining A MIB

Extra extensions objects can be defined in the `%define` section, these extension objects are called MIBs (Modular Information Block). Any object can be extended with a MIB object. MIB objects can be defined, but are not available in the data model by themselves. MIBs must be added to existing objects. A MIB object can be added to any existing object (singleton, multi-instance or instance).

Defining a MIB is exactly the same as defining an object, except the keyword **mib** must be used instead of **object**. A MIB can only be defined at top level, it never can be a child of another object or mib.

Typically MIBs are defined in separate files in a separate directories. Using the APIs `amxo_parser_scan_mib_dir` or `amxo_parser_scan_mib_dirs` these directories can be scanned to build a list of available MIB names. A MIB can be added to existing objects at run-time using the APIs `amxo_parser_apply_mib` or `amxo_parser_add_mibs`. MIBs can be removed from an object at run-time using API `amxo_parser_remove_mibs`.

A MIB can not be added to an object when it would create naming conflicts with already defined child objects, parameters or functions in the existing object.

No attributes can be set on a mib definition.

###### Mib Body

In a MIB body following elements can be defined:

- Child objects - See [Defining Objects and Hierarchical Object Tree](#defining-objects-and-hierarchical-object-tree)
- Parameters - [Define Parameters](#define-parameters)
- Methods (functions) - [Define Functions](#define-functions)

A MIB body can not be empty.

###### Define MIB Syntax

![Mib Syntax](doc/railroad/mib.svg "mib")<br>

###### Define MIB Example

> ```
> %define {
>     mib IPInfo {
>         object IPAddress[] {
>             string IPv4Address;
>         }
>     } 
> }
>
> ```

##### Extend An Object With A MIB

In the object body an object can be extended with an already defined and known MIB.

###### Extend Using A MIB Syntax

![Extend Using Syntax](doc/railroad/extend_mib.svg "extend using")<br>

For more information about `<NAME>` read [\<NAME\>](#name) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

###### Extend Using A MIB Example

> ```
> %define {
>     mib IPInfo {
>         object IPAddress[] {
>             string IPv4Address;
>         }
>     } 
>
>     object Device {
>         extend using mib IPInfo;
>         string Name; 
>     }
> }
>
> ```

#### Extend Using Select

When an object is already defined, it is possible to extend the existing object definition. This can be achieved either by using the keyword `select` or by using the parser behavior configuration option `define-behavior.existing-object`.

With the `select` the existing object is selected and in the body of the select, extra parameters, functions or child objects can be defined. The `select` takes an object path, so there is no need to repeat the full hierarchy. When extending multi-instance objects, the `[` and `]` must not be mentioned in the `select`.

The `select` keyword is typically used in included odl files.

As an alternative the configuration options `define-behavior.existing-object` can be set to `update`.

##### Select Syntax

![Select Syntax](doc/railroad/select.svg "select")<br>

For more information about `<PATH>` read [\<PATH\>](#path) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

##### Select Example

> **NOTE**  
> To simplify the examples given here the select is done in the same odl file. Typically the select is used in included odl files, to extend the existing definition. 
> The select can be done in the same `%define` section, here it is split in two `%define` sections.

> ```
> %define {
>     object MQTT {
>         object Capabilitites {
>    
>         }
>         object Client[] {
>             Subscription[] {
> 
>             }
>             UserProperty[] {
> 
>             }
>         }
>     }
> }
>
> %define {
>     select MQTT.Client {
>         Stats {
> 
>         } 
>     }    
> }
> ```

**Alternatives**

Repeat the hierarchical data model object tree with select:

> ```
> %define {
>     object MQTT {
>         object Capabilitites {
>   
>         }
>         object Client[] {
>             Subscription[] {
>
>             }
>             UserProperty[] {
>
>             }
>         }
>     }
> }
>
> %define {
>     select MQTT {
>         select Client {
>             Stats {
>             }
>         } 
>     }    
> }
> ```

Use configuration option `define-behavior.existing-object` and repeating the object hierarchy:

> ```
> %config {
>     define-behavior = {
>         existing-object = "update"
>     };
> }
>
> %define {
>     object MQTT {
>         object Capabilitites {
>   
>         }
>         object Client[] {
>             Subscription[] {
>
>             }
>             UserProperty[] {
>
>             }
>         }
>     }
> }
>
> %define {
>     object MQTT {
>         object Client {
>             Stats {
>             }
>         } 
>     }    
> }
> ```

Use configuration option `define-behavior.existing-object` and object path:

> ```
> %config {
>     define-behavior = {
>         existing-object = "update"
>     };
> }
>
> %define {
>     object MQTT {
>         object Capabilitites {
>   
>         }
>         object Client[] {
>             Subscription[] {
>
>             }
>             UserProperty[] {
>
>             }
>         }
>     }
> }
>
> %define {
>     object "MQTT.Client" {
>         Stats {
>         }
>     }    
> }
> ```

> **NOTE**  
> In the last example the object path is put between quotes. When specifying an object path after the keyword **object** the path must be put between quotes. 
> It is recommended (and easier) to use the **select** keyword to select an object which will be extended.

### Object Event Callback Handlers

To be able to act on changes in the data model, events are send. In the odl file it is possible to define callback functions that will be called for one or more of these events. 

In the object body an event callback handler can be defined that is only called for events of that specific object. When the object is a template object (aka multi-instance or table object), the event handler will also be called for events of the instance objects.

It is possible to specify for which events the callback function is called.

As an extra options a filter can be defined. The filter checks if the event data is matching with the provided boolean expression. If the filter evaluates to false for a specific event, the callback is not called for that event.

It is possible to provide function resolver instructions, these instructions are used by the odl parser to select the function resolver and by the function resolver itself to find the function implementation. More information about function resolving can be found in [Function Resolving](#function-resolving). In most cases the default function resolving will be sufficient, so most of the time there is no need to specify function resolver instructions.

To define an event callback handler in an object body use the keywords `on event` followed by the event name. You can use `*` to subscribe for all events of the object (and its instance objects) or use a regular expression to match some events. The event name must be put between quotes (single or double), when using a regular expression use `regexp(<NAME>)` where \<NAME\> is the regular expression. Only event names matching the provided regular expression will trigger the event callback handler. 

Optionally an event filter can be provided by using the keyword `filter` followed with the event filter. The event filter is a quoted string (single or double) containing a boolean expression. More information about the boolean expressions can be found in document [Logical Expression](https://gitlab.com/prpl-foundation/components/ambiorix/libraries/libamxp/-/blob/main/doc/logical_expressions.md).

#### Object Event Callback Handlers Syntax

![Subscription Syntax](doc/railroad/event_subscribe.svg "subscription")<br>

For more information about `<NAME>` read [\<NAME\>](#name) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>
For more information about `<BOOLEAN EXPRESSION>` read [\<BOOLEAN EXPRESSION\>](#boolean-expression) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

### Define parameters

Parameters can only be defined in an object body.

The minimum requirement to define a parameter is the type of the parameter and a name. All other parts of a parameter definition are optional.

The name of a parameter can be put between quotes (single or double).

A name of a parameter must comply with these rules:
- must start with a letter or underscore.
- subsequent characters must be letters, digits, underscores or hyphens.

In other words parameter names must match this regular expression `^[a-zA-Z_][a-zA-Z_\-0-9]*$`

A parameter definition can have a parameter body, which starts with `{` and ends with `}`.

A parameter body may be empty. An empty parameter body is the same as terminating the parameter with a `;`.

A default value for the parameter can be provided after the parameter name or in the parameter body with the keyword `default`. The value specified must be a valid value for the defined parameter type. If the specified value is not of the same type, conversion will be applied. If conversion of the value to the defined parameter type fails, the value is considered invalid and parsing of the odl file stops with an error. 

Valid types for data model parameters are:
- Integers: `int8`, `int16`, `int32`, `int64`, `uint8`, `uint16`, `uint32`, `uint64`
- Strings: `string`, `csv_string`, `ssv_string`, `datetime`
- Boolean: `bool`

***
> **NOTE**<br>
>
> The type `csv_string` indicates that the string contains a list of strings separated with a comma `,`. (commas separated string).
> The type `ssv_string` indicates that the string contains a list of strings separated with a space (space separated string).
> The type `datetime` is a special string and must be a date time notation in [RFC-3339](https://tools.ietf.org/html/rfc3339) notation.
***

When no default value is specified for the parameter the default for that type is used as default value.
- Empty string for string types
- 0 for integers and double types
- Unknown time for datetime type
- false for boolean types

Before the type of the parameter, attributes can be specified. More information about parameter attributes is available in [Appendix B - Attributes](#parameter-attributes).

#### Parameter Body

In a parameter body following elements can be defined:
- Parameter action callback handlers - see [Object and Parameter Action Callbacks Handlers](#object-and-parameter-action-callbacks-handlers)
- Default value - see [Parameter Default Value](#parameter-default-value).
- Userflags - see [Parameter User Flags](#parameter-user-flags).

For more information about one of these, please read the related topic in this document.

#### Parameter Syntax

![Parameter Syntax](doc/railroad/parameter.svg "parameter")<br>

For more information about `<NAME>` read [\<NAME\>](#name) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>
For more information about `<VALUE>` read [\<VALUE\>](#value) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

***
> **NOTE**<br>
>
> Although the syntax refers to `<VALUE>` not all value types can be used as a parameter value. First of all the provided value must be convertible to the parameter defined type. Array and table values can not be used as parameter values.
> When an invalid value is provided for a parameter the parser will fail with an error.
***
#### Parameter Example

> ```odl
> %define {
>     %persistent object MQTT {
>         object Capabilities {
>             %read-only csv_string ProtocolVersionsSupported = "3.1,3.1.1,5.0";
>             %read-only csv_string TransportProtocolSupported = "TCP/IP,TLS,WebSocket";
>         }
>         %persistent object Client[] {
>             %persistent %unique %key string Alias;
>             %persistent string Name;
>             bool Enable = false;
>             %read-only string Status = "Disabled";
>             %persistent string Interface;
>         }
>     }
> }
> ```

#### Special Parameters

##### Instance Count Parameters

In [tr181](https://usp-data-models.broadband-forum.org/tr-181-2-15-1-usp.html#D.Device:2.Device.MQTT.ClientNumberOfEntries) a read-only parameter is defined for each multi-instance object that contains the number of instances currently created.
This automatic read-only counter parameter can be created in the body of the template object using the keywords `counted with`. The parameter will be created in the parent object of the template object.

The `counted with` only has effect when put in the body of a template object definition, it will be ignored when put in a singleton object definition.

![Counted With Syntax](doc/railroad/counted_with.svg "counted with")<br>

For more information about `<NAME>` read [\<NAME\>](#name) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

> ```
>  %define {
>     %persistent object MQTT {
>         %persistent object Client[] {
>             counted with ClientNumberOfEntries;
>         }
>     }
> }
> ```

##### Alias Parameters

All (or almost all) multi-instance objects in [tr-181](https://usp-data-models.broadband-forum.org/tr-181-2-15-1-usp.html#D.Device:2.Device.MQTT.Client.{i}.Alias) define a string parameter with name `Alias`. In the definition is stated:

From tr-181 specification
> A non-volatile unique key used to reference this instance. Alias provides a mechanism for a Controller to label this instance for future reference.
> The following mandatory constraints MUST be enforced:
>  - The value MUST NOT be empty.
>  - The value MUST start with a letter.
>  - If the value is not assigned by the Controller at creation time, the Agent MUST assign a value with an "cpe-" prefix.
>
> If the value isn't assigned by the Controller on creation, the Agent MUST choose an initial value that doesn't conflict with any existing entries.
>
> This is a non-functional key and its value MUST NOT change once it's been assigned by the Controller or set internally by the Agent.

All this functionality is already pre-implemented in the data model engine. All you need to do is define the parameter in the template object's body.

Defining an `Alias` unique key parameter in a singleton object has not effect, as singleton objects can not have key parameters. 

> ```
> %define {
>     %persistent object MQTT {
>         %persistent object Client[] {
>             counted with ClientNumberOfEntries;
>             %persistent %unique %key string Alias
>         }
>     }
> }
> ```

When a new instance is created and no value is provided for the `Alias` parameter a value is created in form `cpe-<PARENT-NAME>-<INDEX>`, in above example it would be `cpe-Client-X` where x will be the index of the instance.

##### Parameter Default Value

A default value can be given with the parameter definition. To do this there are two options:
1. The value is provided with `= <VALUE>` after the parameter name.
2. In the parameter body with the keyword `default`.

Which notation is chosen has no impact, both alternatives will behave equally the same.

When a value is provided after the parameter name with the `= <VALUE>` and a default value is provided in the body with the keyword `default` the value specified with the `default` will be taken. 

###### Parameter Default Syntax

![Default Syntax](doc/railroad/default.svg "default")<br>

For more information about `<VALUE>` read [\<VALUE\>](#value) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

###### Parameter Default Example

> ```
> %define {
>     %persistent object MQTT {
>         object Capabilities {
>             %read-only csv_string ProtocolVersionsSupported {
>                 default "3.1,3.1.1,5.0";
>             }
>         }
>     }
> }
> ```

##### Parameter User Flags

A set of custom defined flags can be set on a parameter. These flags have no functionality or meaning for the data model engine or parser. These flags are mainly used by other services or applications to filter in or out parameters with one or more of these flags set.

An example of a use case for the user flags is to mark parameters as `upgrade persistent`. The service managing the `firmware upgrade` can then collect all parameters that are marked with a specific flag and after `firmware upgrade` restore the values.

To define parameter user flags use the keyword `userflags` in the parameter body, followed with the one or more user flag name. It is possible to set (add) a user flag by prefixing it with `%` or remove a user flag by prefixing it with `!`.

Multiple user flags can be mentioned on the same line, separated with a `,`.

Multiple user flag lines can be added to same parameter body.

A user flags line must be terminated with a `;`.

###### Parameter User Flags Syntax

![User Flags Syntax](doc/railroad/userflags.svg "user flags")<br>

For more information about `<NAME>` read [\<NAME\>](#name) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

###### Parameter User Flags Example

> ```
> %define {
>     %persistent object MQTT {
>         object Capabilities {
>             %read-only csv_string ProtocolVersionsSupported = "3.1,3.1.1,5.0" {
>                 userflags %ups,!custom;
>             }
>         }
>     }
> }
> ```

### Object And Parameter Action Callbacks Handlers

Actions can be added in parameter body definitions.

Actions are callback functions that are called for a specific reason. The function implementation will be fetched using one of the [function resolvers](#function-resolving). It is possible to add resolver instructions after the function name. Multiple callback functions for the same reason (action name) can be provided.

If no function implementation can be resolved, the action is not added to the object or parameter and the default implementation is used.

If multiple callback handlers are defined for the same action, all of them are added and will be called in the order they are added.

To define an action callback handler start with the keywords `on action`, followed with an action name. The function name that must be resolved must be put after the keyword `call`.

Extra function resolver data or instructions can be provided after the function name. When using the default odl parser function resolvers these resolver instructions will not be needed. 

Some action implementations can require that data is provided, this data can be put after the resolver data or when no resolver data is provided, after the function name. 

The most common added action on parameters is the `validate` action.

If private data is added to the object or parameter it is also common to add a `destroy` action, to clean-up any allocated memory block used to store the private data.

More details about action implementation and how the actions must behave is described in the document [Actions, Transactions and Events](https://gitlab.com/prpl-foundation/components/ambiorix/libraries/libamxd/-/blob/main/doc/actions_transactions_events.md) of the data model library (libamxd).

The possible action names for parameters are:

- `read` - called when the value of the parameter needs to be read.
- `write`  - called when the value of the parameter needs to be changed.
- `validate`  - called when the when a new value for a parameter needs to be validated.
- `destroy` - called when the parameter is going to be deleted. 
- `describe` - called for introspection reasons. 

The possible action names for objects are:

- `read` - called when the parameter values of the object need to be read.
- `write`  - called when the parameter values of the object need to be changed.
- `validate`  - called when a new object needs to be validated.
- `destroy` - called when the object is going to be deleted. 
- `describe` - called for introspection reasons.
- `add-inst` - called to add a new instance for a multi-instance object, this action only has effect on multi-instance objects.
- `del-inst` - called to delete an instance from a multi-instance object, this action only has effect on multi-instance objects.

All actions have a default implementation. 

The values of the parameters will be stored and kept in memory of the process owning the data model. The values of the parameters will be stored using the parameter's defined type. The default parameter `validate` implementation will verify if the new provided value can be converted to the defined type of the parameter. The default object `validate` implementation will always succeed.

#### Object and Parameter Action Callback Data

Some actions implementations require that data is provided. 

The data can be added behind the function name (or resolver instructions). The data can be a simple value like a number or text, or composite values like arrays or table (key value pairs). The data will be passed to the function as the private data and will always be a pointer to a variant.

***
> **NOTE**<br>  
>
>The C function implementation must match the following signature, if the C implementation does not match the signature the behavior is undefined when called.
>
> ```
> amxd_status_t <function_name>(amxd_object_t *object,
>                               amxd_param_t *param,
>                               amxd_action_t reason,
>                               const amxc_var_t * const args,
>                               amxc_var_t * const retval,
>                               void *priv);
> ```
***

#### Object and Parameter Action Callbacks Handlers Syntax

![Action](doc/railroad/action.svg "action")<br>

For more information about `<NAME>` read [\<NAME\>](#name) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>
For more information about `<VALUE>` read [\<VALUE\>](#value) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

#### Object and Parameter Action Callbacks Handlers Example

> ```
> %define {
>     %persistent object MQTT {
>         %persistent object Client[] {
>             on action validate call mqtt_instance_is_valid;
>             on action destroy call mqtt_instance_cleanup;
>
>             %persistent %unique %key string Alias;
>             %persistent string Name {
>                 on action validate call check_maximum_length 64;
>             }
>             bool Enable = false;
>             %read-only string Status = "Disabled" {
>                 on action validate call check_enum 
>                     ["Disabled", "Connecting", "Connected",
>                      "Error_Misconfigured", "Error_BrokerUnreachable",
>                      "Error"];
>             }
>         }
>     }
> }
> ```

#### Action Callback Handlers Considerations

Some limitations are applied on action callback implementations. Action callback handlers should perform a specific task and nothing else. The data model engine applies some limitations.

- `write` handlers should only push the new values to storage.
- `read` handlers should only pull the current values from storage.
- `describe` handlers should only return the current values (get them from storage) and the meta data.
- `list` handlers should only provide lists of the items available in an object.
- `destroy` handlers should only do some clean-up, most of the time they will free memory that was allocated and attached to the parameter or object as private data.
- `add-inst` handlers should only verify that a new instance can be created and create the instance if possible. An `addinst` handler may set the parameter values of the newly created instance.
- `del-inst` handlers should only verify if the instance can be deleted and delete the instance if possible.
- `validate` handlers should only verify if the newly provided value is valid for the parameter or should verify if the object is valid. Object validation handlers mostly check that the combination of the values of the parameters contained in the object are valid.

The storage can be anything, like a location in local memory (for instance an allocated C struct), a file, a table in a database. The default implementations of the actions will store the values in local memory.
##### Getters Are No Setters

All actions that perform a `read` operation on the data model (`read`, `list`, `describe`) can not change any value in the data model.  This is checked by the data model engine. If one of these handler implementations tries to change a value the action will fail.

##### Write Restrictions

Parameter `write` action callback handlers can only change the value of the parameter for which it was called. Object `write` action callback handlers can change other parameters in the same object or in the same object tree (downwards), although it is not recommended to do so. If an object `write` callback handler changes other parameters than the ones requested to change, it is possible that the transaction that started the `write` action doesn't see that the other parameters are changed, so these parameters will not be added to the change event.

##### Write Handlers Are No Event Handlers

Do not use `write` action callback handlers as an event handler. When a client application changes values of parameters in an object and a write handler is available for the object or the changed parameters it will be called, this will enable you to push the new value to storage. For parameters the `validate` action will be called before the `write` action, so there is no need to perform validation in the `write` action handler to check if the newly provided value is valid, use a `validation` action handler for this. Object `validation` handlers are called after all parameters are updated, this will allow you to verify if the combination of the values in the object still provides a valid object.

When the `validation` or `write` actions fail, or when the changes in a certain object are part of a larger transaction (spanning multiple objects), it is possible that at some point a failure is encountered. At that point all changes done will be reverted. Reverting can cause that the `write` action callback handlers are called again.

It is better to put an `event` callback handler on objects or a data model (sub-)tree. These `event` handlers will be called only when all changes have been accepted and stored in storage. In `event` handlers no limitations are set on which actions can be performed.

So it is possible that your `write` action callback is called twice, once to set the new value and once to revert the value when there was a failure. The `event` handler will be called only when the changes were accepted and applied.

### Define functions

Functions can only be defined in an object body.

The minimum requirement to define a function is the return type of the function, a name and an empty argument list. The argument list starts with `(` and ends with `)`. The argument list may be empty. All other parts of a function definition are optional.

The name of a function can be put between quotes (single or double).

A name of a function must comply with these rules:
- must start with a letter or underscore.
- subsequent characters must be letters, digits, underscores or hyphens.

In other words function names must match this regular expression `^[a-zA-Z_][a-zA-Z_\-0-9]*$`

An function definition can have a function body, which starts with `{` and ends with `}`.

An function body may be empty. An empty function body is the same as terminating the function with a `;`.

Valid return types for data model functions are:
- Integers: `int8`, `int16`, `int32`, `int64`, `uint8`, `uint16`, `uint32`, `uint64`
- Strings: `string`, `csv_string`, `ssv_string`
- Datetime string: `datetime`
- Boolean: `bool`
- Table: `htable`
- Array: `list`
- Void: `void`
- Anything: `variant`

Before the type of the function, attributes can be specified. More information about parameter attributes is available in [Appendix B - Attributes](#function-attributes).

#### Function arguments

A function can have arguments, the arguments must be defined between `(` and `)` and are  comma separated. Each argument has a type and a name.

The valid types for function arguments are:

- Integers: `int8`, `int16`, `int32`, `int64`, `uint8`, `uint16`, `uint32`, `uint64`
- Strings: `string`, `csv_string`, `ssv_string`
- Datetime string: `datetime`
- Boolean: `bool`
- Table: `htable`
- Array: `list`
- Void: `void`
- Anything: `variant`

Attributes can be set on arguments, before the type. More information about argument attributes can be found in [Appendix B - Attributes](#function-argument-attributes). If no attributes are specified for an argument the default is `%in`.

#### Function Implementation Signature

***
> **NOTE**<br>
>
>The C function implementation must match the following signature, if the C implementation does not match the signature the behavior is undefined when called.
>
> ```
> amxd_status_t <function-name>(amxd_object_t *object,
>                               amxd_function_t *func,
>                               amxc_var_t *args,
>                               amxc_var_t *ret);
> ```
***

#### Function Syntax

![Function](doc/railroad/function.svg "function")<br>
**ARGUMENT**<br>
![Argument](doc/railroad/argument.svg "argument")<br>

For more information about `<NAME>` read [\<NAME\>](#name) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>
For more information about `<VALUE>` read [\<VALUE\>](#value) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

#### Function example

> ```odl
> %define {
>    object Greeter {
>        read-only uint32 MaxHistory = 10;
>
>        uint32 say(%in %mandatory string from,
>                   %in %mandatory string message,
>                   %in bool retain = false);
>
>        uint32 setMaxHistory(%in %mandatory uint32 max);
>
>        %read-only object History[] {
>            %read-only string From;
>            %read-only string Message;
>            bool Retain;
>        }
>    }
> }
> ```

#### Default Data Model Functions

All objects in the data model will have some default `%protected` functions. These functions are implemented in the [ambiorix data model library](https://gitlab.com/prpl-foundation/components/ambiorix/libraries/libamxd). In most cases you should not call these functions directly. When accessing a `remote data model` use one of the [ambiorix bus agnostic APIs](https://gitlab.com/prpl-foundation/components/ambiorix/libraries/libamxb), for accessing the `local data model` the data model management functions from the [ambiorix data model library](https://gitlab.com/prpl-foundation/components/ambiorix/libraries/libamxd) could be a better choice as they allow more detailed operations on the data model.

It is possible to overide the default implementations, but it is not recommended to do so. An example of a module that overides the default implementations is [mod_dmproxy](https://gitlab.com/prpl-foundation/components/core/modules/mod_dmproxy) which can translate and forward the incoming requests and is mainly used to provide the [BBF TR181 data model](https://usp-data-models.broadband-forum.org/tr-181-2-15-1-usp.html).

The default functions must not be defined on each object, the default functions are added automatically with the default implementation.

The default functions definitions are in odl definition format:

```
%protected %template %instance htable _list(%in %strict rel_path = "",
                                            %in bool parameters = true,
                                            %in bool functions = true,
                                            %in bool objects = true,
                                            %in bool instances = true,
                                            %in bool template_info = true,
                                            %in events = true,
                                            %in uint32 access = 1);

%protected %template %instance htable _describe(%in %strict string rel_path = "",
                                                %in bool parameters = false, 
                                                %in bool functions = false, 
                                                %in bool instances = false, 
                                                %in bool events = false, 
                                                %in bool exists = false, 
                                                %in uint32 access = 1);

%protected %template %instance htable _get_supported(%in %strict string rel_path = "",
                                                    %in bool first_level_only = true, 
                                                    %in bool parameters = true, 
                                                    %in bool functions = true, 
                                                    %in bool events = true, 
                                                    %in uint32 access = 1);

%protected %template %instance htable _get(%in %strict string rel_path = ".", 
                                           %in %strict list parameters, 
                                           %in %uin32 depth, 
                                           %in %strict filter,
                                           %in uint32 access = 1);

%protected %template %instance void _set(%in %strict string rel_path = "", 
                                         %in %strict htable parameters, 
                                         %in %strict oparameters, 
                                         %in bool allow_partial = false,
                                         %in uint32 access = 1);

%protected %template void _add(%in %strict string rel_path = "", 
                               %in %strict htable parameters, 
                               %in uint32 index, 
                               %in string name, 
                               %in uint32 access = 1);

%protected %template void _del(%in %strict string rel_path = "", 
                               %in uint32 index, 
                               %in string name, 
                               %in uint32 access = 1);

%protected %template variant _exec(%in %strict string rel_path = "", 
                                   %in %mandatory %strict string method, 
                                   %in %out %strict htable args);

%protected %template htable _get_instances(%in %strict string rel_path = "", 
                                           %in uint32 depth, 
                                           %in uint32 access = 1);

```

When accessing a remote data model from your code it is adviced to use the functions provided by [ambiorix bus agnostic API's](https://gitlab.com/prpl-foundation/components/ambiorix/libraries/libamxb) instead of calling these default functions directly. The methods of libamxb will do more then just call these data model methods, depending on the underlying bus system used. Some bus systems have a native data model as well, these native data models will not provide these data model methods. Using the bus agnostic API will make sure that the correct translations are done to be able to access these native data models as well.

- use [amxb_get](http://sah2artifactory01.be.softathome.com:10000/documentation/ambiorix/libraries/libamxb/master/d4/d3e/a00116.html#ga9eec71677be213ec7ef464c9cd6d51f0) or [amxb_get_multiple](http://sah2artifactory01.be.softathome.com:10000/documentation/ambiorix/libraries/libamxb/master/d4/d3e/a00116.html#ga6e76cdde8fc3f543da36c43ec1d1a30f) to invoke `_get`
- use [amxb_set](http://sah2artifactory01.be.softathome.com:10000/documentation/ambiorix/libraries/libamxb/master/d4/d3e/a00116.html#ga0f4f007c7994a819fbc7825e4a9d6ed6) or [amxb_set_multiple](http://sah2artifactory01.be.softathome.com:10000/documentation/ambiorix/libraries/libamxb/master/d4/d3e/a00116.html#gae880bb11f8b02f5797b974af0c96ec4c) to invoke `_set`
- use [amxb_add](http://sah2artifactory01.be.softathome.com:10000/documentation/ambiorix/libraries/libamxb/master/d4/d3e/a00116.html#gaadb1bb7fc795a52fbaa12aea909d0039) to invoke `_add`
- use [amxb_del](http://sah2artifactory01.be.softathome.com:10000/documentation/ambiorix/libraries/libamxb/master/d4/d3e/a00116.html#ga0df0f3487c1f161df5c1869bd8d91ca6) to invoke `_del`
- use [amxb_describe](http://sah2artifactory01.be.softathome.com:10000/documentation/ambiorix/libraries/libamxb/master/d4/d3e/a00116.html#ga2b51ae59983b243165d8baebe93a1add) to invoke `_describe`
- use [amxb_list](http://sah2artifactory01.be.softathome.com:10000/documentation/ambiorix/libraries/libamxb/master/d4/d3e/a00116.html#gaeef98f6796976f70ab697d6b04f4d9f5) to invoke `_list`. Note that `amxb_list` is the only one that can handle an empty data model path.
- use [amxb_get_supported](http://sah2artifactory01.be.softathome.com:10000/documentation/ambiorix/libraries/libamxb/master/d4/d3e/a00116.html#ga1152828903956a1baaa0cdb662d289e7) to invoke `_get_supported`
- use [amxb_get_instances]() to invoke `_get_instances`

### Define events

The data model can send events when something has happened. The Ambiorix data model already provides a set of default events, these default events are automatically sent when the data model is updated using a `transaction`.

The default defined events are:

- `dm:root-added` - sent when a new `root` object has been added
- `dm:root-removed` - sent when a `root` object has been deleted
- `dm:object-added` - sent when an object has been added in the hierarchical tree
- `dm:object-removed` - sent when an object has been removed from the hierarchical tree
- `dm:instance-added` - sent when an instance has been created
- `dm:instance-removed` - sent when an instance has been deleted
- `dm:object-changed` - sent when an object has been changed
- `dm:periodic-inform` - sent at a regular interval, only when periodic-inform has been enabled
- `app:start` - The application should send this event when the data model is loaded
- `app:stop` - The application should send this event when the data model is going to be removed.

Besides these events, it is possible to define your own events in the data model. Most events are sent for a certain object, an event must be defined within the `object definition body`. It is currently not possible to define the content of the event in the `ODL`.

#### Event syntax

![Event](doc/railroad/event.svg "event")<br>

For more information about `<NAME>` read [\<NAME\>](#name) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

#### Event example

> ```
> %define {
>    object LocalAgent {
>        event Triggered!;
>    }
> }
> ```

### Define entry points

Entry points can only be defined in `%define` section body.

Entry-point functions can only be resolved using the [import resolver](#import-resolver).

Entry points are functions in your plug-in implementation that can be called for multiple reasons. The Ambiorix runtime applications calls the entry points with reason 0 (AMXO_START) after the odl files are loaded and with reason 1 (AMXO_STOP) when the application is stopping. It calls the entry points with reason 2 (AMXO_ODL_LOADED) after the post include files are loaded. Note that these entry points will never be called if there are no post includes.

This gives the implementor of the plug-in the ability to provide a function that can do initialization and clean-up. Other applications that are using the odl parser library can invoke the entry points for other different reasons.

The entry points also open the possibility to have a different entry point for a different platform, so it will be possible to write the code once and use it on different platforms without changing it.

The only thing that needs to be changed is a single odl, that refers to another entrypoint.

To resolve an entry point a library name must be given followed by a function name. Here it is not possible to specify a function resolver instructions. The entry point definition always uses the [import resolver](#import-resolver).

The shared object that implements the entry-point functions(s) must be imported with [import](#import) instruction.

Entry-point functions are defined in the odl by using the keyword **entry-point** followed with the shared object name (or its alias) a dot (`.`) followed by the entry-point function name. 

***
> **NOTE**  
>
>The C function implementation must match the following signature, if the C implementation does not match the signature the behavior is undefined when called.
>
> ```
> int <function-name>(int reason,
>                     amxd_dm_t *dm,
>                     amxo_parser_t *parser);
> ```
***

#### Entry point syntax

![Entry Point](doc/railroad/entry_point.svg "entry_point")<br>

For more information about `<NAME>` read [\<NAME\>](#name) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

#### Entry point example

> ```odl
>
> import "tr181-mqtt.so" as "tr181-mqtt";
>
> %define {
>     entry-point tr181-mqtt.mqtt_main;
> }
> ```

## Section %populate

In the `%populate` section it is possible to:

- create instances of defined template objects and set the parameter values of these newly created instances
- change parameter values of defined singleton objects, already created instance objects or template objects
- change some attributes of parameters and objects.
- Add event callback handlers for data model events.

Each `%populate` section starts with `%populate {` and ends with `}`.

Everything between the curly braces `{` and `}` is considered as the body of the `%populate` section.

### %populate Syntax

![Populate](doc/railroad/populate.svg "populate")<br>

### %populate Example

> ```
> %populate {
> }
> ```

### Selecting Object and Populate

Before instances can be added or parameters values changed the correct object must be selected. This can be done by using the keyword `object` followed with the object name or object path.

The object attributes can be either added or removed. To add object attributes use one of:

- `%protected`
- `%private`
- `%persistent`
- `%read-only`

Adding an attribute that was already set has no effect.

To remove attributes from an object use one of:

- `!protected`
- `!private`
- `!persistent`
- `!read-only`

Removing an attribute that was not set has no effect.

Adding and removing of the attributes must be grouped, so either first remove attributes and then add attributes or vica-versa.

Adding and removing of attributes is optional.

More information about the object attributes can be found at [Object Attributes](#object-attributes). 

After the object name or object path the body of the populate can be put between `{` and `}`.

In the body it is possible to set the value of one or more parameters of the selected object or add one or more instances if the selected object is a template object (aka mult-instance or table object).

Child objects can be selected using the same syntax in the object populate body.

#### Selecting Object and Populate Syntax

![Populate Object](doc/railroad/populate_object.svg "populate_object")<br>

For more information about `<PATH>` read [\<PATH\>](#path) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

#### Selecting Object and Populate Example

> ```
> %populate {
>     object MQTT.Client {
>         instance add (0, "cpe-default") {
>             parameter BrokerAddress="broker.hivemq.com";
>             parameter Enable = false;
>         }
>     }
> }
> ```

Alternativaly the object can be selected by using the object hierarchy.

> ```
> %populate {
>     object MQTT
>         object Client {
>             instance add (0, "cpe-default") {
>                 parameter BrokerAddress="broker.hivemq.com";
>                 parameter Enable = false;
>             }
>         }
>     }
> }
> ```


### Create instances

Instances of template objects can be created in the populate section using `instance add`. The `instance add` can only be used within a template object populate body.

See [Select Object and Populate](#selecting-object-and-populate) for more information on how to select the object.

When the template object contains `key` or `unique key` parameters, a value must be provided with the `instance add` and put between round brackets `(` and `)` separated with a comma.

```
%populate {
    object Path.To.My.Template {
        instance add(MyKey1 = "KeyValue1", MyKey2 = "KeyValue2") { ... }
    }
}
```

Instances always have an index, it is possible to provide the index with the `instance add`, the index must be put as first argument between the round brackets. The index must be unique within the template object. It is not allowed to have multiple instances with the same index. When no index is specified, the next available index is chosen automatically.

An instance name can be provided as well and must be provided as first (if no index) or as second argument. The name for the instance object must follow the naming rules as defined for all service elements.

> The name of each node in the hierarchy MUST start with a letter or underscore, and subsequent characters MUST be letters, digits, underscores or hyphens

If the template defines a `%unique %key` parameter `Alias` the name will be used as the value for the `Alias` parameter, the name must then follow the `Alias` naming rules as specified in TR-181.

> - The value MUST NOT be empty.
> - The value MUST start with a letter.
> - If the value is not assigned at creation time, a value is assigned with an "cpe-" prefix.

Writing:

```
%populate {
    object Path.To.My.Template {
        instance add(Alias = "AliasValue") { ... }
    }
}
```

is the same as:

```
%populate {
    object Path.To.My.Template {
        instance add("AliasValue") { ... }
    }
}
```

if the object has a `Alias` parameter.

All `%key` parameters must get a value assigned in the `instance add` as the values are needed at creation time.

All non `%key` parameter values can be set in the body. The body of an `add instance` is optional

The parameter attributes can be changed for that specific instance.

By default the instances inherit the attributes from the template object, if the template object has the `%persistent` attribute, it is possible to remove this attribute from the instance by using `!persistent`.

#### Create instances Syntax

![Instance Add](doc/railroad/instance_add.svg "instance_add")<br>

For more information about `<NUMBER>` read [\<NUMBER\>](#number) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>
For more information about `<NAME>` read [\<NAME\>](#name) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>
For more information about `<VALUE>` read [\<VALUE\>](#value) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>


#### Create instances Example

> ```
> %populate {
>     object MQTT.Client {
>         instance add (0, "cpe-default") {
>             parameter BrokerAddress="broker.hivemq.com";
>             parameter Enable = false;
>         }
>     }
> }
> ```

### Set/Change parameter values

The value of any defined parameter in any object can be changed using the ODL file. This can be done in any object populate body by using the keyword `parameter` followed by the name of the parameter, a `=` and the value. As always the line must end with a `;`.

Selecting the object for which a parameter value must be changed can be done in two different ways.

1. Specify the full path

    ```ODL
    %populate {
        object Path.To.MyObject {
            parameter MyParam = "SomeValue";
        }
    }
    ```

2. Use the hierarchical notiation

    ```ODL
    %populate {
        object Path {
            object To {
                object MyObject {
                    parameter MyParam = "SomeValue";
                }
            }
        }
    }
    ```

The ODL parser will throw an error when the object or parameter is not found. For non-existing parameters the behavior of the ODL parser can be changed by setting the option `populate-behavior.unknown-parameter` to `add`. This is not recommended, it is better to define the parameter in the `%define` section.

Parameters of existing instance objects can be changed as well using this notation. In the object path the index, the name or the alias of the instance object can be used.

It is possible to change the attributes of parameters here as well, although not recommended. When in a template object (aka multi-instance object) a parameter is defined with the `%read-only` attribute, it is possible to create an instance in the `%populate` section in which that parameter can be made writable. To remove an attribute use `!` in front of the attribute name, so to remove the `%read-only` attribute, specify `!read-only` to remove that attribute. It is also possible to add attributes.

#### Set/Change parameter values syntax

![Parameter Set Syntax](doc/railroad/parameter_set.svg "parameter set")<br>

#### Set/Change parameter values example

>```ODL
>%populate {
>    object Greeter.History.1 {
>        !read-only parameter From = "odl parser";
>        !read-only parameter Message = "Welcome to the Greeter App";
>        parameter Retain = true;
>    }
>}
>```

### Set Event Handlers

Using the `Object Definition Language` it is possible to register `event callback` functions. These functions are called when an event has happened. The simplest form of registering an `event callback` is:

```
on event "<event_name>" call <event_handler_function>;
```

To register an event callback function for all events, use `*` as the event name.

When using the `on event` it is only possible to set event handlers on events that are already `defined`. It is not possible to set a `event callback` function for  events that are defined in the code after the ODL is loaded.

As an alternative a regular expression can be used instead of a fixed event name. To indicate that the `event name` is a regular expression use `regexp( ... )`

```
on event regexp("<regexp>") call <event_handler_function>;
```

When a regular expression is used the `event handler` function is called for any event where the event name matches the regualar expression, even if the `event` is defined after the ODL has been loaded.

When only interested in events of a certain object it is possible to specify the object path:

```
on event "<event_name>" of "<object_path>" call <function_name>;
```

The `object path` itself may be a regular expression:

```
on event "<event_name>" of regexp("<object_path_reg_exp>") call <function_name>;
```

When using this alternative syntax it is not possible to provide a extra filter. When an object path is specified, the event callback handler is called for all events of objects in the sub-tree starting from the object mentioned. When a regular expression is used, the event callback handler is called for all events where the object path is matching the regular expression.

It is also possible to define event callback handlers in the object definition, for more information see [Object Event Callback Handlers](#object-event-callback-handlers).

The `Ambiorix` data model implementation already provides the following events:

- `dm:root-added`
- `dm:root-removed`
- `dm:object-added`
- `dm:object-removed`
- `dm:instance-added`
- `dm:instance-removed`
- `dm:object-changed`
- `dm:periodic-inform`
- `app:start`
- `app:stop`

After the function name it is possible to set `resolver instructions`, for more information about function resolver instructions read [Function Resolving](#function-resolving).

***
> **NOTE**<br>  
>
> The C function implementation must match the following signature, if the C implementation does not match the signature the behavior is undefined when called.
>
> ```
> void <function_name>(const char * const event_name,
>                      const amxc_var_t * const event_data,
>                      void * const priv)
>
> ```
> 
> The `priv` argument for event handlers registered using an `ODL` file will always be `NULL`
***

#### Using Event Filters

Often only some event must be handled and others can be ignored. To avoid adding code to filter out the events that needs handling `event filtering` is available. The `event filtering` is done on the event data. What is in the event data, depends on the event itself.

Using a boolean expression it is possible to describe the filter:

```
    on event "dm:object-changed" call disable_greeter
        filter 'object == "Greeter." && 
                parameters.State.from == "Running" &&
                parameters.State.to == "Stop"';
```

In the above example the `event handler callback` is only called when the event is coming from object "Greeter." and the parameter `State` changes from "Running" to "Stop".

#### Set Event Handlers syntax

![Subscription Syntax](doc/railroad/event_subscribe.svg "subscription")<br>

![Subscription Alternative Syntax](doc/railroad/event_subscribe_2.svg "subscription alternative")<br>

For more information about `<NAME>` read [\<NAME\>](#name) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>
For more information about `<BOOLEAN EXPRESSION>` read [\<BOOLEAN EXPRESSION\>](#boolean-expression) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

#### Set Event Handlers example

>```
>%populate {
>    on event "dm:object-changed" call disable_greeter
>        filter 'object == "Greeter." && 
>                parameters.State.from == "Running" &&
>                parameters.State.to == "Stop"';
>
>    on event "*" call print_event;
>}
>```

# Function Resolving

The odl parser provides three function resolvers:

- Auto resolver - **auto** - will call all resolvers in a defined order until a function is found or no more resolvers are available. The order can be defined with the configuration option `auto-resolver-order` in the `%config` section.
- Function Table - **ftab** - an application can build a function table using the odl parser library API, before starting the odl parsing itself. The ftab resolver (function table) will use this information to resolve the functions.
- Import resolver - **import** - the import resolver can load plug-ins (in shared object format using dlopen) and resolves the functions using dlsym. By default the import resolver will prefix the function names with `_<object name>_` or `_<param name>_` depending on the context. When the function is not defined in an object or parameter body the function name is prefixed with `_`.

In the odl syntax resolver instruction may be provided after each function reference, except for [entrypoints](#define-entry-points) functions.

## Default Function Resolving

By default the functions are resolved using the **auto** resolver, the default is used when no function resolver instruction is provided for the function. The auto resolver will try all available function resolvers available until one of them returns a function implementation (function pointer).

The **auto** resolver will use the function name as defined in the odl file and passes it to the real function resolvers. Some function resolvers may use a prefix or suffix to resolve the functions. The order in which the resolvers are used by the **auto** resolver can be changed using the configuration option `auto-resolver-order`

The default order of resolving is:

1. Call the **ftab** resolver. The **ftab** resolver will check if a function is available in the function table with exact the same name as defined in the odl file.
2. Call the **import** resolver. The **import** resolver will check each loaded shared object file (see [import](#import)) for the function prefixed with a `_`, if not found the import resolver will prefix it with the object name or parameter name `_<OBJECT_NAME>_<fUNCTION_NAME>`or `_<PARAMETER_NAME>_<FUNCTION_NAME>`.
3. Try any other registered function resolver in any order (unless specified differently in the configuration option `auto-resolver-order`).

***
> **NOTE**<br>
> In most cases it will not be needed to provide function resolver instructions, as the default behavior will be able to resolve the symbols.
>
> However using resolver instructions could help in quickly creating a proof of concept or prototype without the need to fully implement the solution. A shared object that implements `dummy` implementations could be used.
***

## Resolver Instructions

In most cases it will not be required to specify a function resolver instruction. The default odl parser function resolving behavior is in most cases sufficient. The behavior of the `import` and `auto` resolver can be modified using configuration options.

When specifiying a resolver, at least the resolver name must be given. Parsing of the odl file fails when a unknown resolver name is set.

The resolver instructions starts with `<!` and ends with  `!>` and must be set behind the closing `)` of the argument list (blanks are allowed) or after the function name for actions or event handlers. Within the resolver definition, the first thing to mention is the resolver name which can not contain any blanks. The name must be ended with `:` or the closing `!>`.

Function resolver data can be provided after the resolver name, separated with a `:`. The content and the format of the data depends on the resolver used. The resolver data is passed as is to the specified function resolver, the data is not interpreted by the parser itself.

When function resolver instructions are provided, only the specified resolver is used.

### Function Table Resolver

When the function table resolver is used, the function name can be specified as data. If no function name is provided, the name as defined in the odl is used, eventually prefixed with the object path. To specify the name of the function put a colon `:` behind the resolver name followed with the function name.

> ```
> %define object MQTT {
>     %define object Client[] {
>         void ForceReconnect() <!ftab:dummy_impl!>;
>     }
> }
> ```

In above example the odl parser will use the function table resolver (**ftab**) and it will try to find a function implementation named `dummy_impl`. If no function is found the parser will continue with a warning. 

If no function resolver instruction was specified the **auto** resolver would be used. In that case the function resolving would happen in this order and stops when a function imlpementation is found or no more function resolvers are available.

1. A function implementation named `ForeReconnect` would be searched using the **ftab** resolver
2. A function implementation named `_ForceReconnect` would be searched using the **import** resolver
3. A function implementation named `_Client_ForceReconnect` would be searched using the **import** resolver
4. Try any other registered function resolver in any order (unless specified differently in the configuration option `auto-resolver-order`).

You can force the use of the **ftab** resolver by only specifying the resolver name. When no function name is specified in rhe resolver data, the name of the function as defined in the odl is used, prefixed with the object path.

> ```
> %define object MQTT {
>     %define object Client[] {
>         void ForceReconnect() <!ftab!>;
>     }
> }
> ```

In above example the odl parser will use the function table resolver (**ftab**) and it will try to find a function implementation named `MQTT.Client.ForceReconnect`, if the function name prefixed with the object path is not found, the **ftab** resolver will try to fund a function implementation name `ForeReconnect`. If no function is found the parser will continue with a warning. 

When using the **ftab** resolver instruction when defining an event handler in the `%populate` section no object path is available, then only the defined function name is used.

> ```
>    on event "dm:object-changed" call disable_greeter<!ftab!>
>        filter 'object == "Greeter." && 
>                parameters.State.from == "Running" &&
>                parameters.State.to == "Stop"';
> ```

In above example the **ftab** resolver will search for a function called `disabble_greeter`.

#### Adding Functions to the Function Table

In C code function implementations can be added to the function table.

To add a function to the fuction table call:

```C
int amxo_resolver_ftab_add(amxo_parser_t* parser, const char* fn_name, amxo_fn_ptr_t fn);
```

This must be done before the odl file is parsed.

> ```C
> amxo_resolver_ftab_add(parser, "check_minimum", AMXO_FUNC(amxd_action_param_check_minimum));
> ```

#### Default Functions in Function Table

The odl parser already adds a set of parameter validation functions. All these functions can be used with `on action validate` in parameter definition bodies.

- `check_minimum` - will call function `amxd_action_param_check_minimum`
- `check_minimum_length` - will call function `amxd_action_param_check_minimum`
- `check_maximum` - will call function `amxd_action_param_check_maximum`
- `check_maximum_length` - will call function `amxd_action_param_check_maximum`
- `check_range` - will call function `amxd_action_param_check_range`
- `check_enum` - will call function `amxd_action_param_check_enum`
- `check_is_in` - will call function `amxd_action_param_check_is_in`

A parameter read action function is added to the function table. This function can be used with `on action read` in parameter definition bodies.

- `hide_value` - will callfunction `amxd_action_param_read_hidden_value`

An object add instance action function is added to the function table. This function can be used with `on action add-inst` in multi-instance object definition bodies.

- `verify_max_instances` - will call function `amxd_action_object_add_inst`

All these functions take data as input. For more information about these functions read the documentation of the called function in [libamxd](https://gitlab.com/prpl-foundation/components/ambiorix/libraries/libamxd)

> ```C
> %define {
>     object MQTT {
>         %protected uint32 MaxClients = 10 {
>             on action validate call check_minimum 1;
>         }
>         object Client[] {
>             on action add-inst call verify_max_instances "MQTT.MaxClients";
>         }         
>     }
> }
> ```

In above example a protected (internal only) parameter `MaxClients` is added to the object `MQTT` which must have a minimum value of 1. To limit the number of `MQTT.Clients.` it is possible to define the maximum number of instances between `[` and `]`, but this maximum can not be changed at run-time, it is a fixed maximum. To make it configurable at run time the action implementation `add-inst` is overiden with `verify_max_instances` which can take a references to a parameter in the data model. The value of that parameter is used as maximum.

### Import Resolver

The import resolver uses the linux dynamic loaded libraries. Dynamically loaded (DL) libraries are libraries that are loaded at times other than during the startup of a program. They're particularly useful for implementing plugins or modules.

The import resolvers uses [dlopen](https://man7.org/linux/man-pages/man3/dlopen.3.html) to load a shared library and [dlsym](https://linux.die.net/man/3/dlsym) to resolve symbols. Whenever the `import` keyword is encountered, the odl parser will instruct the import resolver to load the shared library mentioned. A shared object can only be loaded once, only the first import of a shared object will be taken into account, all other occurences of imports of the same shared object will have no effect. When a function implmentation needs to be resolved the import resolver will try to resolve the symbol using `dlsym` from the shared libraries. It will try each shared library (in the order they are loaded), and stops trying to resolve the symbol as soon as it is found in one of the shared libraries. 

The symbol name is by default the same as mentioned in the odl file, prefixed with an underscore (`_`) or prefixed with an underscore followed by the object name.

**Example**
```
import "tr181-mqtt.so" as "tr181-mqtt";

%define {
    %persistent object MQTT {
        %persistent object Client[] {
            on action validate call mqtt_instance_is_valid;
            on action destroy call mqtt_instance_cleanup;
        }
    }
}
```

In the above example the shared object (dl library) `tr181-mqtt.so` will be loaded using `dlopen`. The acion callback handlers `mqtt_instance_is_valid` and `matt_instance_cleanup` will be resolved using `dlsym` after prefixing them with `_` or `_CLient_`. The symbols that will be resolved by the import resolver are `_mqtt_instance_is_valid` or `_Client_mqtt_instance_is_valid` and `_mqtt_instance_cleanup` or `_Client_mqtt_instance_cleanup`.

When with the `import` instruction a relative path is specified (not starting with a `/`), the import resolver will search the shared object in the directories mentioned by the `import-dirs` configuration option.

The import resolver will print error messages to `stderr` when loading of the shared object fails, or print extra information when resolving symbols to `stdout`. These messages are only printed when `import-dbg` configuration option is set to `true`. These messages can help in finding out issues when loading shared objects or when resolving symbols.

Shared objects that were loaded with `import` but no symbols were resolved from the shared object will be unloaded after parsing the odl file, unless the attribute `RTLD_NODELETE` is set. More information about other attributes that can be set on import instructions can be found in [import](#import) section in this document.

When multiple shared objects exports the same symbols, it is possible to provide resolver instruction to indicate which sahred object must be used. When the symbol name in the shared object doesn't match the function name in the odl, resolver instructions can be used to specify which symbol must be resolved.

In the resolver instructions, the first part is the name of the function resolver, to use the import resolver specify the name `import`. The import resolver take as data the name of the shared object that needs to be used and optionally the name of the symbol that needs to be resolved. When import resolver instructions are provided, the name of the symbol is taken as is, no prefixing is done.

It is possible to instruct the odl parser to only use the import resolver to resolve a certain symbol, all you need to do is specify the name of the import resolver in the resolver instructions.

**Example**

```
import "tr181-mqtt.so" as "tr181-mqtt";

%define {
    %persistent object MQTT {
        %persistent object Client[] {
            on action validate call mqtt_instance_is_valid <!import:tr181-mqtt:dummy_func!>;
            on action destroy call mqtt_instance_cleanup<!import!>;
        }
    }
}
```

In above example, the odl parser is instructed to use in both cases the import resolver to resolve the symbols. The first function `mqtt_instance_is_valid` will be resolved from `tr181-mqtt.so` (note that the alias is used here) and will search for the symbol `dummy_func`. For the second function `mqtt_instance_cleanup` the odl parser is instructed to use the `import` resolver. In the last case the import resolver will go through the list of loaded shared objects and search for the symbol `_mqtt_instance_cleanup` or `_Client_mqtt_instance_cleanup`.

When functions can not be resolved (no implementation is found) the odl parser will continue with a warning.

Using the import resolver it is possible to load shared objects with common data model functionality implemented, these shared objects are commonly refered to as `modules`. The [mod-dmext.so](https://gitlab.com/prpl-foundation/components/core/modules/mod_dmext) is an example of such a module. This module provides extra parameter validation action callbacks and provides common TR-181 data model functionality.

## Resolver Instruction Syntax

![Resolver Syntax](doc/railroad/resolver.svg "resolver")<br>

For more information about `<RESOLVER>` read [\<RESOLVER\>](#resolver) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>
For more information about `<RESOLVER-DATA>` read [\<RESOLVER-DATA\>](#resolver-data) in [Appendix A - Syntax Overview](#appendix-a-syntax-overview).<br>

## Resolver Instruction Example

> ```
> import "tr181-mqtt.so" as "tr181-mqtt";
>
> %define {
>     %persistent object MQTT {
>         %persistent object Client[] {
>             on action validate call mqtt_instance_is_valid <!import:tr181-mqtt:dummy_func!>;
>             on action destroy call mqtt_instance_cleanup<!import!>;
>             %persistent string Name {
>                 on action validate call check_maximum_length <!import:tr181-mqtt!> 64;
>             }
>             bool Enable = false;
>             %read-only string Status {
>                 default "Disabled";
>                 on action validate call check_enum <!ftab!>
>                     ["Disabled", "Connecting", "Connected",
>                      "Error_Misconfigured", "Error_BrokerUnreachable",
>                      "Error"];
>             }
>         }
>     }
> }
> ```

# Appendix A - Syntax Overview
## **ACTION**
![Action Syntax](doc/railroad/action.svg "action")<br>
## **ARGUMENT**
![Argument Syntax](doc/railroad/argument.svg "argument")<br>
## **CONFIG**
![Config Syntax](doc/railroad/config.svg "config")<br>
## **COUNTER**
![Counted With Syntax](doc/railroad/counted_with.svg "counted with")<br>
## **DEFAULT**
![Default Syntax](doc/railroad/default.svg "default")<br>
## **DEFINE**
![Define Syntax](doc/railroad/define.svg "define")<br>
## **EVENT**
![Event Syntax](doc/railroad/event.svg "event")<br>
## **EXTEND-MIB**
![Extend Using Syntax](doc/railroad/extend_mib.svg "extend using")<br>
## **FUNCTION**
![Function Syntax](doc/railroad/function.svg "function")<br>
## **IMPORT**
![Import Syntax](doc/railroad/import.svg "import")<br>
## **INCLUDE**
![Include Syntax](doc/railroad/include.svg "include")<br>
## **INSTANCE ADD**
![Instance Add](doc/railroad/instance_add.svg "instance_add")<br>
## **MIB**
![Mib Syntax](doc/railroad/mib.svg "mib")<br>
## **OBJECT**
![Object Syntax](doc/railroad/object.svg "object")<br>
## **ODL**
![ODL Syntax](doc/railroad/odl.svg "odl")<br>
## **PARAMETER**
![Parameter Syntax](doc/railroad/parameter.svg "parameter")<br>
## **PARAMETER SET**
![Parameter Set Syntax](doc/railroad/parameter_set.svg "parameter set")<br>
## **POPULATE**
![Populate](doc/railroad/populate.svg "populate")<br>
## **POPULATE OBJECT**
![Populate Object](doc/railroad/populate_object.svg "populate_object")<br>
## **PRINT**
![Print Syntax](doc/railroad/print.svg "print")<br>
## **REQUIRES**
![Requires Syntax](doc/railroad/requires.svg "requires")<br>
## **RESOLVER**
![Resolver Syntax](doc/railroad/resolver.svg "resolver")<br>
## **SELECT**<br>
![Select Syntax](doc/railroad/select.svg "select")<br>
## **SUBSCRIPTION**
![Subscription Syntax](doc/railroad/event_subscribe.svg "subscription")<br>
## **SUBSCRIPTION ALTERNATIVE**
![Subscription Alternative Syntax](doc/railroad/event_subscribe_2.svg "subscription alternative")<br>
## **USERFLAGS**
![Userflags Syntax](doc/railroad/userflags.svg "userflags")<br>

## \<ACTION\>

Most of the time it is not needed to set action callback functions, but in some cases it can be helpful or handy. One such case is parameter value validation or object content validation.

The valid action names are:

- `read` - can be used on parameters and objects
- `write`  - can be used on parameters and objects
- `validate`  - can be used on parameters and objects
- `describe` - can be used on parameters and objects
- `list` - can be used on objects
- `add-inst` - can be used on multi-instance objects
- `del-inst` - can be used on multi-instance objects
- `destroy` - can be used on parameters and objects
## \<BOOLEAN EXPRESSION\>

A boolean expression is a string that contains a logical expression. This expression can be used to evaluate data. The expression can evaluate to true or false.

Read [boolean expressions](https://gitlab.com/prpl-foundation/components/ambiorix/libraries/libamxp/-/blob/main/doc/logical_expressions.md) for more information on boolean expressions.

In the odl file boolean expressions are used to filter events from the local data model. The boolean expression evaluates the event data of the received event.

## \<DATETIME\>

A date-time value is a string that is formatted according to [RFC-3339](https://tools.ietf.org/html/rfc3339) specifications. This string contains a date, followed by a time and optionally followed be a timezone offset. The time is always noted in UTC. To make the time local time, a time zone offset can be added. Note that in the data model in most situations the time is stored in UTC.

## \<FILE\>

Is a string containing an absolute or relative (filesystem) path folowed with a file name. The string must be put between quotes (single or double). Depending on the context where it is used, there could be a limitation on the files that can be used. A file path or name can also contain a configuration option or environment variable. To use a value of a confuration option use `${<NAME>}` or `${<PATH>}` where  `NAME` is the name of a configuration option or `PATH` is the path to the configuration option, this string will be replaced with the value of the configuration option. To use a value of an environment variable use `$(<NAME>)` where `NAME` is the name of the environment variable, the string will be replaced with the value of the environment variable. 

The path, if specified, and file name must be a valid filesystem file name or resolve to a valid filesystem file name.

When the file starts with a `/` it is considered an absolute path, if not starting with `/` it is considered a relative path.

## \<NAME\>

Is a string quoted or unquoted which doesn't contain any spaces or space characters (like tab or newline), all other characters can be used in the string. Extra limitation can be specified depending on the context.

A name may contain a reference to configuration options or environment variable, unless otherwise specified, using the notation `${<NAME>}` or `${<PATH>}` for configuration options and `$(<NAME>)` for environment variables. 

## \<NUMBER\>

Any positive or negative integer value. A number can be provided in:

- hexa-decimal notation - prefix with `0x`
- octal notation - prefix with `0`
- binaray notation - prefix with `0b`
- plain decimal notation

A number may be prefixed with `+` or `-`.

## \<PATH\>

Depending on the context this can have two meanings:

- A configuration option path, only used in `%config` sections.
- A data model object path, used in `%define` and `%populate` sections.

The path to a configuration option is the concatination of each successive node separated with a "." (dot). In many places in the odl file the value of configuration options can be used. To use the value of a configuration option the notation `${<NAME>}` or `%{<PATH>}` can be used. At these places the string will be replaced with the value of the referenced configuration option. Be aware that when the configuration option is not defined, it will be replaced with an empty string.

The path to a data model object is is the concatination of of each successive node in the hierarchy separated with a “.” (dot).

## \<RESOLVER\>

Is the name of the function resolver that must be used. The resolver name must be provided as an unqouted string, no spaces allowed.

The odl parser provides three function resolvers by default:

- **auto** - Will try to resolve the function implementation by trying all other function resolvers in a specific order. When no function resolving instructions are provided, this resolver is used. For more information see [Default Function Resolving](#default-function-resolving).
- **ftab** - Uses a function table to resolve the function. For more information see [Function Table Resolver](#function-table-resolver).
- **import** - Uses `dlsym` to resolve the function from one of the loaded shared objects with `import`. For more information see [Import Resolver](#import-resolver).

## \<RESOLVER-DATA\>

The resolver data is depending on the resolver itself. See the documentation of the specific resolver.

The default provided function resolvers are documented in [Function Resolving](#function-resolving)

## \<TEXT\>

Text start with a quote (single or double) and ends with quote (single or double) . The same type of quote must be used at start end end. In between the double quotes any character is allowed. When the text is in between double quoutes, any double quote inside the text must be escaped with a '\' (backslash). When the text is in between single quotes, any single quote inside the text must be escaped with a `\`. A string may contain references to configuration options or environment variables using the notations `${<NAME>}` or `${<PATH>}` for configuration options and `$(<NAME>)` for environment variables. At these places the value of the configuration option or environment variable will be used. When they don't exist, they will be replaced with an empty string.

## \<TYPE\>

The supported types are:

- void
- string
- csv_string (comma separated value string)
- ssv_string (space separated value string) 
- int8
- int16
- int32
- int64
- uint8
- uint16
- uint32
- uint64
- bool
- list
- htable
- fd
- variant
- double
- datetime (string containg date time in [RFC-3339](https://tools.ietf.org/html/rfc3339) format)

These types can be used on functions (function return value) and function arguments.
For parameter definitions not all types are supported. For more information about supported parameter types see [Define Parameters](#define-parameters).

## \<VALUE\>

![Value Syntax](doc/railroad/value.svg "value")<br>

Can be one of:

- **Text**: Text start with a quote (single or double) and ends with quote (single or double) . The same type of quote must be used at start end end. In between the double quotes any character is allowed. When the text is in between double quoutes, any double quote inside the text must be escaped with a '\' (backslash). When the text is in between single quotes, any single quote inside the text must be escaped with a `\`. A string may contain references to configuration options or environment variables using the notations `${<NAME>}` or `${<PATH>}` for configuration options and `$(<NAME>)` for environment variables. At these places the value of the configuration option or environment variable will be used. When they don't exist, they will be replaced with an empty string.
- **Word**: A string wihtout any space characters (space, tab, newline), can contain any character (except spaces) and doesn't need to be put between quotes (single or double).
- **Number**: A number can only contain numeric characters, a '-' sign can be put in front to indicate negative numbers. Numbers between quotes (double quotes) are considered text. Numbers can be notated in hexadecimal notation (prefixed with 0x), octal notation (starts with 0), binary notation (prefixed with 0b) or plain decimal notation.
- **Boolean**: `true` or `false` are the only accepted values. When put between quotes (single or double) the values are considered as text.
- **Table**: A table starts with `{` and ends with `}`. Within a table key-value pairs can be added, a key can be a **word** or **text**. a value can be any value. Duplicate keys are not supported, when the same key is used more then once in a table, only the first occurence is taken into account, the next occurences are ignored. Each entry in the table is separated from the next entry using ','. No ',' may be added after the last entry.
- **Array**: An array starts with '[' and ends with ']'. Within an array any value can be specified. Each entry must be separated from the previous with a ','. No ',' may be added after the last entry.

# Appendix B - Attributes

Multiple attributes can be provided and must be separated with at least on blank (space, tab newline). 

## Import Attributes

- `RTLD_NOW` If this value is specified, or the environment variable LD_BIND_NOW is set to a nonempty string, all undefined symbols in the shared object are resolved before dlopen() returns.  If this cannot be done, an error is returned.
- `RTLD_GLOBAL` The symbols defined by this shared object will be made available for symbol resolution of subsequently loaded shared objects.
- `RTLD_NODELETE` Keep the shared object in memory even if no symbols are used from this shared object by the odl parser.

If no attributes are specified with an import instruction, the shared object is removed (unloaded using `dlclose()`) from memory.
The default flags passed to `dlopen()` if not attributes are specified is `RTLD_LAZY`.

## Config Attributes

- `%global` - indicates that the new value of the config option must be applied globally and not only in the current scope.

## Object Attributes

Object attribute names must always be prefixed with `%` or `!`. The `%` sign indicates that the object attributes must be set and the `!` sign indicates that the object attribute must be unset.

In the `%define` section attributes can only be set. In the `%populate` section attributes can be set and unset.

- `%read-only` or `!read-only`
- `%persistent` or `!persistent`
- `%private` or `!private`
- `%protected` or `!protected`

The attribute `%read-only` only has effect on template objects. When a template object is set as read-only it will not be possible for external sources (data model clients) to add or delete instances.
The instances itself will not inherit this attribute. The `%read-only` attribute has no effect on singleton or instance objects.

The attribute `%persistent` marks the object for persistent storage. Only objects with the attribute set will be stored.

The attribute `%private` makes the object invisible for external sources (data model clients). Instances of template objects will not inherit this attribute. Take into account when a template object is set private, all it's instances are not visible either. Objects marked private are only accessible for the owner.

The attribute `%protected` marks the object for internal use only. Typically this is set on objects that extend the standard data model as define in [TR-181](https://usp-data-models.broadband-forum.org/tr-181-2-15-1-usp.html) but the object should not be replied on requests from remote management applications (like USP-controller, CWNPD, web-applications, ...). This attribute makes it possible to define objects that provide extra internal functionality that is visible and accessible for data model clients.

Attributes can not be put between quotes. Zero, on or more attributes can be specified before the keyword `object`, and must be separated with at least on space (space, tab, new line).

The object attributes are reserved odl parser keywords.

## Parameter Attributes

Parameters attribute names must always be prefixed with `%` or `!`. The `%` sign indicates that the parameter attributes must be set and the `!` sign indicates that the parameter attribute must be unset.

In the `%define` section attributes can only be set. In the `%populate` section some attributes can be set and unset.

- `%read-only` or `!read-only`
- `%persistent` or `!persistent`
- `%private` or `!private`
- `%protected` or `!protected`
- `%template`
- `%instance`
- `%volatile` or `!volatile`
- `%key`
- `%unique`

Attributes can be set on parameters. The valid attributes are:

- `%read-only`
- `%persistent`
- `%private`
- `%protected`
- `%template`
- `%instance`
- `%volatile`
- `%key`
- `%unique`
- `%mutable`

The attribute `%read-only` prevents external sources from changing the parameter's value with a `set` operation. The process that owns the data model can change the value.

The attribute `%persistent` marks the parameter for persistent storage. Only parameters with the attribute set will be stored.

The attribute `%private` makes the parameter invisible for external sources. The process that owns the data model can see and access the value.

The attribute `%protected` marks the parameter for internal use only. Typically this is set on parameters that extend the standard data model as define in [TR-181](https://usp-data-models.broadband-forum.org/tr-181-2-15-1-usp.html) but the parameter should not be replied on requests from remote management applications (like USP-controller, CWNPD, web-applications, ...). This attribute makes it possible to define parameters that provide extra internal functionality that is visible and accessible for data model clients.

The attribute `%template` makes the parameter `usable` on template objects. A parameter with only the `%template` attribute set (and not the `%instance`) will not be inherited by instance objects. This attribute has only effect on parameters defined in template objects and is ignored on parameters defined in singleton objects.

The attribute `%instance` makes the parameter `usable` on instance objects. A parameter with the `%instance` attribute set will be inherited by instance objects. This attribute has only effect on parameters defined in template objects and is ignored on parameters defined in singleton objects. If only this attribute is set and not the `%template` attribute the parameter definition is still accessible in the template object.

The attribute `%volatile` marks the parameter as "changes often". The only effect that this attribute has is that no events are send when the parameter's value changes.

The attribute `%key` can only be used on parameters defined in a template object. The attribute can be set on parameters in the `%populate` section if the template object does not have any instances yet. The `key` parameters are used to identify the instance in a unique way. The combination of the values of the `key` parameters must be unique. These parameters are immutable, once set the value can not be changed, unless the attribute `%mutable` is set as well. 

The attribute `%unique` indentifies that only one instance can exists with the same value. When the attribute is used in combination with `%key` only one instance can exists with a certain value for the parameter and the parameters becomes immutable.

Attributes can not be put between quotes. Zero, on or more attributes can be specified before the type of the parameter, and must be separated with at least on space (space, tab, new line).

When no attributes are specified on parameters defined in a template object, by default the `%instance` attribute is set.

Defining a parameter with the attributes `%mutable %unique %key` is the same as using attribute `%unique`. A mutable unique key is just a unique value that can be changed. Defining a parameter with attributes `%mutable %key` makes the parameter part of a composite key. A composite key is the combination of multiple parameters that can identify the instance, but defining it as `%mutable` that parameter which is part of the composite key can be changed.

## Function Attributes

Function attribute names must always be prefixed with `%`. Function attributes can only be set in the `%define` section

- `%private`
- `%protected`
- `%template`
- `%instance`

The attribute `%private` makes the function invisible for external sources. The process that owns the data model can see and invoke the method.

The attribute `%protected` can be used to indicate that the function is for `internal` use only. 

The attribute `%template` makes the function `callable` on template objects. A function with only the `%template` attribute set (and not the `%instance`) will not be inherited by instance objects. This attribute has only effect on template objects and is ignore on singleton objects.

The attribute `%instance` makes the function `callable` on instance objects. A function with only the `%instance` attribute set will be inherited by instance objects. This attribute has only effect on template objects and is ignored on singleton objects. If only this attribute is set and not the `%template` attribute the function definition is still accessible in the template object, but can't be called on the multi-instance object.

## Function Argument Attributes

- `%in` - the argument is an in argument (caller to callee)
- `%out` - the argument is an out argument (callee to caller)
- `%mandatory` - the argument is mandatory, a function invocation without all mandatory arguments provided fails (this attribute is ignored on `out` only arguments)
- `%strict` - the caller must provide the argument with the correct type, if the type does not match the defined type, the function invocation fails.

The attribute `%in` indicates that the arguments can be provided by the caller.

The attribute `%out` indicates that the arguments will be set by the callee and its value is accessible when the function returns.

Arguments declared with attribute `%madatory` are required, if a function caller does not provide such an argument, the function call fails. This attribute is ignored on `%out` only arguments.

Arguments declared with attribute `%strict` must be provided by the caller with the correct type, if the provided value is not of the defined type, the function call fails. If an argument is not strictly typed, data type conversion can be done by the callee.

A default value for non-mandatory in arguments can be provided by adding a `=` behind the name of the argument followed with the default `value`.

If no attributes are provided for an argument the default is `%in`. 

# Appendix C - ODL Keywords

None of the keywords can be used as a name or as a single word, but can be used in text (between quotes double or single).

The keywords are grouped and in alphabetic order.

## Language

- `action`
- `add`
- `as`
- `call`
- `counted`
- `default`
- `entry-point`
- `event`
- `extend`
- `false`
- `filter`
- `include`
- `#include`
- `?include`
- `&include`
- `import`
- `instance`
- `mib`
- `object`
- `on`
- `parameter`
- `regexp`
- `select`
- `using`
- `true`
- `with`

## Import Attributes

- `RTLD_GLOBAL`
- `RTLD_NODELETE`
- `RTLD_NOW`

## Section Keywords

- `%config`
- `%define`
- `%populate`

## Attribute Keywords

- `%global`
- `%in`
- `%instance`
- `%key`
- `%mandatory`
- `%out`
- `%persistent`
- `!persistent`
- `%private`
- `!private`
- `%protected`
- `!protected`
- `%read-only`
- `!read-only`
- `%strict`
- `%template`
- `%unique`
- `%volatile`
- `!volatile`

## Type Keywords

- `bool`
- `csv_string`
- `double`
- `datetime`
- `fd`
- `htable`
- `int8`
- `int16`
- `int32`
- `int64`
- `list`
- `ssv_string`
- `string`
- `uint8`
- `uint16`
- `uint32`
- `uint64`
- `variant`
- `void`

## Action Names

- `add-inst`
- `del-inst`
- `describe`
- `list`
- `read`
- `validate`
- `write`

# Appendix D - Saving the Data Model and Configuration Options

The odl parser library (libamxo) provides functions that can save the configuration options or the data model in odl format.

To save configuration options use `amxo_parser_save_config` or `amxo_parser_save`.

To save a data model (or part of a data model) use `amxo_parser_save_object` or `amxo_parser_save`. 

When saving the data model only objects and parameters with the attribute `%persistent` are taken into account. When an object is encounterd in the hierarchical object tree that doesn't have the `%persistent` attribute, nothing of that object or it's sub-tree is saved, event in child objects or parameters in that object have the `%persistent` attribute set. 

# Appendix E - Tools and Applications Using ODL Parser

## Ambiorix Run Time (amxrt) 

The ambiorix runtime (amxrt) is an application that provides base functionality to turn your defined data model using the Object Definition Language (odl) into a functional application that expsoses a data modle to other applications using a bus system or a protocol.

Amxrt provides besides parsing the odl files using libamxo (the odl parser):

- an eventloop (implemented using [libevent](https://libevent.org/)).
- tracking of required objects.
- invoking the entry-points defined in the odl files at startup with reason 0 (AMXO_START).
- invoking the entry-points (in reverse order) defined in the odl files when stopping with reason 1 (AMXO_STOP).
- loading of the post-includes (`&include` in the odl files).
- automatic saving of persistent objects and parameters when changes are done to the data model.
- provides a bunch of default configuration options.
- auto-detection of (most) bus systems (backends and sockets) and connect to them.
- registration of the loaded data model. 

Amxrt is single-threaded and event-driven.

More information and more details of `amxrt` can be found in the README.md file in the [amxrt gitlab repository](https://gitlab.com/prpl-foundation/components/ambiorix/applications/amxrt).

## Ambiorix ODL Compiler/Generator (amxo-cg)

The application `amxo-cg` is capable of verifying that your data model defined in the odl files is valid and correct. Besides verification is is also capable to generate xml files from your odl, or create template C functions for the data model methods (RPC methods) you defined in the odl files.

`amxo-cg` can use the comments placed in front of objects, parameters and functions as documentation. It support some tags that can be used to create structured documentation for your data model.

Creating documentation is done in two steps:

1. Generate xml files using the odl files.
2. Use `amxo-xml-to` to translate the generated xml file into other documents (like html) using xml transformation (xslt).

More information can be found in the README.md files of [amxo-cg](https://gitlab.com/prpl-foundation/components/ambiorix/applications/amxo-cg) and [amxo-xml-to](https://gitlab.com/prpl-foundation/components/ambiorix/applications/amxo-xml-to).

# Appendix F - Creating A Function Resolver

Extra function resolvers can be registered to the odl parser. Probably you will never create a function resolver but there are some use case where adding a function resolver can be interesting:

- Support for other languages, so it is possible to resolve functions that are not written in C. Some examples are LUA, Python, Rust.
- Easy prototyping or proof-of-concept implementations. If you don't want to provide function resolver instructions in the odl file, but want to use dummy implementations, it can be handy to have a separate function resolver that returns function pointer to these dummy implementations.

## Registering A Function Resolver

To register an extra function resolver to the odl parser a `amxo_resolver_t` structure needs to defined.

```C
typedef struct _amxo_resolver {
    amxc_htable_it_t hit;
    amxo_res_get_default_t get;
    amxo_res_resolve_fn_t resolve;
    amxo_res_clean_fn_t clean;
    void* priv;
} amxo_resolver_t;
```

This structure contains a `hash table iterator`, this will be used by the `function resolver manager` to store the function resolver in a hash table containing all the function resolvers.

Three function pointers can be provided:

1. `get` - This function is optional, when available (not NULL) it will be called when a new odl parser is created using `amxo_parser_init` or `amxo_parser_new`. This function can be used to set the `default` options for the function resolver in the parser `configuration` and to do extra initialization if needed.
2. `clean` - This function is optional, when availble (not NULL) it will be called when an odl parser instance is deleted using `amxo_parser_clean` or `amxo_parser_delete`. This function can be used to remove the resolver `configuration` from the odl parser and to do extra clean-up if needed.
3. `resolve` - This function is mandatory and does the real work. It should return a valid function pointer or NULL if no function could be provided. 

The prototypes of these functions are:

```C
typedef void (* amxo_res_get_default_t) (amxo_parser_t* parser, void* priv);

typedef amxo_fn_ptr_t (* amxo_res_resolve_fn_t) (amxo_parser_t* parser,
                                                 const char* fn_name,
                                                 amxo_fn_type_t type,
                                                 const char* data,
                                                 void* priv);

typedef void (* amxo_res_clean_fn_t) (amxo_parser_t* parser,
                                      void* priv);
```

It is possible to add some `private` data to your function resolver. The pointer to this private data will always be passed to functions when called.

The resolver function takes following arguments:

- `parser` - pointer to the odl parser
- `fn_name` - the name of the function that needs to be resolved.
- `type` - type of function, this can be one of `amxo_function_action` for action callback functions, `amxo_function_rpc` for data model RPC functions, `amxo_function_event` for event callback functions.
- `data` - some resolver specific data, this is the data provided with resolver instructions in the odl file. This can be a NULL pointer if no data is available.

To register the function resolver all you need to do is call

```C
int amxo_register_resolver(const char* name, amxo_resolver_t* resolver);
```

The first argument is the name of the function resolver, this must be unique only one resolver with a specific name. This name can be used in function resolver instructions.

When the registration of the function resolver is successful (`amxo_register_resolver` returns 0), the `auto` resolver will use this resolver as well. Were in the resolving order it is used all depends on the configuration option `auto-resolver-order`. For more information about the `auto` resolver see [Default Function Resolving](#default-function-resolving).

Typically a function resolver is implemented in a shared object, which can be loaded in the odl file using the `import` instruction. It could be that the option `RTLD_NODELETE` is needed to prevent that the shared object is unloaded after parsing the odl files.

Using a `constructor` function is the easiest way to register the `function resolver` as soon as the shared object that implements the resolver is loaded.

**Example**
```C
static amxo_resolver_t ftab = {
    .hit = { .ait = NULL, .key = NULL, .next = NULL },
    .get = amxo_resolver_ftab_defaults,
    .resolve = amxo_resolver_ftab,
    .clean = amxo_resolver_ftab_clean,
    .priv = NULL
};

CONSTRUCTOR_LVL(110) static void amxo_ftab_init(void) {
    amxo_register_resolver("ftab", &ftab);
}
```

The default function resolvers `ftab` and `import` are also registered to the odl parser using this function. 

## Unregistering A Function Resolver

A registered function resolver can be removed as well.

To remove a function resolver use the function `amxo_unregister_resolver`.

```C
int amxo_unregister_resolver(const char* name);
```

**Example**
```C
DESTRUCTOR_LVL(110) static void amxo_ftab_cleanup(void) {
    amxo_unregister_resolver("ftab");
}
```

***
> **NOTE**<br>
> It is possible to remove the default function resolvers, altough not recommended. 
***

## Examples

### The LUA Function Resolver

The LUA function resolver [mod-lua-amx](https://gitlab.com/prpl-foundation/components/ambiorix/bindings/lua/mod-lua-amx) adds a function resolver that makes it possible to implement validate action callback handlers, event callback handlers and data model function in LUA directly in the odl file using the function resolver instructions.

This can be handy to implement simple data models, no need to write C code, compile and istall it, just write the implementations in LUA directly in the odl file. Other use cases to use the LUA function resolver are:

- prototyping 
- Proof Of Concept implementations

This make it possible to create more functional data models without the need to start a full implementation.

More information about the LUA function resolver can be found in the README.md file in [mod-lua-amx](https://gitlab.com/prpl-foundation/components/ambiorix/bindings/lua/mod-lua-amx).
