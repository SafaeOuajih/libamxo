# Introduction

First off, thank you for considering contributing to lib_amxo and the [Ambiorix](https://gitlab.com/soft.at.home/ambiorix) project. It's people like you that makes [Ambiorix](https://gitlab.com/soft.at.home/ambiorix) such a great set of tools and libraries.

Following these guidelines helps to communicate that you respect the time of the developers managing and developing this open source project. In return, they should reciprocate that respect in addressing your issue, assessing changes, and helping you finalize your merge requests.

All members of our community are expected to follow our [Code of Conduct](https://gitlab.com/soft.at.home/ambiorix/ambiorix/blob/master/doc/CODE_OF_CONDUCT.md). Please make sure you are welcoming and friendly in all of our spaces.

# What can I contribute?

lib_amxo is a part of the [Ambiorix](https://gitlab.com/soft.at.home/ambiorix) project. All parts of [Ambiorix](https://gitlab.com/soft.at.home/ambiorix) are open source projects and we love to receive contributions from our community — you!

There are many ways to contribute, from writing tutorials or blog posts, improving the documentation, submitting bug reports and feature requests or writing code which can be incorporated into lib_amxo or the [Ambiorix](https://gitlab.com/soft.at.home/ambiorix) project itself.

Please, don't use the issue tracker for `support questions`. You can send an e-mail to one of the maintainers with your questions, Stack Overflow is also worth considering.

Contributing to the repositories can be done, make sure an issue is created in the [issue tracker](https://gitlab.com/soft.at.home/groups/ambiorix/-/issues)

# Your First Contribution

Unsure where to begin contributing to Ambiorix? You can start by looking through the documentation and help-wanted issues:

- documentation: Most of the documentation is written by none native English speaking people, reviewing the documentation and fixing grammar mistakes, or just re-write parts can help in make it all more understandable.
- help-wanted: Help wanted issues are very specific and some-one is needed with a good knowledge of that specific area.

# Contributions to lib_amxo

## Getting started

Fork the main (called `upstream`) repository to a private repository. 

After forking you have your own copy of the repository, available at `https://gitlab.com/soft.at.home/<USER>/lib_amxo` where `<USER>` is your user name on GitLab.

Clone your fork on your computer (replace `<USER>` by your gitlab user name):

```bash
git clone git@gitlab.com/soft.at.home:<USER>/lib_amxo.git
```

Add `upstream` as a remote called `upstream`:

```bash
git remote add upstream git@gitlab.com/soft.at.home:ambiorix/libraries/lib_amxo.git
```

## For Each Contribution

### On this repository (using gitlab ui)

- Select the issue from the issue list you want to contribute to.
- Create a new branch for the issue
  - Select `create branch` and push the button
  - Do not change the name of the branch

### On your local clone (using a shell console)

- Fetch new stuff from `upstream`

```bash
git fetch upstream
```

- Make sure you're on the newly created branch:

```bash
git checkout <branchname>
```

- Update your branch so it's in sync with `upstream/<branchname>`:

- Write code, create commits etc on your forked repository. (Push on origin not upstream)

### When done

- Push your changes upstream (make sure you are pushing the correct branch)

```bash
git push upstream
```

- make sure that the pipeline on the branch succeeds, if not fix it before continuing.

- Create a merge request to `upstream` (using gitlab ui)
  - Make sure that the gitlab ci/cd pipeline succeeds, if not it will not be possible to merge your changes in the `upstream` project 
  - For new features/new code, make sure the code is covered by tests

Wait for the CI to be completed, if everything is going well, it's ready to merge.
Otherwise, you can update your branch (in your forked project), the MR will be updated automatically.

## Contribution Rules

- Code changes should be tested - the README.md explains how to run tests
- Always start from an issue, check the [issue tracker](https://gitlab.com/soft.at.home/groups/ambiorix/-/issues). If no issue exists for your new feature/bug fix, just create one.
- Always document public API
- Make sure new code is following the [coding guidelines](https://gitlab.com/soft.at.home/ambiorix/ambiorix/blob/master/doc/CODING_GUIDELINES.md)

## No issue tracking needed for small contributions

As a rule of thumb, changes are obvious fixes if they do not introduce any new functionality or creative thinking. As long as the change does not affect functionality, some likely examples include the following:

- Spelling / grammar fixes
- Typo correction, white space and formatting changes
- Comment clean up
- Extend/complete documentation

# How to report a bug

When filing an issue, make sure to answer these five questions:

1. What operating system and processor architecture are you using?
1. What did you do?
1. What did you expect to see?
1. What did you see instead?

# How to suggest a feature or enhancement

Do you have great idea's for amazing features, first check the issue tracker to see if someone else already added such a feature request.

If you find yourself wishing for a feature that doesn't exist in Ambiorix (in general) or lib_amxo (specific), you are probably not alone. Open an issue on our issues list on GitLab which describes the feature you would like to see, why you need it, and how it should work.

# Code review process

The core team looks at Merge Requests on a regular basis in a weekly triage meeting.
After feedback has been given we expect responses within two weeks. After two weeks we may close the merge request if it isn't showing any activity.
