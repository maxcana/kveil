#pragma once
#include <ntddk.h>
#include <stdint.h>

#define offsetof(type, member) ((size_t)&(((type*)0)->member))

void* memmem(const void* hay, size_t hlen, const void* ned, size_t nlen);
UNICODE_STRING uniencode(const wchar_t* in);
char* where(const wchar_t* routine_name);
NTSTATUS kmemcpy(VOID UNALIGNED* Destination, CONST VOID UNALIGNED* Source, ULONG Length);
void print(PCSTR format, ...);
void bsod();
uint32_t find_a_session1_process_pid();