#pragma once
#include <stdint.h>
#include <undoc.h>

//* # EXPORTS

//* ## Values
// const uint64_t hidden_images_count; // c has no array.length kek
// const UNICODE_STRING hidden_images[];
int should_hide(UNICODE_STRING target);

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


int init_globals();
