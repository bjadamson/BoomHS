# BoomHS
This repository contains the source code for the <b>BoomHS</b> project. BoomHS is a project in
active development.

![Alt text](/screenshots/22.png?raw=true "02/05/2018")

## Table of contents

  + [Getting Started](#getting-started)
  + [Project Information](#project-information)
    * [Install Dependencies](#install-dependencies)
    * [Bootstrapping](#bootstrap-the-project)
    * [Compiling](#compiling-the-project)
    * [Run the Project](#run-the-project)
  + [Other Information](#other-information)
  + [Hints](#hints)
  + [Screenshots](#screenshots)

## Getting Started
The easiest way to get started is to use the installation script to install required libraries. Run
the following commands to install dependencies, 
```bash
git clone https://github.com/bjadamson/BoomHS.git
cd BoomHS
scripts/install-dependencies.bash
scripts/bootstrap.bash
scripts/build.bash
scripts/build-and-run.bash
```

## Project Information
### Install dependencies.
To install external dependencies, we rely on a script (for reproducability) so we can reliably
setup builds that work on new machines. The intent is to make it easy to startup development on a
new machine quickly. <i>Today</i> this is a goal, not a reality.
```bash
scripts/install-dependencies.bash
```

### Bootstrap the project
Bootstrapping the project is handled through a script. This script takes care of enumerating all
the source files in the project and telling cmake about them. It also takes care of finding the
compiler on your system. It then invokes cmake with the list of source file (and external libraries
defined in the script as well).
```bash
scripts/bootstrap.bash
```

### Compiling the Project
Compiling is easy, all the hard work was done during the bootstrapping process. Compiling the
project is as easy as running the build script.

```bash
scripts/build.bash
```

#### Important Note
When adding a new source file to the project, you must run the bootstrap script again.

### Run the project
Running the project is easy too (yep you guessed it) if your using the command line, you just need
to run the build-and-run script.
```bash
scripts/build-and-run.bash
```

## Other Information
  + Source-code formatting. The clang-formatter tool is used for this purpose.
```bash
scripts/code-format.bash
```

## HINTS
### The following symlinks in the project's root directory exist to make running the scripts easier
on the fingers (from the command line). You can see the symlink for yourself in the root directory.
  + bb  => build the project
  + bbc => clean the project (requires bootstrapping again)
  + bbr => build and RUN the project
  + bbf => run the source code formatter.
  + bbk => kill the running game process (parses PID table, unstable).

## Screenshots
![Alt text](/screenshots/21.png?raw=true "01/25/2018")
![Alt text](/screenshots/20.png?raw=true "01/25/2018")
![Alt text](/screenshots/18.png?raw=true "01/25/2018")
![Alt text](/screenshots/17.png?raw=true "01/24/2018")
![Alt text](/screenshots/16.png?raw=true "01/24/2018")
![Alt text](/screenshots/15.png?raw=true "01/10/2018")
![Alt text](/screenshots/14.png?raw=true "01/02/2018")
![Alt text](/screenshots/13.png?raw=true "01/02/2018")
![Alt text](/screenshots/12.png?raw=true "12/29/2017")

#### links
* https://github.com/cbaggers/cepl
