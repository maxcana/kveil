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

# install: `pip install setuptools && pip install "construct==2.10.70" && pip install pdbparse --no-build-isolation --no-deps`

import sys, os, requests, tempfile, pdbparse

SYMBOLS = {
    "ntkrnlmp.pdb": ["ExpQuerySystemInformation", "KeQueryPrimaryGroupThread"],
    "win32kfull.pdb": ["GreProtectSpriteContent"],
}
IMAGE_BASE = 0x140000000


def fmt_guid(g):
    d4 = [g[16 + i * 2 : 18 + i * 2] for i in range(8)]
    return f"{{0x{g[:8].lower()}, 0x{g[8:12].lower()}, 0x{g[12:16].lower()}, " f"{{{', '.join('0x' + b.lower() for b in d4)}}}}}"


def get_rva(pdb, sym_name):
    try:
        sects = pdb.STREAM_SECT_HDR_ORIG.sections
    except AttributeError:
        sects = pdb.STREAM_SECT_HDR.sections
    for sym in pdb.STREAM_GSYM.globals:
        if getattr(sym, "name", "") == sym_name and hasattr(sym, "segment") and 1 <= sym.segment <= len(sects):
            return sym.offset + sects[sym.segment - 1].VirtualAddress
    return None


def main():
    if len(sys.argv) != 4:
        sys.exit(f"Usage: {sys.argv[0]} <pdb_name> <guid> <age>")
    pdb_name, guid, age = sys.argv[1], sys.argv[2].upper(), int(sys.argv[3])
    r = requests.get(f"https://msdl.microsoft.com/download/symbols/{pdb_name}/{guid}{age}/{pdb_name}")
    r.raise_for_status()
    with tempfile.NamedTemporaryFile(delete=False, suffix=".pdb") as f:
        f.write(r.content)
        tmp = f.name
    try:
        pdb = pdbparse.parse(tmp)
        print("    {")
        print(f"        .build_number = {{.guid = {fmt_guid(guid)}, .age = {age}, .name = {{0}}}},")
        for sym in SYMBOLS.get(pdb_name, []):
            rva = get_rva(pdb, sym)
            if rva is not None:
                print(f"        .{sym} = 0x{rva:X},")
        print("    },")
    finally:
        os.unlink(tmp)


if __name__ == "__main__":
    main()
