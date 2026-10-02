// var.c: contains global values or offsets; initialized once on load
#include <ntddk.h>
#include <string.h>
#include <undoc.h>
#include <utils.h>
#include <stdint.h>

//* # EXPORTS

//* ## Values
#define CONST_USTR(s) {sizeof(s) - sizeof(WCHAR), sizeof(s), (PWSTR)(s)}

static const uint64_t hidden_images_count = 3; // c has no array.length kek
static const UNICODE_STRING hidden_images[] = {
    CONST_USTR(L"python.exe"),
    CONST_USTR(L"python3.exe"),
    CONST_USTR(L"pythonw.exe"),
};

int should_hide(UNICODE_STRING target)
{
    for (int i = 0; i < hidden_images_count; i++)
    {
        if (RtlEqualUnicodeString(&target, &hidden_images[i], TRUE))
        {
            // we need to hide this
            return 1;
        }
    }
    return 0;
}

//* ## Offsets
// modules
char* loc_ntoskrnl;
char* loc_win32kfull;
// whereable
NtQuerySystemInformation_t* loc_NtQuerySystemInformation;
char* loc_NtQuerySystemInformationEx;
// hardcoded
ExpQuerySystemInformation_t* loc_ExpQuerySystemInformation;
KeQueryPrimaryGroupThread_t* loc_KeQueryPrimaryGroupThread;
char* loc_GreProtectSpriteContent;

//* # INTERNAL LOGIC

//* module bases
static PVOID where_ntoskrnl()
{
    PVOID base = NULL;
    RtlPcToFileHeader((PVOID)ExAllocatePoolWithTag, &base);
    return base;
}
static PVOID where_kernelmodule(PCSTR ModuleFileName)
{
    ULONG bufSize = 0;
    NTSTATUS status;
    PVOID result = NULL;

    // first call gets the required buffer size
    status = loc_NtQuerySystemInformation(0xB, NULL, 0, &bufSize); // 0xB = SystemModuleInformation
    if (status != STATUS_INFO_LENGTH_MISMATCH || bufSize == 0) return NULL;

    // pad for TOCTOU; modules can load between the two calls
    bufSize += sizeof(RTL_PROCESS_MODULE_INFORMATION) * 16;

    PRTL_PROCESS_MODULES mods = (PRTL_PROCESS_MODULES)ExAllocatePool2(POOL_FLAG_NON_PAGED, bufSize, 'BOOM');
    if (!mods) return NULL;

    status = loc_NtQuerySystemInformation(0xB, mods, bufSize, &bufSize);
    if (NT_SUCCESS(status))
    {

        for (ULONG i = 0; i < mods->NumberOfModules; i++)
        {
            PRTL_PROCESS_MODULE_INFORMATION mod = &mods->Modules[i];

            // OffsetToFileName skips "\SystemRoot\System32\" prefix
            // so mod->FullPathName + mod->OffsetToFileName == "win32kfull.sys"
            PCSTR leaf = (PCSTR)mod->FullPathName + mod->OffsetToFileName;

            if (_stricmp(leaf, ModuleFileName) == 0)
            {
                result = mod->ImageBase; // Runtime VA like 0xFFFFF87A00000000
                break;
            }
        }
    }

    // cleanup
    ExFreePoolWithTag(mods, 'BOOM');
    return result;
}

//* hardcoding
typedef struct
{
    uint64_t win11_build_number;
    uint64_t ExpQuerySystemInformation; // ntoskrnl!
    uint64_t KeQueryPrimaryGroupThread; // ntoskrnl!
    uint64_t GreProtectSpriteContent;   // win32kfull!
} KveilOffsets;

// dump System32 then find these in IDA / or use the PDB for that windows version
static const KveilOffsets hardcoded_offsets[] = {
    {
        .win11_build_number = 26200, // Win11 25H2
        .ExpQuerySystemInformation = 0x0000000140A06E08 - 0x140000000,
        .KeQueryPrimaryGroupThread = 0x0000000140490F54 - 0x140000000,
        .GreProtectSpriteContent = 0x0000000140256110 - 0x140000000,
    },
};

//* init
int init_globals()
{
    // populate whereable (need to do first)
    loc_NtQuerySystemInformation = (NtQuerySystemInformation_t*)where(L"NtQuerySystemInformation");
    if (loc_NtQuerySystemInformation == NULL) return 1;

    loc_NtQuerySystemInformationEx = where(L"NtQuerySystemInformationEx");
    if (loc_NtQuerySystemInformationEx == NULL) return 1;

    // populate modules
    loc_ntoskrnl = (char*)where_ntoskrnl();
    loc_win32kfull = (char*)where_kernelmodule("win32kfull.sys");
    if (loc_ntoskrnl == NULL || loc_win32kfull == NULL)
    {
        print("failed to get bases of kernel modules. win32kfull only loads after you logged in. did you run the driver on boot (bad?)\n");
        return 1;
    }

    // populate hardcoded
    OSVERSIONINFOW osvi;
    RtlGetVersion(&osvi);
    uint64_t build = osvi.dwBuildNumber;

    for (int i = 0; i < ARRAYSIZE(hardcoded_offsets); i++)
    {
        KveilOffsets ko = hardcoded_offsets[i];
        if (build == ko.win11_build_number)
        {
            loc_ExpQuerySystemInformation = (ExpQuerySystemInformation_t*)((uint64_t)loc_ntoskrnl + (uint64_t)ko.ExpQuerySystemInformation);
            loc_KeQueryPrimaryGroupThread = (KeQueryPrimaryGroupThread_t*)((uint64_t)loc_ntoskrnl + (uint64_t)ko.KeQueryPrimaryGroupThread);
            loc_GreProtectSpriteContent = (char*)((uint64_t)loc_win32kfull + (uint64_t)ko.GreProtectSpriteContent);

            print("loaded hardcoded offsets {0x%X, 0x%X} for win11 build %d\n", loc_ExpQuerySystemInformation, loc_GreProtectSpriteContent, build);
            return 0;
        }
    }
    print("ERROR your win11 version (%d) is missing hardcoded offsets, go mine the offsets\n", build);
    return 1;
}