# Synthetico
Pure-past LTL realizability checker based on BLACK

This tool solves the realizability problem for temporal formulas of the form
$\mathsf{F}(\alpha)$ or $\mathsf{G}(\alpha)$, where $\alpha$ is a *pure-past LTL* formula.

## Installation 

The tool currently works only on Linux systems.
1. Install [Pedant](https://github.com/fslivovsky/pedant-solver) and
   **make sure** the `pedant` executable is in your `PATH`, *i.e.* the command:
   ```
   $ which pedant
   ```
   should show its location.
2. Install [BLACK](https://www.black-sat.org) following the instructions on the website, **but pay attention**:
   BLACK needs to be compiled **from source** and from the `features/synthetico` branch, 
   so after cloning the repository from GitHub, make sure to run the following from BLACK's
   source directory:
      ```
      $ git checkout features/synthetico
      ```
   We suggest to install BLACK in a local prefix (*i.e.* pass the `-DCMAKE_INSTALL_PREFIX=[install location]` option to CMake with `[install location]` replaced by a local path).
3. Install [**this specific version**](https://github.com/KavrakiLab/cudd) of the CUDD BDD library. 
   1. Make sure to have `automake` installed.
   2. After cloning the repository run the following:
      ```
      $ autoreconf -i
      $ ./configure --enable-silent-rules --enable-obj --enable-dddmp --prefix=[install location]
      ```
      Replace `[install location]` with the same prefix you used in BLACK's installation.
   
   
4. Compile `synthetico`:
   ```
   $ git clone https://github.com/black-sat/synthetico.git
   $ cd synthetico
   $ mkdir build && cd build
   $ cmake -DCMAKE_INSTALL_PREFIX=[install location]..
   $ make
   ```
   Again, replace `[install location]` with the right prefix.
   
Now, you can run the `synth` executable from the `build` directory.

## Usage

The tool expects on the command line the following arguments:
1. the choice of which algorithm to run:

   a. `qbf`, for our qbf symbolic algorithm
   
   b. `bdd`, for the classic fixpoint backward reachability algorithm

2. a $\mathsf{F}(\alpha)$ or $\mathsf{G}(\alpha)$ formula
3. the list of which variables in the formula have to be treated as *inputs* (i.e. *uncontrollable* variables)

Please be sure to *quote* the formula on the shell's command line with *single quotes*.

The syntax of the formulas is the one accepted by BLACK (see [Input Syntax](https://www.black-sat.org/en/stable/syntax.html)).

Examples:
```
$ ./synth qbf 'F(u & c)' u
UNREALIZABLE

$ ./synth classic 'G(u | c)' u
REALIZABLE
```

## Run the benchmarks

The `tests` directory contains a `test.sh` script to run the tool in batch on
many benchmarks and collect the runtimes. It expects a timeout amount in seconds
and an file with the benchmarks to execute, each line being a command line for
the tool itself.

Look at `tests/aaai/random-300-5-20-42.txt` for an example.

Usage:
```
$ ./test.sh 300 ../tests/aaai/random-300-5-20-42.txt
```