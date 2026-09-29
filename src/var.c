#include <ntddk.h>
#include <string.h>
#include <undoc.h>
#include <utils.h>

ExpQuerySystemInformation_t* loc_ExpQuerySystemInformation;
char* loc_NtQuerySystemInformation;
char* loc_NtQuerySystemInformationEx;

int init_globals()
{
    ExpQuerySystemInformation = try_find_ExpQuerySystemInformation();
    if (ExpQuerySystemInformation == NULL) return 0;

    loc_NtQuerySystemInformation = where(L"NtQuerySystemInformation");
    if (loc_NtQuerySystemInformation == NULL) return 0;

    loc_NtQuerySystemInformationEx = where(L"NtQuerySystemInformationEx");
    if (loc_NtQuerySystemInformationEx == NULL) return 0;
}