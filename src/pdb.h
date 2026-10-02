#include <stdint.h>

typedef struct
{
    GUID guid;
    uint32_t age;
    char name[128];
} PdbId;

int get_pdbid(const char* base, PdbId* out);
void format_guid(const GUID* g, char* buf, size_t buf_size);
int pdbid_equal(PdbId* a, PdbId* b);