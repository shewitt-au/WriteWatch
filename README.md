# WriteWatch

This is hacked together, cast ridden, messy, messy, hastily written stuff.
I'll clean it up at some stage. Only works on x64 Windows apps. It's designed
to detect invalid pointers written to memory it allocates. It's used by
'MingW64-checked-pointer-vector' to create a checked pointer vector to track
down a particuar bug. It didn't find the bug, but ruled out external memory
corruption. It uses vectored exception handling (back in the day I would have
done something like this by [detouring](https://github.com/microsoft/Detours) 
`KiUserExceptionDispatcher`) and dynamic code generation. It uses the Detours
library to copy the faulting instruction into the dynamically generated code.
