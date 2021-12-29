
static int is_prefix_of(const char *a, const char *b) {
	return !strncmp(a, b, strlen(a));
}

static int str_eq(const char *a, const char *b) {
	return !strcmp(a, b);
}
