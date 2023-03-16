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
3. Compile `synthetico`:
   ```
   $ git clone https://github.com/black-sat/synthetico.git
   $ cd synthetico
   $ mkdir build && cd build
   $ cmake ..
   $ make
   
If you installed BLACK in some non-standard location, make sure to give `cmake` the right options.
Now, you can run the `synth` executable from the `build` directory.

## Usage

The tool expects on the command line the following arguments:
1. the choice of which algorithm to run:
   a. `novel`, for our novel symbolic tableau-based algorithm
   b. `classic`, for a classic fixpoint backward reachability algorithm
2. a $\mathsf{F}(\alpha)$ or $\mathsf{G}(\alpha)$ formula
3. the list of which variables in the formula have to be treated as *inputs* (i.e. *uncontrollable* variables)

Please be sure to *quote* the formula on the shell's command line with *single quotes*.

The syntax of the formulas is the one accepted by BLACK (see [Input Syntax](https://www.black-sat.org/en/stable/syntax.html)).

Examples:
```
$ ./synth novel 'F(u & c)' u
UNREALIZABLE

$ ./synth classic 'G(u | c)' u
REALIZABLE
```


