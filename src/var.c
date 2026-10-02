// var.c: contains global values or offsets; initialized once on load
#include <ntddk.h>
#include <pdb.h>
#include <stdint.h>
#include <string.h>
#include <undoc.h>
#include <utils.h>

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
    print("found ntoskrnl @ 0x%I64X\n", (uint64_t)base);
    return base;
}
static PVOID where_kernelmodule(PCSTR ModuleFileName)
{
    uint32_t bufSize = 0;
    NTSTATUS status;

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
        for (uint32_t i = 0; i < mods->NumberOfModules; i++)
        {
            PRTL_PROCESS_MODULE_INFORMATION mod = &mods->Modules[i];

            // OffsetToFileName skips "\SystemRoot\System32\" prefix
            // so mod->FullPathName + mod->OffsetToFileName == "win32kfull.sys"
            PCSTR leaf = (PCSTR)mod->FullPathName + mod->OffsetToFileName;

            if (_stricmp(leaf, ModuleFileName) == 0)
            {
                // Runtime VA like 0xFFFFF87A00000000
                print("found %s @ 0x%I64X\n", ModuleFileName, (uint64_t)mod->ImageBase);
                ExFreePoolWithTag(mods, 'BOOM');
                return mod->ImageBase;
            }
        }
    }

    // cleanup
    ExFreePoolWithTag(mods, 'BOOM');
    return NULL;
}

//* hardcoding

typedef struct
{
    PdbId build_number;
    uint64_t ExpQuerySystemInformation;
    uint64_t KeQueryPrimaryGroupThread;
} NtoskrnlOffsets;

typedef struct
{
    PdbId build_number;
    uint64_t GreProtectSpriteContent;
} Win32kfullOffsets;

static const NtoskrnlOffsets ntoskrnl_offsets[] = {
    {
        .build_number = {.guid = {0x01234567, 0x89ab, 0xcdef, {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef}}, .age = 1, .name = {0}},
        .ExpQuerySystemInformation = 0x0000000140A06E08 - 0x140000000,
        .KeQueryPrimaryGroupThread = 0x0000000140490F54 - 0x140000000,
    },
};

static const Win32kfullOffsets win32kfull_offsets[] = {
    {
        .build_number = {.guid = {0x01234567, 0x89ab, 0xcdef, {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef}}, .age = 1, .name = {0}},
        .GreProtectSpriteContent = 0x0000000140256110 - 0x140000000,
    },
};

//* init
int init_globals()
{
    // populate whereable (need to do first)
    loc_NtQuerySystemInformation = (NtQuerySystemInformation_t*)where(L"NtQuerySystemInformation");
    if (loc_NtQuerySystemInformation == NULL) return 0;

    loc_NtQuerySystemInformationEx = where(L"NtQuerySystemInformationEx");
    if (loc_NtQuerySystemInformationEx == NULL) return 0;

    // populate modules
    loc_ntoskrnl = (char*)where_ntoskrnl();
    loc_win32kfull = (char*)where_kernelmodule("win32kfull.sys");
    if (loc_ntoskrnl == NULL || loc_win32kfull == NULL)
    {
        print("failed to get bases of kernel modules. win32kfull only loads after you logged in. did you run the driver on boot (bad?)\n");
        return 0;
    }

    // populate hardcoded

    // get build numbers
    PdbId ntoskrnl_build;
    get_pdbid(loc_ntoskrnl, &ntoskrnl_build);
    char ntoskrnl_guid_buf[80];
    format_guid(&ntoskrnl_build.guid, ntoskrnl_guid_buf, 80);
    print("your ntoskrnl build id: %s %s %u\n", ntoskrnl_build.name, ntoskrnl_guid_buf, ntoskrnl_build.age);

    PdbId win32kfull_build;
    get_pdbid(loc_win32kfull, &win32kfull_build);
    char win32kfull_guid_buf[80];
    format_guid(&win32kfull_build.guid, win32kfull_guid_buf, 80);
    print("your win32kfull build id: %s %s %u\n", win32kfull_build.name, win32kfull_guid_buf, win32kfull_build.age);

    // match your build to hardcoded offsets, resolve addresses
    int failed = 0;
    for (int i = 0; i < ARRAYSIZE(ntoskrnl_offsets); i++)
    {
        NtoskrnlOffsets o = ntoskrnl_offsets[i];
        if (pdbid_equal(&ntoskrnl_build, &o.build_number))
        {
            print("found hardcoded ntoskrnl offsets for your build!\n");

            loc_ExpQuerySystemInformation = (ExpQuerySystemInformation_t*)((uint64_t)loc_ntoskrnl + (uint64_t)o.ExpQuerySystemInformation);
            loc_KeQueryPrimaryGroupThread = (KeQueryPrimaryGroupThread_t*)((uint64_t)loc_ntoskrnl + (uint64_t)o.KeQueryPrimaryGroupThread);

            print("ntoskrnl!ExpQuerySystemInformation @ 0x%I64X\n", loc_ExpQuerySystemInformation);
            print("ntoskrnl!KeQueryPrimaryGroupThread @ 0x%I64X\n", loc_KeQueryPrimaryGroupThread);

            print("finished loading ntoskrnl offsets.\n");
            break;
        }
        if (i == ARRAYSIZE(ntoskrnl_offsets) - 1)
        {
            print("ERROR missing hardcoded offsets for your windows build. fetch them from the pdb automatically with the python script using the above build id.\n");
            failed = 1;
        }
    }

    for (int i = 0; i < ARRAYSIZE(win32kfull_offsets); i++)
    {
        Win32kfullOffsets o = win32kfull_offsets[i];
        if (pdbid_equal(&win32kfull_build, &o.build_number))
        {
            print("found hardcoded win32kfull_build offsets for your build!\n");

            loc_GreProtectSpriteContent = (char*)((uint64_t)loc_win32kfull + (uint64_t)o.GreProtectSpriteContent);

            print("win32kfull!GreProtectSpriteContent @ 0x%I64X\n", loc_GreProtectSpriteContent);

            print("finished loading win32kfull offsets.\n");
            break;
        }
        if (i == ARRAYSIZE(win32kfull_offsets) - 1)
        {
            print("ERROR missing hardcoded offsets for your windows build. fetch them from the pdb automatically with the python script using the above build id.\n");
            failed = 1;
        }
    }

    if (failed == 1) return 0;

    print("all offsets loaded successfully.\n");
    return 1;
}