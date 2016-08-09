

typedef struct assoc_nametolink_t{
	char *name;
	char *dst;
}assoc_nametolink_t;

assoc_nametolink_t nametolink[] = {
{".dynsym", ".dynstr"},
{".gnu.version_r", ".dynstr"},
{".gnu.version", ".dynsym"},
{".rela.dyn", ".dynsym"},
{".rela.plt", ".dynsym"},
{".dynamic", ".dynstr"}
};

