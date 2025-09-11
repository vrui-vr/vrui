# Getting started with VRUI

Follow these instructions to install VRUI with standard settings in a standard location, to simplify building and installing add-on packages and VRUI applications.

??? info "Heads up!"
    Angle brackets `<>` in commands below are placeholders, meaning that you have to replace everything between, and including, the angle brackets with some text that depends on your specific circumstances.

    For example, if your host has eight CPUs, instead of entering `-j<number of CPUs>` as part of some command, you would enter `-j8`.

## Step 1: Download the VRUI repository from GitHub

The VRUI code repository can be downloaded either by:

1. downloading the zip file and unpacking it **OR**
2. cloning the repository with `git clone`

!!! warning
    If you are unfamiliar with git and/or GitHub, you should probably go the zip file route.

### Option 1: Downloading and unpacking a zip file from GitHub

On [the VRUI repository's main page](https://github.com/vrui-vr/vrui), click on the green "<> Code" button, and then click on "Download ZIP" in the menu that pops up in response.

<!-- ![Downloading a ZIP from a GitHub repo](download_zip.png) -->

Depending on your browser settings, you may be asked where to store the file being downloaded, or it might be stored in a default location, such as your `Downloads` directory. Take note of what the zip file is called and where it is stored.

Then, once the file is completely downloaded, enter the following multiple lines into a terminal window:

```sh
cd ~
mkdir src
cd src
```

```sh
cd ~/src
```

Then enter into the same terminal window:

```sh
unzip <path to downloaded zip file>
```

Replace `<path to downloaded zip file>` with the full path to the zip file, for example `~/Downloads/vrui-main.zip`.

Finally, check for the name of your new VRUI directory by entering:

```sh
ls
```

which will list all files in the `src` directory, which should include a new directory called `VRUI-main`. Take note of this name, and then enter into that directory by typing this command into the terminal window:

```sh
cd <VRUI directory>
```

where you replace `<VRUI directory>` with the name of the directory where you cloned/unpacked the VRUI in the previous step, as printed by `ls`.

### Option 2: Clone the repository from GitHub

First, create a directory in your terminal where the VRUI code will live:

```sh
cd ~
mkdir src
cd src
```
Then, navigate to this directory:

```sh
cd ~/src
```

Now, we can clone the repository from GitHub:

```sh
git clone https://github.com/vrui-vr/vrui.git
```

Finally, check for the name of your new VRUI directory by entering:

```sh
ls
```

which will list all files in the `src` directory, which should include a new directory called `VRUI`. Take note of this name, and then enter into that directory by typing this command into the terminal window:

```sh
cd <VRUI directory>
```

where you replace `<VRUI directory>` with the name of the directory where you cloned/unpacked the VRUI in the previous step, as printed by `ls`.


## Step 2: Install prerequisite packages

VRUI uses a relatively large set of system-provided packages to implement its functionality. Some of these are essential, some are optional, some are *very* optional. See the README file for the complete list of prerequisite system packages. Ideally, you should install the full set of packages to unlock all of VRUI's functionality.

To simplify installation, we provide a shell script that tries to determine the Linux distribution installed on the host, and tries to download and install prerequisite packages automatically. To run that script, enter into the same terminal window:

```sh
bash ./InstallPrerequisites.sh
```

which will ask you to enter your user account's password when necessary, and will print a green completion message at the end if at least all required system packages were successfully installed. Do not proceed if the script ends with a red error message.

## Step 3: Build VRUI

To build VRUI inside the VRUI base directory, enter into the same terminal window:

```sh
make
```

You can modify the install location of VRUI by passing the VRUI_MAKEDIR=<Vrui build system location> argument to `make`: `make VRUI_MAKEDIR=<Vrui build system location>`, replacing `<Vrui build system location>` with the location you want to install VRUI's build system to.

```sh

???+ tip
    You can **speed up the build process** if your host has multiple CPUs or CPU cores. Instead of the above, enter into the same terminal:

    ```
    make -j<number of cpus>
    ```

    again replacing `<Vrui build system location>` with the location of Vrui's build system on your host, and replacing `<number of cpus>` with the number of CPUs or CPU cores on your host, say `-j8` if you have eight cores. Note that there is no space between the `-j` and the number of cores.

    Using `-j$(nproc)` (exactly as written) will tell your computer to figure out how many cores it has.

Building VRUI might take a few minutes, and will print lots of output to the terminal window. Be patient, and, once it's done, check that there were no error messages. The quickest way to check whether VRUI built successfully is to run `make` a second time:

```sh
make
```

If everything went well the first time, it will print only this line:

```sh
make: Nothing to be done for 'all'.
```

If you run into an error like `/bin/sh: 1: locate: not found`, it means that one of the prerequisite packages was not installed correctly. You will need to correctly install the missing package and then run `make` again.

This build process will prepare VRUI to be installed inside the `/usr/local` directory tree, which is the traditional place for software installed from source. Scroll back through the output from `make` and locate the following section towards the beginning:

```sh
---- VRUI installation configuration ----
Root installation directory               : /usr/local
Header installation root directory        : /usr/local/include/VRUI-13.1
Library installation root directory       : /usr/local/lib64/VRUI-13.1
Executable installation directory         : /usr/local/bin
Plug-in installation root directory       : /usr/local/lib64/VRUI-13.1
Configuration file installation directory : /usr/local/etc/VRUI-13.1
Shared file installation root directory   : /usr/local/share/VRUI-13.1
Makefile fragment installation directory  : /usr/local/share/VRUI-13.1
Build system installation directory       : /usr/local/share/VRUI-13.1/make
pkg-config metafile installation directory: /usr/local/lib64/pkgconfig
Documentation installation directory      : /usr/local/share/doc/VRUI-13.1
```

(Your output may look slightly different.)

The most important of those lines is this one:

```sh
Build system installation directory: /usr/local/share/VRUI-13.1/make
```

showing the location of VRUI's build system, and this is the location you will need to use to build add-on packages or VRUI applications later. Take note of the precise location, in this example `/usr/local/share/VRUI-13.1/make`.

## Step 4: Install VRUI

After building VRUI successfully, you can install it in the configured location by entering the following into the same terminal window:

```sh
sudo make install
```

which will ask you for your user account's password to install VRUI in a system location, and then install it. This should be quick. After the command completes, check that there were no errors.

If there were no errors, you are done! 🎉

## Optional: Build VRUI's example applications

VRUI comes packaged with a few example applications demonstrating how to create VRUI-based VR applications. You can build these now to get a feel for how to build other VRUI applications later, or to test whether VRUI is working properly.

### Step 1: Enter the `ExamplePrograms` directory

Enter into the same terminal window:
```
cd ExamplePrograms
```

### Step 2: Build the example applications

Run `make` to build the example programs. During building VRUI itself, the build procedure automatically configured the `makefile` in the `ExamplePrograms` directory to find the VRUI installation. But to practice for building other VRUI applications later, you should still pass the location of VRUI's build system, mentioned above, to `make`. Enter into the same terminal window to use the default install location:

```sh
make
```

OR for a custom install location, enter:

```sh
make VRUI_MAKEDIR=<VRUI build system location>
```

where you replace `<VRUI build system location>` with the location of VRUI's build system on your host, as described in the previous section.

!!! example
    Your command will look something like this:

    ```
    make VRUI_MAKEDIR=/usr/local/share/VRUI-13.1/make
    ```

???+ tip
    You can **speed up the build process** if your host has multiple CPUs or CPU cores. Instead of the above, enter into the same terminal:

    ```
    make VRUI_MAKEDIR=<Vrui build system location> -j<number of cpus>
    ```

    again replacing `<Vrui build system location>` with the location of Vrui's build system on your host, and replacing `<number of cpus>` with the number of CPUs or CPU cores on your host, say `-j8` if you have eight cores. Note that there is no space between the `-j` and the number of cores.

    Using `-j$(nproc)` (exactly as written) will tell your computer to figure out how many cores it has.

If there were no errors, you should now see a new `bin` directory in the `ExamplePrograms` directory.

### Step 3: Run an example application

To run one of the example applications, enter into the same terminal window:

```sh
./bin/ShowEarthModel
```

This should open a new window on your desktop, titled "ShowEarthModel," and display a spinning globe. Congratulations, your VRUI installation is complete and working! To quit the example program, either press the ++esc++ key, or simply close the application's window.
