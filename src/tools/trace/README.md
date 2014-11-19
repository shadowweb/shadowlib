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

Only the last option will produce trace output in the file `<program name>.TRACE`. Once this file is generated, it will be processed in later steps.

