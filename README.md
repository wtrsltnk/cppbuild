# CppBuild

An alternative build system where you write your build script in c++. Something like this could be in the c++ standard for simple programs.

You can use this file on Windows with the MinGW compiler. Put the files ``cppbuild.cmd`` and ``cppbuild.hpp`` in any folder from your PATH. Then create your build file with the name ``__build__.cpp`` and add the following code to this file:

```c++

#include <cppbuild.hpp>

int main(int argc, char* argv[])
{
    cppbuild::init(argc, argv);

    cppbuild::Target target("program");

    target.files({
                "program.cpp",
                "library.cpp",
            });

    target.includeDirs({
        "include",
    });

    return 0;
}

```

This will create one target with the name ``program`` that will compile and link the files ``program.cpp`` and ``library.cpp`` into ``program.exe``. It will add the ``include`` directory as -I parameter to the compiler.

The following methods can be used on the cppbuild::Target class:

* ``files`` - Adds files to compile
* ``includeDirs`` - Adds directories with the ``-I`` parameter
* ``libraries`` - Adds libraries with the ``-l`` parameter
* ``libraryDirs`` - Adds directories with the ``-L`` parameter
* ``compilerFlags`` - Adds compiler flags
* ``linkerFlags`` - Adds linker flags
* ``installDir`` - Adds directory to install build artifacts into
* ``installFiles`` - Adds files to install

## Cppbuild command and its parameters

To build your program open a commandline window at the folder containing the ``__build__.cpp`` and type ``cppbuild``.

The following parameters can be passed to the ``cppbuild`` command:

* ``clean`` - Cleans all build files
* ``build`` - Builds all project targets
* ``run`` - Runs project target
* ``install`` - Installs the project
