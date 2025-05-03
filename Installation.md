# Simple Vrui Installation

Follow these instructions to install Vrui with standard settings in a standard 
location, to simplify building and installing add-on packages and Vrui 
applications.

In the following, when asked to enter something into a terminal, each line you 
are supposed to enter starts with a `$` denoting the terminal's command prompt. 
Do *not* enter that `$`, but enter everything that follows, and end each line by 
pressing the Enter key.

## Step 0: Download The Vrui Repository From GitHub

If you are reading this, you probably have already done this. :) If not, 
download the entire Vrui repository from github, either by cloning the 
repository using `git clone`, or by downloading and unpacking a zip file.

### Downloading And Unpacking A Zip File From GitHub

If you are unfamiliar with git and/or GitHub, you should probably go the zip 
file route. On the Vrui repository's main page, 
`https://github.com/vrui-vr/vrui`, click on the green "<> Code" button, and 
then click on "Download ZIP" in the menu that pops up in response. Depending on 
your browser settings, you may be asked where to store the file being 
downloaded, or it might be stored in a default location such as your 
`Downloads` directory. Either way, take note of what the zip file is called, 
and where it is stored.

Then, once the file is completely downloaded, enter the following multiple 
lines into a terminal window:
    $ cd ~
    $ mkdir src
    $ cd src

These lines will create a new directory `src` in your home directory (`~` is a 
shortcut for your home directory), and enter into that new directory. Follow that 
with:
    $ unzip <path to downloaded zip file>
where you replace `<path to downloaded zip file>` with the full path to the zip file, 
for example `~/Downloads/vrui-main.zip`.

Finally, check for the name of your new Vrui directory by entering:
    $ ls
which will list all files in the new `src` directory, which should at this 
point be only the Vrui directory. It should be called something like 
`vrui-main`. Take note of this name for the next step.

## Step 1: Install Prerequisite Packages

Vrui uses a relatively large set of system-provided packages to implement its 
functionality. Some of these are essential, some are optional, some are *very* 
optional. See the README file for the complete list of prerequisite system 
packages. Ideally, you should install the full set of packages to unlock all 
of Vrui's functionality.

To simplify installation, we provide a shell script that tries to determine the 
Linux distribution installed on the host, and tries to download and install 
prerequisite packages automatically. To run that script, first enter into the 
Vrui directory:
    $ cd ~/src/vrui-main
where you potentially replace `~/src/vrui-main` with the name of the directory 
where you cloned/unpacked Vrui in the previous step.

Then run the script by entering:
    $ bash ./InstallPrerequisites.sh

The script will ask you to enter your user account's password when necessary, 
and will print a green completion message at the end if at least all required 
system packages were successfully installed. Do not proceed if the script ends 
with a red error message.

## Step 2: Build Vrui

To build Vrui, enter into the same terminal window:  
    $ make

You can speed up the build process if your host has multiple CPUs or CPU cores. 
Instead of the above, enter into the same terminal:  
    $ make -j<num_cpus>
where you replace `<num_cpus>` with the number of CPUs or CPU cores on your 
host, say `-j8` if you have eight cores.

Building Vrui might take a few minutes, and will print lots of output to the 
terminal window. Be patient, and, once it's done, check that there were no 
error messages. The quickest way to check whether Vrui built successfully is to 
run `make` a second time. If everything went well the first time, it will 
print:  
    make: Nothing to be done for 'all'.

This build process will prepare Vrui to be installed inside the `/usr/local` 
directory tree, which is the traditional place for software installed from 
source. Scroll back through the output from `make` and locate the following 
section towards the beginning:
    ---- Vrui installation configuration ----
    Root installation directory               : /usr/local
    Header installation root directory        : /usr/local/include/Vrui-13.1
    Library installation root directory       : /usr/local/lib64/Vrui-13.1
    Executable installation directory         : /usr/local/bin
    Plug-in installation root directory       : /usr/local/lib64/Vrui-13.1
    Configuration file installation directory : /usr/local/etc/Vrui-13.1
    Shared file installation root directory   : /usr/local/share/Vrui-13.1
    Makefile fragment installation directory  : /usr/local/share/Vrui-13.1
    Build system installation directory       : /usr/local/share/Vrui-13.1/make
    pkg-config metafile installation directory: /usr/local/lib64/pkgconfig
    Documentation installation directory      : /usr/local/share/doc/Vrui-13.1
(Your output may look slightly different.)

The most important of those lines is this one:  
    Build system installation directory       : /usr/local/share/Vrui-13.1/make
It shows the location of Vrui's build system, and this is the location you will 
need to use to build add-on packages or Vrui applications later. Take note of 
the precise location, in this example `/usr/local/share/Vrui-13.1/make`.

## Step 3: Install Vrui

After building Vrui successfully, you can install it in the configured location 
by entering the following into the same terminal window:  
    $ sudo make install
which will ask you for your user account's password to install Vrui in a system 
location, and then install it. This should be quick. After the command 
completes, check that there were no errors.

If there were no errors, you are done!

## Optional: Build Vrui's Example Applications

Vrui comes packaged with a few example applications, to demonstrate how to 
create Vrui applications. You can build these now to get a feel for how to 
build other Vrui applications later, or to test whether Vrui is working 
properly.

### Step 1: Enter The ExamplePrograms Directory

Enter into the same terminal window:  
    $ cd ExamplePrograms

### Step 2: Build The Example Applications

Run `make` to build the example programs. During building Vrui itself, the 
build procedure automatically configured the `makefile` in the 
`ExamplePrograms` directory to find the Vrui installation. But to practice for 
building other Vrui applications later, you should still pass the location of 
Vrui's build system, mentioned above, to `make`. Enter into the same terminal 
window:  
    $ make VRUI_MAKEDIR=/usr/local/share/Vrui-13.1/make  
(Adjust the value after the equal sign if the build system installation 
directory you saw when building Vrui was different from the example here.)

If there were no errors, you should now see a new `bin` directory in the 
`ExamplePrograms` directory.

### Step 3: Run An Example Application

To run one of the example applications, enter into the same terminal window:  
    $ ./bin/ShowEarthModel

This should open a new window on your desktop, titled "ShowEarthModel," and 
display a spinning globe. Congratulations, your Vrui installation is complete 
and working! To quit the example program, either press the Esc key, or simply 
close the application's window.
