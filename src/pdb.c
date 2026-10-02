#include <ntddk.h>
#include <stdint.h>
#include <stdlib.h>
#include <utils.h>

typedef struct
{
    GUID guid;
    uint32_t age;
    char name[128];
} PdbId;

// get the pdb file id from a kernel module.
// base = loaded module base address.
// returns 1 and fills pdb if found.
// you can now use this info to get from server via https://msdl.microsoft.com/download/symbols/<pdbname>/<GUID: 32 uppercase hex digits, no-dashes, no {}s><age: the link age>/<pdbname>
int get_pdbid(const char* base, PdbId* out)
{
    const CV_RSDS* cv;

    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;

    const IMAGE_NT_HEADERS64* nt = (const IMAGE_NT_HEADERS64*)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;

    const IMAGE_DATA_DIRECTORY* dd = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    if (!dd->VirtualAddress || !dd->Size) return 0;

    const IMAGE_DEBUG_DIRECTORY* dbg = (const IMAGE_DEBUG_DIRECTORY*)(base + dd->VirtualAddress);
    size_t count = dd->Size / sizeof(IMAGE_DEBUG_DIRECTORY);

    for (size_t i = 0; i < count; i++)
    {
        if (dbg[i].Type != IMAGE_DEBUG_TYPE_CODEVIEW) continue;
        if (!dbg[i].AddressOfRawData) continue; // not mapped

        cv = (const CV_RSDS*)(base + dbg[i].AddressOfRawData);
        if (cv->Signature != 0x53445352) continue;

        out->guid = cv->Guid;
        out->age = cv->Age;

        max_name = dbg[i].SizeOfData - offsetof(CV_RSDS, PdbName);
        if (max_name > PDB_NAME_MAX) max_name = PDB_NAME_MAX;

        size_t j, max_name;
        for (j = 0; j < max_name && cv->PdbName[j]; j++)
            out->name[j] = cv->PdbName[j];
        if (j == max_name) continue;
        out->name[j] = '\0';

        return 1;
    }
    return 0;
}

void format_guid(GUID g, char* buf, size_t buf_size)
{
    RtlStringCchPrintfA(buf, buf_size, "%08lX%04hX%04hX%02X%02X%02X%02X%02X%02X%02X%02X", g->Data1, g->Data2, g->Data3, g->Data4[0], g->Data4[1], g->Data4[2], g->Data4[3], g->Data4[4],
                        g->Data4[5], g->Data4[6], g->Data4[7]);

    return;
}

// returns 1 if they are equal, else 0
int pdbid_equal(PdbId a, PdbId b)
{
    return a->guid == b->guid && a->age == b->age ? 1 : 0;
}