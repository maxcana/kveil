# kveil
The stupid method for kernel hooking™

## what is this
kveil is a simple KMDF driver for windows 11 that hides arbitrary running processes by name through hooking `ntoskrnl.exe!NtQuerySystemInformation`.
for thoroughess, it hooks all of these handlers:
- 0x05: `SystemProcessInformation`
- 0x35: `SystemSessionProcessInformation`
- 0x39: `SystemExtendedProcessInformation`
- 0x94: `SystemFullProcessInformation`
- 0xFC: `SystemBasicProcessInformation`
- 0x67: `SystemCodeIntegrityInformation` (to hide itself)

note: you could still find a kveil-hidden process without its pid through `NtGetNextProcess`, but this is slow and rarely used.

oh right, it also hooks `win32kfull.sys!GreProtectSpriteContent` to just return 1, making `SetWindowDisplayAffinity` do literally nothing.
(because `GreProtectSpriteContent` purely handles graphics, `GetWindowDisplayAffinity` still reports whatever it was set to through `SetWindowDisplayAffinity`, even though `dwAffinity` always behaves as if set to `0`.)

## building
1. download and install [WDK](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk)
2. install Visual Studio on a Windows machine
3. reconfigure the `AdditionalIncludeDirectories` in `kveil.vcxproj` to match your SDK and WDK location
4. "Build" to x64 release

## usage
1. disable PatchGuard
2. install [dbgview64.exe](https://live.sysinternals.com/), run as admin and "Capture Kernel" to see logs (recommended, you might be missing offsets)
3. `sc create kveil binPath="C:\...\kveil.sys" type=kernel && sc start kveil`


## pdboffsetdownloader.py usage
if you are missing offsets, use `python pdboffsetdownloader.py <pdb_name> <guid> <age>`, hardcode them, and rebuild. it will tell you all these values in the error.

you might need to fix some dependency issues: `pip install setuptools && pip install "construct==2.10.70" && pip install pdbparse --no-build-isolation --no-deps && pip install requests`

example:
```log
hello from kernel
init_globals()
found ntoskrnl @ 0xFFFFF80591200000
found win32kfull.sys @ 0xFFFFF80527010000
your ntoskrnl build id: ntkrnlmp.pdb C29EBFB06B78B3C020DCA66D99713F9E 1
your win32kfull build id: win32kfull.pdb 5CD57181BCFDC5AE8F4819BEB1196F09 1
ERROR missing ntoskrnl offsets for your windows build. fetch them from the pdb automatically with the python script using the above build id.
ERROR missing win32kfull offsets for your windows build. fetch them from the pdb automatically with the python script using the above build id.
init_globals failed
```

run: `python pdboffsetdownloader.py ntkrnlmp.pdb C29EBFB06B78B3C020DCA66D99713F9E 1`
to get:
```c
    {
        .build_number = {.guid = {0xc29ebfb0, 0x6b78, 0xb3c0, {0x20, 0xdc, 0xa6, 0x6d, 0x99, 0x71, 0x3f, 0x9e}}, .age = 1, .name = {0}},
        .ExpQuerySystemInformation = 0xA06DF8,
        .KeQueryPrimaryGroupThread = 0x490F54,
    },
```