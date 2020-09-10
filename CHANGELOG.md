# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## Release 0.3.2 - 2020-09-03(14:13:25 +0000)

### Fixes

- Pass version_prefix to make command

## Release 0.3.1 - 2020-09-03(06:13:25 +0000)

### Fixes

- Fixes g++ compilation warnings and errors

### Changes

- Add version prefix to support legacy build system

## Release 0.3.0 - 2020-08-29(21:21:13 +0000)

### Changes

- Needs libamxd v1.0.0 or higher
- Apply API changes of libamxd
- Removes uneeded references to libamxb in unit tests

## Release 0.2.7 - 2020-08-23(15:52:22 +0000)

### New

- Makes new parameter validation and read action implementations available in ODL

## Release 0.2.6 - 2020-08-20(13:03:39 +0000)

### Fixes

- set correct MAJOR number on libraries for internal builds

## Release 0.2.5 - 2020-08-16(10:04:33 +0000)

### Fixes

- Fixes fetching next connection if last was reached

## Release 0.2.4 - 2020-08-13(13:35:41 +0000)

### New

- Event names can be added in object body definitions

### Fixes

- `%unique` parameter attribute is not taken into account

## Release 0.2.3 - 2020-08-04(05:51:11 +0000)

### Changes

- Update contributing guide

### New

- Support for unique key

## Release 0.2.2 - 2020-07-27(11:42:22 +0000)

### Fixes

- Fixes string variable resolving
- list action must not be in quotes
- Fixes memory leak

## Release 0.2.1 - 2020-07-24(12:24:01 +0000)

### Changes

- ODL Syntax: regular expressions must be explicitly indicated using "regexp()"
- Import resolver also checks none-prefixed symbols 

### Fixes

- Only delete action data if the action is owned by the parameter or object on which it is executed.

## Release 0.2.0 - 2020-07-22(18:49:14 +0000)

### New

- Save data model or configuration options API's
- Configuration options can be declared global
- Parser configuration options to change behavior of the odl parser
- Conditional includes

### Changes

- Adds RAW_VERSION to makefile.inc, VERSION must be X.Y.Z or X.Y.Z-HASH
- Updates tests, renamed macro YY_ASSERT macro
- Completes odl parsing tests

### Fixes

- Compilation issue with fortified  musl
- Too long functions
- Scanbuild warnings
- Memory leak (detected with unittests)
- Object type detection

## Release 0.1.3 - 2020-07-16(09:27:35 +0000)

### Changes

- Full path can be added to function name when registering functions to ftab resolver
- Documentation update (ftab API)
- Documentation update (odl)

### New

- Supports %key attributes for parameters

## Release 0.1.2 - 2020-07-13(06:19:29 +0000)

### Changes 

- Use new string split API from libamxc
- Adds alias names for parameter validation functions
- Update README.md
- Updates make files for SAH legacy build system

### Fixes

- Fixes ignoring return value of chdir
- Key-values inconsistencies

## Release 0.1.1 - 2020-07-06(09:12:12 +0000)

### Fixes

- Fixes tests - take new event data structure into account

## Release 0.1.0 - 2020-07-05(16:59:09 +0000)

### Changes

- Uses logical expression (libamxp) for event filtering instead of regexp filtering
- Uses std=c11 instead of std=c18
- Update odl.md

## Release 0.0.11 - 2020-06-30(07:45:20 +0000)

### Changes

- Scrubs Component.* files

## Release 0.0.10 - 2020-06-29(16:22:07 +0000)

### New 

- Support for legacy SAH build system

## Release 0.0.9 - 2020-06-26(18:01:12 +0000)

### New 

- Support for csv_string and ssv_string variant type
- Copybara file

### Changes

- Build libraries in target specific output directory

## Release 0.0.8 - 2020-06-23(16:54:11 +0000)

### Fixes

- Object attributes

## Release 0.0.7 - 2020-06-22(10:48:58 +0000)

### New

- Define mib objects
- Extend objects with mibs (define and populate section)
- Define datetime type
- Datetime type can be used as function return type, parameter type and function argument type

## Release 0.0.6 - 2020-06-16(11:08:07 +0000)

### New

- Event subscription callback with filter
- Validates objects and parameters while parsing
- Adds deprecated "write with" construct
- Support for complex types in config section
- Adds pcb backwards compatibility (constraint keyword)
- Adds PCB compatibility actions on parameters and objects
- Adds actions to objects
- Adds actions for parameters and set default in definition block

### Changes

- Extends connections API
- Extends valid odl test
- Updates odl documentation
- update license to BSD+patent

### Fixes

- Ignore sign-compare in generated flex file

## Release 0.0.5 - 2020-06-04(13:09:15 +0000)

### Fixes

- Fixes compiler error (musl)
- Fixes pipeline testing (first compile lib)

### New

- Creates unit test results file (ELK)
- Adds more tests and fixes
- Adds API documentation

## Release 0.0.4 - 2020-05-28(06:46:08 +0000)

### New

- ODL documentation in markdown
- Automatic instance counter definition

### Fixes

- Function override in definition, does not fail, provides warning
- PCB ODL compatibility

## Release 0.0.3 - 2020-05-26(10:29:37 +0000)

### New 

- Adds support for shebang
- Single line comments can now also start with '#'

## Release 0.0.2 - 2020-05-26(07:27:23 +0000)

### New

- Complete Import resolver
- Definition of entry points
- Adds dlopen flags support

### Changes

- Removes duplicate hook function typedefs

## Release 0.0.1 - 2020-05-24(09:59:17 +0000)

### New

- Tests
- Line tracking
- Auto-resolve order option - support for '*' (any)
- Less string copy and memory allocation during parsing (flex/bison)
- Parser hooks
- Adds parser hooks
- Recursive include protection
