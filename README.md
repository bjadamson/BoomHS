# BoomHS
Boom Headshot?!
<p>
  This repository contains the source code for the <b>BoomHS</b> project. BoomHS is a project in
  active development.
</p>

#![Alt text](/screenshots/17.png?raw=true "01/24/2018")

## Table of contents

  + [Getting Started \(quick-guide\)](#getting-started quick-guide)

  + [Getting Started (longer-guide)]
    * [Install Dependencies](#install-dependencies)
    * [Bootstrapping](#bootstrap-the-project)
    * [Run the Project](#run-the-project)
  + [Notes](#notes)
  + [Screenshots](#notes)

## Getting Started (quick-guide)
```bash
git clone https://github.com/bjadamson/BoomHS.git
cd BoomHS
scripts/install-dependencies.bash
scripts/bootstrap.bash
scripts/build.bash
scripts/build-and-run.bash
```

## Getting Started (longer-guide)
### Install dependencies.
  + On a bash system, simply invoke the auto-installer script like so:
    * scripts/install-dependencies.bash
  + or
    * sudo scripts/install-dependencies.bash

#### Bootstrap the project
  + Invoke the bootstrap script
    * scripts/bootstrap.bash

##### Run the project
  + Invoke the build-then-run script
    * scripts/bbr.bash

## NOTES
### The following symlinks in the project's root directory exist to make running the scripts easier
on the fingers (from the command line). You can see the symlink for yourself in the root directory.
  + bb  => build the project
  + bbc => clean the project (requires bootstrapping again)
  + bbr => build and RUN the project
  + bbf => run the clang formatter on the source code.

#### **IMPORTANT** When adding a new source file to the project, you must run the bootstrap script
   again.
  + Appoplogies ahead of time on this one, but with the way cmake is setup, right now running the
    bootstrapping process again is required to "pickup" the new source file.

## Screenshots
![Alt text](/screenshots/16.png?raw=true "01/24/2018")
![Alt text](/screenshots/15.png?raw=true "01/10/2018")
![Alt text](/screenshots/14.png?raw=true "01/02/2018")
![Alt text](/screenshots/13.png?raw=true "01/02/2018")
![Alt text](/screenshots/12.png?raw=true "12/29/2017")
![Alt text](/screenshots/11.png?raw=true "12/29/2017")
![Alt text](/screenshots/10.png?raw=true "12/21/2017")

#### links
* https://github.com/cbaggers/cepl
