# BoomHS
Boom Headshot?!

#### Required Dependencies (this is not an exhaustive list AT ALL)
1. sudo apt-get intall libc++-dev

#### Getting Started
1. Install dependencies.
  + On a bash system, simply invoke the auto-installer script like so:
    * scripts/install-dependencies.bash
  + or
    * sudo scripts/install-dependencies.bash

2. Bootstrap the project (setup cmake)
  + Invoke the bootstrap script
    * scripts/bootstrap.bash

3. Run the project
  + Invoke the build-then-run script
    * scripts/bbr.bash

#### NOTES
1. The following symlinks in the project's root directory exist to make running the scripts easier
on the fingers (from the command line). You can see the symlink for yourself in the root directory.
  + bb  => build the project
  + bbc => clean the project (requires bootstrapping again)
  + bbr => build and RUN the project
  + bbf => run the clang formatter on the source code.

2. **NOTE ME** When adding a new source file to the project, you must run the bootstrap script
   again.
  + Appoplogies ahead of time on this one, but with the way cmake is setup, right now running the
    bootstrapping process again is required to "pickup" the new source file.

#### Screenshots (some)
![Alt text](/screenshots/17.png?raw=true "01/24/2018")
![Alt text](/screenshots/16.png?raw=true "01/24/2018")
![Alt text](/screenshots/15.png?raw=true "01/10/2018")
![Alt text](/screenshots/14.png?raw=true "01/02/2018")
![Alt text](/screenshots/13.png?raw=true "01/02/2018")
![Alt text](/screenshots/12.png?raw=true "12/29/2017")
![Alt text](/screenshots/11.png?raw=true "12/29/2017")
![Alt text](/screenshots/10.png?raw=true "12/21/2017")

#### links
* https://github.com/cbaggers/cepl
