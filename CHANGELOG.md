# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]


## Release v1.4.9 - 2021-11-25(12:05:53 +0000)

## Release v1.4.8 - 2021-11-22(15:54:15 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.4.7 - 2021-11-18(13:02:17 +0000)

### Fixes

- Fixes regression due to adding any action

## Release v1.4.6 - 2021-11-18(11:11:24 +0000)

### Changes

- Make it possible to set an action callback for all actions

## Release v1.4.5 - 2021-11-16(17:49:50 +0000)

### Fixes

- Missing brackets in function resolver data causes segmentation fault

### Changes

- The function name must be passed as private data to subscriptions taken from an odl file
- Update dependencies in .gitlab-ci.yml

## Release v1.4.4 - 2021-11-10(15:10:07 +0000)

### Changes

- ODL parser should pass function type to resolvers

## Release v1.4.3 - 2021-11-10(13:07:44 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.4.2 - 2021-10-28(22:33:47 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.4.1 - 2021-10-20(19:06:30 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.4.0 - 2021-10-15(09:39:00 +0000)

### New

- Introduces function to add wait-for-write fd to event loop

## Release v1.3.10 - 2021-10-08(13:20:19 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.3.9 - 2021-10-08(10:48:07 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.3.8 - 2021-10-08(07:45:03 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.3.7 - 2021-09-24(15:50:00 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.3.6 - 2021-09-23(09:59:47 +0000)

### Fixes

- Saved odl files with mib extensions can not be loaded
- it must be possible to indicate that an instance parameter must be saved in the header

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.3.5 - 2021-09-07(05:56:13 +0000)

## Release v1.3.4 - 2021-08-23(11:05:21 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.3.3 - 2021-08-02(12:20:05 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.3.2 - 2021-07-22(11:23:49 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.3.1 - 2021-07-12(17:26:11 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.3.0 - 2021-07-12(09:30:38 +0000)

### New

- Make it possible to declare required objects from remote processes

## Release v1.2.11 - 2021-07-09(09:30:26 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.2.10 - 2021-07-05(06:53:57 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.2.9 - 2021-07-04(17:22:45 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.2.8 - 2021-07-02(18:58:30 +0000)

### Fixes

- Generation of version.h files should not be .PHONY

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.2.7 - 2021-06-28(12:56:25 +0000)

### Fixes

- ODL parser is sending the add (intstance) events in the wrong order

### Changes

- Make it possible to load and keep modules when no symbols are resolved
- Update dependencies in .gitlab-ci.yml

### Other

- Issue: ambiorix/libraries/libamxo#59 After loading post-includes data model eventing is disabled

## Release v1.2.6 - 2021-06-21(07:53:30 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.2.5 - 2021-06-18(22:50:10 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.2.4 - 2021-06-15(08:43:53 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.2.3 - 2021-06-11(09:51:54 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.2.2 - 2021-06-11(05:39:00 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.2.1 - 2021-06-10(18:18:57 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.2.0 - 2021-06-10(12:18:46 +0000)

### New

- A hook must be added for counter parameters

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.1.7 - 2021-06-08(09:09:25 +0000)

### Fixes

- [tr181 plugins][makefile] Dangerous clean target for all tr181 components

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.1.6 - 2021-06-03(08:06:12 +0000)

- TM NET Build issue with libamxo

## Release v1.1.5 - 2021-06-01(07:06:21 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.1.4 - 2021-05-31(09:31:58 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.1.3 - 2021-05-21(12:02:37 +0000)

### Fixes

- Recursive includes when parsing odl files can cause a segmentation fault
- Extend event filter parser to resolve variables

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.1.2 - 2021-05-12(15:29:21 +0000)

- Issue: ambiorix/libraries/libamxo#50 When a new instance is added the correct index and name of the object must be passed to the hook function

## Release v1.1.1 - 2021-05-09(20:32:00 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.1.0 - 2021-05-04(08:00:16 +0000)

### New

- Comments in an odl file must be extracted and passed to a hook functions

### Fixes

- ODL populate section does not resolve config variables

### Changes

- Use common macros from libamxc
- Removes core dumps
- Update dependencies in .gitlab-ci.yml

### Other

- Enable auto opensourcing

## Release v1.0.4 - 2021-04-23(18:44:40 +0000)

### Changes

- Update dependencies in .gitlab-ci.yml

## Release v1.0.3 - 2021-04-21(09:10:31 +0000)

### Fixes

- Save persistent protected and private parameters

### Changes

- Add configuration option to disable function resolving
- It must be possible to add user flags to functions

## Release 1.0.2 - 2021-04-15(20:07:54 +0000)

### Changes

-  remove fakeroot dependency on host to build WRT image 

## Release 1.0.1 - 2021-04-15(10:57:27 +0000)

### Fixes

- parsing multiple odl files with import causes a segmentation fault 

## Release 1.0.0 - 2021-04-08(20:40:58 +0000)

### Fixes

- Loading empty directory must return 2 (not found
- When including an empty directory the parser must fail
- When saving an odl file all names must be quoted

### Changes

- Move copybara to baf

## Release 0.8.1 - 2021-03-31(13:58:19 +0000)

### Changes

- Make it possible to call entry-points in reverse order

## Release 0.8.0 - 2021-03-29(17:35:49 +0000)

### New

- Add amxo_parser_load_mib function

### Change 

- Always save persistent instance regardless of template object attributes

## Release 0.7.2 - 2021-03-26(23:31:22 +0000)

### New 

- it must be possible to include a directory
- It must be possible to add and remove flags to parameters in odl files

### Changes

- Rename keyword flags to userflags

### Fixes

- Clean-up resolvers after reading odl files
- Send dm:instance-added events when creating instances from odl file

## Release 0.7.1 - 2021-03-10(12:09:54 +0000)

### Changes

- Step-up versions of libamxc, libamxp and libamxd for CI
- The function table resolve should provide a function name to default add instance action
- Handle data model events before post includes
- Removes commented code

## Release 0.7.0 - 2021-02-25(13:54:55 +0000)

### New 

- It must be possible to include odl files that are loaded after invoking the entry points

### Changes

- Migrate to new licenses format (baf)

## Release 0.6.7 - 2021-02-14(08:19:12 +0000)

### Changes

- Step-up versions of libamx,libamxp,libamxd for CI

## Release 0.6.6 - 2021-01-31(16:10:30 +0000)

### New

- It must be possible to get a list of open listen sockets

## Release 0.6.5 - 2021-01-28(09:03:33 +0000)

### Changes

- Update versions of libamxc,libamxp,libamxd for CI/CD pipelines

### Fixes

- correct documentation error on "on action validate"

## Release 0.6.4 - 2021-01-18(17:26:45 +0000)

### New

- Generate makefiles using build agnostic file (baf)

### Fixes

- Only include objects in static library

## Release 0.6.3 - 2021-01-08(15:13:29 +0000)

### Fixes

- Fixes listen sockets

## Release 0.6.2 - 2021-01-04(15:14:17 +0000)

### New

- Support for listen and accept sockets

### Fixes

- PCB odl parsing incompatibility
- Conditional jump or move depends on uninitialised value(s)

## Release 0.6.1 - 2020-11-30(16:08:44 +0000)

### Changes

- Update makefiles

### Fixes

- Adds in attribute for fuinction arguments if no in and no out is set

## Release 0.6.0 - 2020-11-29(16:54:54 +0000)

### Changes

- Update dependency versions in gitlab CI yml file

## Release 0.5.5 - 2020-11-25(19:28:31 +0000)

### Fixes

- Fix debian package dependencies

### Changes

- Update readme

## Release 0.5.4 - 2020-11-16(12:50:03 +0000)

### Changes

- Stores connection uri
- Updates gitlab CI/CD yml file

## Release 0.5.3 - 2020-11-01(21:55:43 +0000)

### Changes

- Add function to lookup connection context by file descriptor

## Release 0.5.2 - 2020-10-28(17:49:02 +0000)

### Fixes

- Set protected attribute as valid for parameters, functions and objects

## Release 0.5.1 - 2020-10-28(05:48:23 +0000)

### Fixes

- Paths in directory configuration list can contain variables
- Not all toolchain support secure_getenv
- Remove mibs using mib expression

## Release 0.5.0 - 2020-10-19(19:29:29 +0000)

### New 

- Scan mib dir feature
- Apply mib feature
- Protected attribute for objects, functions and parameters

### Changes

- Updates odl documentation - adds protected keyword and attribute

## Release 0.4.1 - 2020-10-14(07:25:44 +0000)

### New

- Add support for 8 and 16 bit integers

### Fixes

- Fixes segmentation fault when empty filename is passed to odl parser

## Release 0.4.0 - 2020-10-02(14:08:25 +0000)

### Changes

- Update code style
- Removed dead code

## Release 0.3.4 - 2020-09-17(20:44:37 +0000)

### New

- Make it possible to extend object definitions
- Adds select keyword for %define section

## Release 0.3.3 - 2020-09-17(13:45:59 +0000)

### Fixes

- ODL parser behavior configuration option names are not in line with other configuration options
- When defining a function or parameter in an existing instance object the %instance attribute must be set automatically

### Changes

- Update ODL documentation


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
