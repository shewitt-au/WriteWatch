# WriteWatch

This is hacked together, cast ridden, messy, messy, hastily written stuff. I'll clean it up at some stage. Only works on x64 Windows apps. It's designed to detect invalid pointers written to the memory it allocates. It uses vectored exception handling (back in the day I would have done something like this by [detouring](https://github.com/microsoft/Detours) `KiUserExceptionDispatcher`) and dynamic code generation.

The memory is allocated with read-only access. On write the vectored exception handler enables writing, re-executes the faulting code, restore the original read-only permissions and then checks the pointer is valid. If the written pointer is not readable a breakpoint is generated.  

It's used by 'MingW64-checked-pointer-vector' to create a checked pointer vector I used to track down a particular bug. It didn't find the bug, but ruled out external memory corruption.  It uses the Detours library to copy the faulting instruction into the dynamically generated code.
