# Object Definition Language

The Object Definition Language provides a simple mean to define a data model.

A data model is a hierarchical tree of objects where each object can contain 0, 1 or more parameters and 0, 1 or more functions.

***
> ### __Table of Contents__  
>
>  1. [Writing comments](#writing-comments)  
>  2. [Sections](#sections)  
  2.1. [Section %config](#Section%20%25config)  
  |_______ [Syntax](#%25config-syntax)  
  |_______ [Example](#%25config-example)  
  2.1.1 [Predefined Configuration Options](#predefined-configuration-options)  
  |_______ [ODL parser configuration options](#odl-parser-configuration-options)  
  |_______ [Ambiorix Run Time configuration options](#ambiorix-run-time-configuration-options)  
  2.2. [Section %define](#section-%25define)  
  |_______ [Syntax](#%25define-syntax)  
  |_______ [Example](#%25define-example)  
  2.2.1. [Define objects](#define-objects)  
  |_______ [Attributes](#object-attributes)  
  |_______ [Syntax](#object-syntax)  
  |_______ [Example](#object-example)  
  2.2.2. [Define parameters](#define-parameters)  
  |_______ [Attributes](#parameter-attributes)  
  |_______ [Value](#parameter-value)  
  |_______ [Syntax](#parameter-syntax)  
  |_______ [Example](#parameter-example)  
  2.2.3. [Define functions](#define-functions)  
  |_______ [Attributes](#function-attributes)  
  |_______ [Arguments](#function-arguments)  
  |_______ [Function Signature](#function-signature)  
  |_______ [Default functions](#default-functions)  
  |_______ [Syntax](#function-syntax)  
  |_______ [Example](#function-example)  
  2.2.4. [Define entry points](#define-entry-points)  
  |_______ [Syntax](#entry-point-syntax)  
  |_______ [Example](#entry-point-example)  
  2.2.5 [Define actions](#define-actions)  
  |_______ [Action names](#action-names)  
  |_______ [Data](#action-data)  
  |_______ [Syntax](#action-syntax)  
  |_______ [Example](#action-example)  
  2.2.6 [Resolver Instructions](#resolver-instructions)  
  2.3. [Section %populate](#section-%25populate)  
  2.3.1. [Create instances](#create-instances)  
  2.3.2. [Change parameter values](#change-parameter-values)  
  2.3.3. [Event subscription](#event-subscription)
>  3. [Include](#include)  
  |_______ [Syntax](#include-syntax)  
  |_______ [Example](#include-example)  
>  4. [Import](#import)
>  5. [Syntax Overview](#syntax-overview)  
  5.1.[VALUE](#value)  
  5.2.[ATTRIBUTES](#attributes)  
  5.3.[TYPE](#type)  
  5.4 [RESOLVER](#resolver)  
  5.5 [RESOLVER-DATA](#resolver-data) 
>  6. [ODL Keywords](#odl-keywords)
>  7. [PCB compatibility](#pcb-compatibility)

***

## Writing comments

In an odl file it is possible to write comments.

- Single line comments starts with `//` and spans till the end of the line (newline)

- Multi-line comments starts with `/*` and ends with `*/`

Comments can be used at any place in the odl file.

## Sections

There are 3 kinds of sections available in an odl file:

- `%define`
- `%config`
- `%populate`

Section rules:

- All sections are optional, so an odl file without any section is valid.
- Each section can be used multiple times in a single odl file.
- The different kind of sections can appear in any order.

***
> NOTE  
> Although the order of the sections doesn't matter, it matters that objects and parameters are defined in a `%define` section, before they are used in the `%populate` section.
***

### Section %config

In the config section values for configuration options can be specified.
There is no restriction on which configuration options are set. Which configuration options will be used all depends on the application or plug-in.

Each config section starts with `%config {` and ends with `}`.

Everything between the curly braces '{' and '}' is considered as the body of the config section.

#### %config Syntax

>Syntax:
>
>```odl
> %config {
>    <name> = <VALUE>;
>    <name> = [ <VALUE>, <VALUE>, ... ];
>    <name> = { <name> = <VALUE>, <name> = <VALUE>, ... };
>  ...
>}
>```

#### %config Example

> Example:
>
> ```odl
> %config {
>     import-dbg = true;
>     message = "My welcome message";
>     auto-resolver-order = [ "import", "ftab", "*" ];
> }
> ```

#### Predefined Configuration Options

##### ODL parser configuration options

The parser itself supports the following configuration options:

- `include-dirs` - an array of include directories (quoted strings). Include files specified in the odl file with no path or a relative path are searched in these directories.
- `import-dirs` - an array of of import directories (quoted strings). Import files (plug-in shared objects) specified in the odl file with no path or a relative path are searched in these directories.
- `import-dbg` - boolean (true or false). Makes the import function resolver print more information to stderr
- `import-pcb-compat` - uses the PCB style prefixing to resolve symbols.
- `auto-resolver-order` - an array of resolver names (quoted strings). The order specified in this list is the order the auto resolver invokes them to resolve a function symbol. This list can end with a `*` to indicate all other resolvers in no specified order. When the list is empty, the auto resolver will not resolve any symbol.

##### Ambiorix Run Time configuration options

- `uris` - an array of uris (quoted strings) that is used to automatically connect to (when `auto-connect` is set to `true`)
- `backends` - an array of Bus specific back-end shared objects (quoted strings) for the bus agnostic API. The back-ends in the list are loaded and used. If one of these can not be loaded or fails to load the application does not start.
- `auto-detect` - `true` or `false`, will try to auto-detect which back-ends are available and loads them, also tries to auto-detect the available bus unix domain sockets.
- `auto-connect` - `true` or `false`, will try to connect to the connection uri's specified in the configuration option `uris`.
- `daemon` - `true` or `false`. When set to `true` the Ambiorix run time will daemonize after calling the odl defined entry points.
- `priority` - sets the nice level of the launched application.

### Section %define

In the `%define` section the data model and entry points are defined.
Objects can contain parameters, functions or other objects.

Each define section starts with `%define {` and ends with `}`.

***
> For backwards compatibility reasons (PCB ODL) the `%define` and the open `{` and close `}` curly brace can be omitted. The notation without is considered depricated and should not be used in new written odl files, unless they need to be used for PCB based applications as well.
***

Everything between the curly braces '{' and '}' is considered as the body of the define section.

#### Define objects

Objects can be defined in the body of a define section or in the body of an object definition.

The minimum requirement to define an object is the keyword `object` and a name. All other parts of an object definition are optional.

An object definition can have an object body, which starts with `{` and ends with `}`. In this body other objects can be defined. Objects defined in the body of another object are child objects.

To define template object put `[` and `]` after the object name. Optionally the maximum number of possible instances can be put between the square brackets `[` and `]`

##### Object Attributes

Attributes can be set on objects. The valid attributes are:

- `%read-only`
- `%persistent`
- `%private`

The attribute `%read-only` only has effect on template objects. When a template object is set as read-only it will not be possible for external sources to add or delete instances.
The instances itself will not inherit this attribute. The `%read-only` attribute has no effect on singletons or instances.

The attribute `%persistent` marks the object for persistent storage. Only objects with the attribute set will be stored. Instances of template objects do no inherit this attribute.
If needed set the attribute on the instance when it is created.

The attribute `%private` makes the object invisible for external sources. Instances of template objects will not inherit this attribute. Take into account when a template object is set private, all it's instances are not visible either.

##### Object syntax

>Syntax:
>
>```odl
>[<ATTRIBUTES>] object <name>;
>
>[<ATTRIBUTES>] object <name> {
>    ...
>}
>
>[<ATTRIBUTES>] object[] <name>;
>
>[<ATTRIBUTES>] object[] <name> {
>    ...
>}
>```

##### Object example

> Example:
>
> ```odl
> %define {
>    object Greeter {
>        object History[];
>    }
> }
> ```

#### Define parameters

Parameters can only be defined in an object body.

The minimum requirement to define a parameter is the type of the parameter and a name. All other parts of a parameter definition are optional.

##### Parameter attributes

Attributes can be set on parameters. The valid attributes are:

- `%read-only`
- `%persistent`
- `%private`
- `%template`
- `%instance`
- `%volatile`
- `%key`

The attribute `%read-only` prevents external source to change the parameter's value with a `set` operation. The process that owns the data model can change the value.

The attribute `%persistent` marks the parameter for persistent storage. Only parameters with the attribute set will be stored.

The attribute `%private` makes the parameter invisible for external sources. The process that owns the data model can see and access the value.

The attribute `%template` makes the parameter `usable` on template objects. A parameter with only the `%template` attribute set (and not the `%instance`) will not be inherited by instance objects. This attribute has only effect on template objects and is ignore on singleton objects.

The attribute `%instance` makes the parameter `usable` on instance objects. A parameter with the `%instance` attribute set will be inherited by instance objects. This attribute has only effect on template objects and is ignore on singleton objects. If only this attribute is set and not the `%template` attribute the parameter definition is still accessible in the template object.

The attribute `%volatile` marks the parameter as "changes often". The only effect that this attribute has is that no events are send when the parameter's value changes.

The attribute `%key` can only be used on parameters defined in a template object. The attribute can be set on parameters in the `%populate` section if the template object does not have any instances yet. The `key` parameters are used to identify the instance in a unique way. The combination of the values of the `key` parameters must be unique. These parameters are immutable, once set the value can not be changed. 

When no attributes are specified on parameters defined in a template object, by default the `%instance` attribute is set.

##### Parameter value

A default value can be defined.

When no value is specified the default value for the declared type is used.

When the parameter definition has a definition body (starts with `{` and ends with `}` behind the parameter name) the default value must be defined in the parameter body using the keyword `default`.
This is typically done when action for the parameter are defined.

##### Parameter syntax

>Syntax:
>
>```odl
>[<ATTRIBUTES>] <TYPE> <name>;
>
>[<ATTRIBUTES>] <TYPE> <name> = <VALUE>;
>
> [<ATTRIBUTES>] <TYPE> <name> {
>   ...
> }
>```

##### Parameter example

> Example:
>
> ```odl
> %define {
>    object Greeter {
>        %read-only uint32 MaxHistory = 10;
>        %read-only object History[] {
>            %read-only string From;
>            %read-only string Message;
>            bool Retain;
>        }
>    }
> }
> ```

#### Define functions

Functions can only be defined in an object body.

The minimum requirement to define a function is the return type of the function, a name and an empty argument list. All other parts of a function definition are optional.

##### Function attributes

Attributes can be set on functions. The valid attributes are:

- `%private`
- `%template`
- `%instance`

The attribute `%private` makes the function invisible for external sources. The process that owns the data model can see and invoke the method.

The attribute `%template` makes the function `callable` on template objects. A function with only the `%template` attribute set (and not the `%instance`) will not be inherited by instance objects. This attribute has only effect on template objects and is ignore on singleton objects.

The attribute `%instance` makes the function `callable` on instance objects. A function with only the `%instance` attribute set will be inherited by instance objects. This attribute has only effect on template objects and is ignore on singleton objects. If only this attribute is set and not the `%template` attribute the function definition is still accessible in the template object.

##### Function arguments

A function can have arguments, the arguments must be defined between '(' and ')' and are 
comma separated. Attributes can be set on arguments, for arguments these are the valid attributes:

- `%in`
- `%out`
- `%mandatory`
- `%strict`

The attribute `%in` indicates that the arguments can be provide by the caller.

The attribute `%out` indicates that the arguments will be set by the callee and it's value is accessible when the function returns.

Arguments declared with attribute `%madatory` are required, if a function caller does not provide such an argument, the function call fails. This attribute is ignored on `%out% only arguments.

Arguments declared with attribute `%strict` must be provided by the caller with the correct type, if the provided value is not of the defined type, the function call fails. If an argument is not strict typed, data type conversion can be done by the callee.

##### Function Signature

>The C function implementation must match the following signature, if the C implementation does not match the signature the behavior is undefined when called.
>
> ```
> amxd_status_t <function-name>(amxd_object_t *object,
>                               amxd_function_t *func,
>                               amxc_var_t *args,
>                               amxc_var_t *ret);
> ```

##### Default functions

The default functions must not be defined on each object, the default functions are added automatically with the default implementation.

The default functions definitions are:

- %template %instance htable list(parameters = true, functions = true, objects = true, instance = true);
- %template %instance htable get(%strict list parameters);
- %template %instance void set(%strict htable parameters);
- %template void add(%strict htable parameters, uint32 index, string name);
- %template void del(uint32 index, string name);

It is possible to overide these function, either with exactly the same definition but another function implementation or even with a total different definition (only the same name).

##### Function syntax

>Syntax:
>
>```odl
>[<ATTRIBUTES>] <TYPE> <name>([<ATTRIBUTES name [= <VALUE>]>, ...]);
>
>[<ATTRIBUTES>] <TYPE> <name>([<ATTRIBUTES name [= <VALUE>]>, ...])<!<RESOLVER>:<RESOLVER-DATA>!>;
>```

##### Function example

> Example:
>
> ```odl
> %define {
>    object Greeter {
>        read-only uint32 MaxHistory = 10;
>
>        uint32 say(%in %mandatory string from,
>                   %in %mandatory string message,
>                   %in bool retain = false);
>
>        uint32 setMaxHistory(%in %mandatory uint32 max)<!ftab:my_set_max_impl!>;
>
>        %read-only object History[] {
>            %read-only string From;
>            %read-only string Message;
>            bool Retain;
>        }
>    }
> }
> ```

#### Define entry points

Entry points can only be defined in `%define` section body.

When using the import resolver (see [Import](#import)), entry points can be defined.
Entry points are functions in your plug-in implementation that can be called for multiple reasons. The Ambiorix runtime calls the entry points with reason 0 (START) after the odl files are loaded and with reason 1 (STOP) when the application is stopping.

This gives the implementor of the plug-in the ability to provide a function that can do initialization and clean-up. Other applications that are using the odl parser library can invoke the entry points for other different reasons.

The entry points also open the possibility to have a different entry point for a different platform, so it will be possible to write the code once and use it on different platforms without changing it.

The only thing that needs to be changed is a single odl, that refers to another entrypoint.

To resolve an entry point a library name must be given followed by a function name. Here it is not possible to specify a function resolver. The entry point definition always uses the `import` resolver.

The library must be imported with [import](#import)

> NOTE  
>
>The C function implementation must match the following signature, if the C implementation does not match the signature the behavior is undefined when called.
>
> ```
> int <function-name>(int reason,
>                     amxd_dm_t *dm,
>                     amxo_parser_t *parser);
> ```

##### Entry point syntax

>Syntax:
>
> ```odl
> entry-point <name>.<name>;
>```

##### Entry point example

> Example:
>
> ```odl
> %define {
>    entry-point greeter.greeter_main;
> }
> ```

#### Define actions

Actions can be added in object and parameter definitions (in the object body definition or in the parameter body definition).

Actions are callback functions that are called for a specific reason. The function implementation will be fetched using one of the function resolvers. It is possible to add resolver instructions after the function name. Multiple callback functions for the same reason (action name) can be provided.

Some action implementations can requiere that data is provided.

The most common added action on objects and parameters is the `validate` action.

If private data is added to the object or parameter it is also common to add a `destroy` action.

More details about action implementation and how the actions must behave is described in the document [Actions, Transactions and Events](https://gitlab.com/soft.at.home/ambiorix/libraries/lib_amxd/-/blob/master/doc/actions_transactions_events.md) of the data model library (libamxd).

##### Action names

The possible action names are:

- `read` - called when the object or parameter needs to be read (values).
- `write`  - called when the object or parameter needs to be written.
- `validate`  - called when the object or parameter needs to be validated.
- `describe` - this is a special read action and only available on objects, only used for introspection
- `list` - this is a special read action and only available on objects, only used for introspection.
- `add-inst` - called when an instance needs to be added. This action can only be used on template objects
- `del-inst` - called when an instance needs to be deleted. This action can only be used on template objects.
- `destroy` - called when the object or parameter is going to be deleted. 


##### Action data

The data can be added behind the function name (or resolver instructions). The data can be a simple value like a number or text, or composite values like arrays or table (key value pairs). The data will be passed to the function as the private data and will always be a pointer to a variant.

##### Action syntax

>Syntax:
>
> ```odl
> on action <ACTION> call <function name> [<!RESOLVER:<RESOLVER-DATA>!>];
> on action <ACTION> call <function name> [<!RESOLVER:<RESOLVER-DATA>!>] [ <VALUE>, <VALUE>, ...];
> on action <ACTION> call <function name> [<!RESOLVER:<RESOLVER-DATA>!>] {
>             <key> = <VALUE>,
>             <key> = <VALUE>,
>             ...
> };
> on <ACTION> call <function name> [<!RESOLVER:<RESOLVER-DATA>!>] <VALUE>;
>```

##### Action Example

> ```odl
> %define {
>     object Greeter {
>         %read-only uint32 MaxHistory {
>             on action validate call check_minimum 1;
>             default 10;
>         }
>     }
> }
>   

#### Resolver Instructions

Everywere were a function name is provided, it is possible to provide function resolving instructions, except for [entrypoints](#define-entry-points)

In most cases it will not be required to specify a function resolver instruction. The default odl parser function resolving behavior is in most cases suficient. The behavior of the `import` and `auto` resolver can be modified using configuration options.

When specifiying a resolver, at least the resolver name must be given. Parsing of the odl file fails when a unknow resolver name is set.

The resolver definition starts with `<!` and ends with  `!>` and must be set behind the closing `)` of the argument list (blanks are allowed). Within the resolver definition, the first thing to mention is the resolver name which can not contain any blanks. The name must be ended with `:` or the closing `!>`.

The odl parser library provides three default function resolvers:

- **auto** - will call all resolvers in a defined order until a function is found or no more resolvers are availble. The order can be defined with the configuration option `auto-resolver-order` in the `%config` section.
- **ftab** - an application can build a function table using the odl parser library API, before starting the odl parsing itself. The ftab resolver (function table) will use this information to resolve the functions.
- **import** - the import resolver can load plug-ins (in shared object format using dlopen) and resolves the functions using dlsym. By default the import resolver will prefix the function names with `_<object name>_` or `_<param name>_` depending on the context. When the function is not defined in an object or parameter body the function name is prefixed with `_`.

### Section %populate

In the `%populate` section it is possible to:

- create instance of template objects and set the parameter values of these newly created instance
- change parameter values of defined singleton or template objects
- change some attributes of parameters and objects

Each `%populate` section starts with `%populate {` and ends with `}`.

***
> For backwards compatibility reasons (PCB ODL) the `%define` can be replaced with `datamodel`. The keyword `datamodel` is considered depricated and should not be used in new written odl files, unless they need to be used for PCB based applications as well.
***

Everything between the curly braces '{' and '}' is considered as the body of the `%populate` section.

#### %populate Syntax

>Syntax:
>
>```odl
> %populate {
> ...
> }
>```

#### %populate Example

> Example:
>
> ```odl
> %populate {
>     object Greeter.History {
>        instance add() {
>          ...
>        }
>     }
> }
> ```

#### Create instances

TODO

#### Change parameter values

TODO

#### Event Subscription

TODO

## Include

With `include` or `#include` other odl files can be included. Parsing of the include file is done first before continuing the current odl file (that contains the include).

Mandatory include are specified with `include`, optional includes with `#include`. When an optional include file is not available, parsing of the current odl continues. When a mandatory include file is not available, parsing stops with an error.

Includes can be done anywhere outside a section (`%config`, `%define`, `%populate`).

The name of the file must be put between double quotes (`"`) and can contain an absolute path or relative path. When a relative path is specified (not starting with `/`), the file is searched in the include dirs. (by default the current working directory). The include directories can be configured in the config option `include-dirs` (see [ODL parser configuration options](#odl-parser-configuration-options))

#### Include Syntax

>Syntax:
>
>```odl
> include "<odl file>";
> #include "<odl file>";
>```

#### Include Example

> Example:
>
> ```odl
> include "greeter-definitions.odl"
> #include "greeter-values.odl"
> ```

## Import

TODO

## Syntax Overview

- `<all small>` - file name (with or without absolute or relative path), name or text.
- `<ALL CAPS>` - replace with one of the values indicated in the section below with `ALL CAPS` name.
- `...` - more are allowed.
- `[something here]` - optional

***
> NOTE  
>
> - The square brackets '[' and ']' after an object name do not indicate an optional syntax, but indicates that the object is a template object.  
> - The symbols `<!` and `!>` do not indicate that you have to provide a keyword, name or text, but indicate function resolving `instructions` see [RESOLVER](#resolver) and [RESOLVER-DATA](#resolver-data)
***

```odl
// mandatory include
include "<include_file>";

// optional include
#include "<include_file>";

// config section
%config {
    // set a config option
    <config option> = <VALUE>;

    // set a config option using array of values
    <config option> = [ <VALUE>, <VALUE>, ... ];

    // set a config option using key-value pairs
    <config option> = { <key> = <VALUE>, <key> = <VALUE>, ... };
    ...
}

// define section
%define {
    // define a singleton object
    [<ATTRIBUTES>] object <name>;

    // define a singleton object with parameters, functions, objects
    [<ATTRIBUTES>] object <name> {
        // add an action callback without data
        on action <ACTION> call <name> [<!RESOLVER:<RESOLVER-DATA>!>];

        // add an action callback with data (array)
        on action <ACTION> call <name> [<!RESOLVER:<RESOLVER-DATA>!>] [ <VALUE>, <VALUE>, ...];

        // add an action callback with data (key - value pairs)
        on action <ACTION> call <name> [<!RESOLVER:<RESOLVER-DATA>!>] { <key> = <VALUE>, <key> = <VALUE>, ... };

        // add an action callback with data (simple)
        on action <ACTION> call <name> [<!RESOLVER:<RESOLVER-DATA>!>] <VALUE>;

        // define a parameter
        [<ATTRIBUTES>] <TYPE> <name> [= <VALUE>];

        // define a parameter with action callbacks
        [<ATTRIBUTES>] <TYPE> <name> [= <VALUE>] {
          // set the default value
          default <VALUE>;

          // add an action callback without data
          on action <ACTION> call <name> [<!RESOLVER:<RESOLVER-DATA>!>];

          // add an action callback with data (array)
          on action <ACTION> call <name> [<!RESOLVER:<RESOLVER-DATA>!>] [ <VALUE>, <VALUE>, ...];

          // add an action callback with data (key - value pairs)
          on action <ACTION> call <name> [<!RESOLVER:<RESOLVER-DATA>!>] { <key> = <VALUE>, <key> = <VALUE>, ... };

          // add an action callback with data (simple)
          on action <ACTION> call <name> [<!RESOLVER:<RESOLVER-DATA>!>] <VALUE>;

          ...
        }

        // define a function
        [<ATTRIBUTES>] <TYPE> <name>([ [<ATTRIBUTES>] <TYPE> arg [= <VALUE>], ...])[<!RESOLVER:<RESOLVER-DATA>!>];

        [<ATTRIBUTES>] object <name>;

        [<ATTRIBUTES>] object <name> {
            ...
        }

        ...
    }

    // define a template object
    // (the `[` and `]` must be put behind the name to indicate it is a template object)
    [<ATTRIBUTES>] object <name>[];

    // define a template object with parameters, functions, objects. 
    // (the `[` and `]` must be put behind the name to indicate it is a template object)
    [<ATTRIBUTES>] object <name>[] {
        [counted with <name>]
        // define a parameter
        [<ATTRIBUTES>] <TYPE> <name> [= <VALUE>];

        // define a function
        [<ATTRIBUTES>] <TYPE> <name>([ [<ATTRIBUTES>] <TYPE> arg [= <VALUE>], ...])[<!RESOLVER:<RESOLVER-DATA>!>];
        [<ATTRIBUTES>] object <name>;

        [<ATTRIBUTES>] object <name> {
            ...
        }

        ...
    }

    // define an entry point
    entry-point <name>.<name>;
}

// populate section
%populate {
    object <path>  {
        instance add(<index>,<name>) {
          [<SET ATTRIBUTES>] [<UNSET ATTRIBUTES>] parameter <name> = <VALUE>;
          ...
        }
        instance add() {
          [<SET ATTRIBUTES>] [<UNSET ATTRIBUTES>] parameter <name> = <VALUE>;
          ...
        }
        instance add(<index>,<name>,<key param>=<value>,<key param>=<value>, ...) {
          [<SET ATTRIBUTES>] [<UNSET ATTRIBUTES>] parameter <name> = <VALUE>;
          ...
        }
    }

    object <path> {
          [<SET ATTRIBUTES>] [<UNSET ATTRIBUTES>] parameter <name> = <VALUE>;
          ...
    }

    on event <regexp> call <name> [!RESOLVER:<RESOLVER-DATA!>];
    on event <regexp> of <regexp> call <name> [!RESOLVER:<RESOLVER-DATA!>];
    ...
}
```

### VALUE

- A single word. A single word must start with a alphabetic character (upper or lower case) followed with alphabetic or numeric characters, can contain `-` or `_`.
- Text. Text start with `"` (double quote) and ends with `"` (double quote) . In between the double quotes any character is allowed, double quotes inside the text must be escaped with a '\' (backslash)
- A number. A number can only contain numeric characters, a '-' sign can be put in front to indicate negative numbers. Numbers between `"` (double quotes) are considered text.
- A boolean. `true` or `false` or the only accepted values. When put between `"` (double quotes) the values are considered as text.

### ATTRIBUTES

Multiple attributes can be provided and must be separated with at least on blank (space, tab newline). Attribute names must always be prefixed with `%` or `!`. The `%` signs indicates that the attributes must be set and the `!` indicates that the attribute must be unset.

In the `%define` section attributes can only be set. In the populate section some attributes can be unset.

Valid attribute names are

- `%read-only` or `!read-only`
- `%persistent` or `!persistent`
- `%private` or `!private`
- `%template`
- `%instance`
- `%volatile` or `!volatile`
- `%in`
- `%out`
- `%mandatory`
- `%strict`
- `%key`

The notation without pre-pending the attribute name with '%' is considered deprecated.

Deprecated attribute names are:

- `read-only` - is the same as `%read-only`
- `persistent` - is the same as `%persistent`
- `template-only` - is the same as `%template`
- `volatile` - is the same as `%volatile`

Depending on the context where attributes are used only a subset of the attributes will be valid:

#### Valid object attributes

- `%read-only` or `!read-only`
- `%persistent` or `!persistent`
- `%private` or `!private`

For more information about these attributes on object see section [Define objects](#define-objects)

#### Valid parameter attributes

- `%read-only` or `!read-only`
- `%persistent` or `!persistent`
- `%private` or `!private`
- `%template`
- `%instance`
- `%volatile` or `!volatile`
- `%key`

For more information about these attributes on object see section [Define parameters](#define-parameters)


#### Valid function attributes

- `%private` or `!private`
- `%template`
- `%instance`

#### Valid function argument attributes

- `%in` - the argument is an in argument (caller to callee)
- `%out` - the argument is an out argument (callee to caller)
- `%mandatory` - the argument is mandatory, a function invocation without all mandatory arguments provided fails (this attribute is ignored on `out` only arguments)
- `%strict` - the caller must provide the argument with the correct type, if the type does not match the defined type, the function invocation fails.

### TYPE

The supported types are:

- void
- string
- int32
- int64
- uint32
- uint64
- bool
- list
- htable
- fd
- variant
- double

These types can be used on functions (function return value) and function arguments.
For parameter definitions not all types are supported. The not allowed types in parameter definitions are:

- list (complex type)
- htable (complex type)
- fd (file descriptor)

### ACTION

Most of the time it is not needed to set action callback functions, but in some cases it can be helpful or handy. One such case is parameter value validation or object content validation.

The valid action names are:

- `read` - can be used on parameters and objects
- `write`  - can be used on parameters and objects
- `validate`  - can be used on parameters and objects
- `describe` - can be used on objects
- `list` - can be used on objects
- `add-inst` - can be used on objects
- `del-inst` - can be used on objects
- `destroy` - can be used on parameters and objects

Deprecated action descriptions

```
constraint minvalue <VALUE>;
constraint maxvalue <VALUE>;
constraint range [ <VALUE>, <VALUE> ];
constraint enum [ <VALUE>, <VALUE>, ... ];
constraint custom <name>;
constraint custom <name> using <name>;
```

These can be replaced with (same order)
```
on action validate check_minimum <VALUE>;
on action validate check_maximum <VALUE>;
on action validate check_range { min = <VALUE>, max = <VALUE> };
on action validate check_enum [ <VALUE>, <VALUE>, ... ];
on action validate <name>;
on action validate <name> <!RESOLVER:<RESOLVER-DATA>!>;
```

### RESOLVER

The ambiorix odl parser provides 3 resolvers:

- **auto** - the default if no resolver is specified, this resolver will iterator over all resolvers in the specified order (see config option `auto-resolver-order`) until a resolver returns a function.
- **ftab** - the function table resolver, the function table can be filled with functions using `amxo_resolver_ftab_add`
- **import** - loads shared objects (so files) and uses `dlsym` to resolve the functions. Share objects can be loaded using the `import` keyword in the odl, or by calling `amxo_resolver_import_open` in your code.

Other resolver can be added to the function resolving system using `amxo_register_resolver` or removed using `amxo_unregister_resolver`.

By specifying the resolver name behind a function, the odl parser will only use the resolver specified.

Adding resolver information is optional, and is not needed when the function name in odl file can be resolved automatically.

***

> NOTE
> - The import resolver will unload all shared objects of which no functions are used after parsing the odl files. 

***

### RESOLVER-DATA

The resolver data is depending on the resolver itself. See the documentation of the specific resolver.

The default provide function resolvers are documented here

#### Auto resolver data

The auto resolver does not take any data. This is the default function resolver.

This resolver does not tries to resolve to function names to a function pointer by itself, it will use the other registered function resolvers in a pre-defined order. The first function resolver that returns a function pointer will be used.

The order of the resolvers can be configured in the `%config` section using configuration option `auto-resolver-order`

#### Ftab resolver data

The ftab resolver takes as data the function name as it is registered in the function table.
This data is optional, if the function name in the odl is the same as the function name in the function table, it is not needed to specify the function name.

The data syntax:

```odl
<!ftab[:function_name]!>
```

Example:

This examples shows how to instruct the odl parser to use the function table resolver and search for the function `_function_dump`.

```odl
variant echo(%in %mandatory variant message)<!ftab:_function_dump!>;
```

While in this example, the odl parser is instructed to use the function table resolver and search for the function `echo`

```odl
variant echo(%in %mandatory variant message)<!ftab!>;
```

In both examples, the **ftab** resolver is the only resolver used. If no function pointer is returned by the **ftab** resolver, the function is considered unresolved.

Warnings or extra information is dumped to `stdout` of your application when a function can not be resolved.

> NOTE
>
> The **ftab** resolver provides some parameter validation callback functions that can be used in parameter validation action definitions `on action validate call ....`.

The function names that are available are: 

- check_minimum
- check_minimum_length
- check_maximum
- check_maximum_length
- check_range
- check_enum

#### Import resolver data

TODO

## ODL keywords

None of the keywords can be used as a name or as a single word, but can be used in text (between double quotes `"`).

The keywords are grouped and in alphabetic order.

__Language__

- `action:`
- `add`
- `as`
- `call`
- `constraint` (DEPRECATED - use `on action validate`)
- `counted`
- `default`
- `entry-point`
- `enum` (DEPRECATED - use `on action validate check_enum` )
- `event:`
- `false`
- `include`
- `#include`
- `import`
- `instance`
- `maxvalue`  (DEPRECATED - use `on action validate check_maximum` )
- `minvalue`  (DEPRECATED - use `on action validate check_minimum` )
- `object`
- `of`
- `on`
- `parameter`
- `range` (DEPRECATED - use `on action validate check_range` )
- `using` (DEPRECTATED - use `import`)
- `true`
- `with`

__Import Flags__

- `RTLD_GLOBAL`
- `RTLD_NOW`

__Section Keywords___

- `%config`
- `datamodel` (DEPRECATED - use `%populate`)
- `%define`
- `%populate`


__Attribute Keywords__

- `%in`
- `%instance`
- `%mandatory`
- `mandatory`  (DEPRECATED - use `%mandatory`)
- `%out`
- `%persistent`
- `!persistent`
- `persistent` (DEPRECATED - use `%persistent`)
- `%private`
- `!private`
- `%read-only`
- `!read-only`
- `read-only` (DEPRECATED - use `%read-only`)
- `%template`
- `template-only` (DEPRECATED - use `%template` and do not specify `%instance`)
- `%volatile`
- `!volatile`

__Type Keywords__

- `bool`
- `double`
- `fd`
- `htable`
- `int32`
- `int64`
- `list`
- `void`
- `string`
- `uint32`
- `uint64`
- `variant`

__Action Name__

- `read` - can be used on parameters and objects
- `write`  - can be used on parameters and objects
- `validate`  - can be used on parameters and objects
- `describe` - can be used on objects
- `list` - can be used on objects
- `add-inst` - can be used on objects
- `del-inst` - can be used on objects

## PCB Compatibility

This odl parser is capable of parsing PCB odl files, but the other way around is not always possible.

If you need to write an odl file that is usable with the Ambiorix ODL parser and with the PCB ODL parser at the same time then follow these simple rules:

- Optional includes are not possible (`#include`), only mandatory includes (`include`)
- Do not use the keyword `import`, use `uses` instead.
- Multiple `uses` are not allowed in one odl file
- `includes` and `uses` must be at the top of the odl file.
- Do not use `%config` sections
- Do not put object definitions in a section (`%define)
- Use `datamodel` instead of `%populate`
- Do not define entry-points (`%define` section)
- Do not subscribe for events (`%populate` section)
- Do not use `RESOLVER` instructions.
- Specify attributes without `%` or `!`
- Do not specify a shebang (`#!`) at the first line