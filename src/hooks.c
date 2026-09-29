#include <utils.h>
#include <var.h>

void hook_all()
{
    // TODO
    memmem(loc_NtQuerySystemInformation, 1000);
    return;
}