#include <ntddk.h>
#include <stdlib.h>
#include <string.h>
#include <wdf.h>

void* memmem(const void* hay, size_t hlen, const void* ned, size_t nlen)
{
    if (nlen == 0) return (void*)hay;
    if (hlen < nlen) return NULL;

    const unsigned char* h = hay;
    const unsigned char* n = ned;
    size_t last = hlen - nlen;

    for (size_t pos = 0; pos <= last; pos++)
    {
        const unsigned char* p = memchr(h + pos, n[0], last - pos + 1);
        if (!p) return NULL;
        pos = (size_t)(p - h);
        if (memcmp(p, n, nlen) == 0) return (void*)p;
    }
    return NULL;
}

char* where(const wchar_t[] routine_name)
{
    UNICODE_STRING uni;
    RtlInitUnicodeString(&uni, routine_name);
    return MmGetSystemRoutineAddress(uni);
}

// void* find()

void print(PCSTR format)
{
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, format);
}

void yikes(PCSTR format)
{
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[ERROR] yikes: ");
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, format);
}

void bsod(PCSTR format)
{
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[FATAL] bsod: ");
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, format);

    WdfVerifierKeBugCheck(0xE2, (ULONG_PTR)format, 0, 0, 0);
}