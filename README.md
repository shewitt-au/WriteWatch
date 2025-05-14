# WriteWatch

This is hacked together, cast ridden, messy, messy, hastily written stuff.
I'll clean it up at some stage. Only works on x64 Windows. It's design to
detetect invalid pointers written to memory it allocates. It's used by
'MingW64-checked-pointer-vector' to create a checked pointer vector used
to track down a particuar bug. It didn't find the bug, but ruled out external
memory corruption. It uses vectored exception handling and dynamic code
generation.
