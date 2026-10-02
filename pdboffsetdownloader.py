# pdb offset downloader:

# 1. fetches pdb from https://msdl.microsoft.com/download/symbols/<pdbname>/<GUID><age>/<pdbname>
# 2. parses it
# 3. selects specific symbols (hardcoded in python) (depending on pdb name)
# 4. finds their addresses
# 5. formats it into C object notation
# 6. prints it


# input format:

# `pdboffsetdownloader (pdb name) (guid) (linker age)`
# `pdboffsetdownloader ntkrnlmp.pdb DBB1F0A27EDF48859ED6FAD12495B5ED 2`
# `pdboffsetdownloader win32kfull.pdb 3FA4661AB3974DF2A500FC9EB2E6BFDB 1`

# output format:

#     {
#         .build_number = {.guid = {0x01234567, 0x89ab, 0xcdef, {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef}}, .age = 1, .name = {0}},
#         .ExpQuerySystemInformation = 0x0000000140A06E08 - 0x140000000,
#         .KeQueryPrimaryGroupThread = 0x0000000140490F54 - 0x140000000,
#     },

#     {
#         .build_number = {.guid = {0x01234567, 0x89ab, 0xcdef, {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef}}, .age = 1, .name = {0}},
#         .GreProtectSpriteContent = 0x0000000140256110 - 0x140000000,
#     },
