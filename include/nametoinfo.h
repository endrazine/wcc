

typedef struct assoc_nametoinfo_t{
	char *name;
	char *dst;
}assoc_nametoinfo_t;

assoc_nametoinfo_t nametoinfo[] = {
{".dynsym", ".note.ABI-tag"},
{".gnu.version_r", ".note.ABI-tag"},
{".rela.plt", ".plt"},
};

