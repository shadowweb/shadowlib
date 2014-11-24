Trace Utility
=============

Build Instructions
------------------

Add the following statements to `-finstrument-functions -finstrument-functions-exclude-file-list=murmur-hash3`. The reason for the exclusion patter is that `src/collections/murmur-hash3.c` file contains some `always-inline` functions that do not play well with the instrumentation. Since the call tree for murmur hash is straignt forward, it is not a big deal to have it excluded.

The statements need to be appended to the value of `cflags` variable.

If target executable does not have similar issues, `-finstrument-functions-exclude-file-list` is actually not needed, but it is can be used as a mechaninsm to contain the amount of output the program is going to generate and in general to stub out a lot of irrelevant information. A similar argument `-finstrument-functions-exclude-function-list` can be used to inhibit tracing on function level.

Run `~/ninja/ninja/ninja -v` to rebuild everything. It will also build `trace.so` file that will be needed later.

Run Instructions
----------------

From this point on we can run executables in three different ways:

* as is
* with `LD_PRELOAD=build/src/tools/trace/trace.so` but without turning tracing on yet
* with `LD_PRELOAD=build/src/tools/trace/trace.so` and with empty `TRACE` file in the running directory

Only the last option will produce trace output in the files `<program name>.TRACE.<thread id>`. These generated files will be processed in later steps.

Post Process Instructions
-------------------------

To generate function address call tree run run run run:

```
build/src/tools/trace/build-call-tree -input-file <program name>.TRACE --input-threads=<comma separated list of thread ids> --log-stdout
```

In this step for each thread id mentioned in the list the tool will try to find a file with the following name `<program name>.TRACE.<thread id>`, read it, and generate call tree output with function addresses in file `<program name>.TRACE.<thread id>.out`. Additionally, it will also generate file `<program name>.TRACE.addr` which will contain a list of all unique functions addresses it has encountered when analyzing input files.

The next step is to generate function address to symbol mapping. This is done using the original executable file and the list of function addresses generated in the previous step by running the following command:

```
src/tools/trace/get-symbols.pl -e <executable name> -i <program name>.TRACE.addr -o <program name>.TRACE.symbols
```

The last step is to generate call tree for each thread output that we are interested in. This can be done by running the following command:

```
src/tools/trace/print-call-tree.pl -i <program name>.TRACE.<thread id>.out -s <program name>.TRACE.symbols -o <program name>.TRACE.<thread id>.tree
```
The generated tree file can be examined now as it contains a fairily good representation of user space function calls.

Future Work
-----------

The intention is to have a tree file viewer which supports collapsing and expanding call tree levels. Also, for big executables the tree might be pretty big and therefore it might be better to convert all tool outputs to binary format.


