#pragma once
#include <Windows.h>

typedef void (*PutsWCallback)(LPCWSTR str);

struct s_mb_entry {
	char code[4];
	char conversion[11];
	char priority;
};

int mb_init(LPCWSTR mb_bin_path, PutsWCallback callback);
struct s_mb_entry* mb_findfirst(const char* four_byte_code);
LPCWSTR mb_getconv(struct s_mb_entry* ent);
void mb_finalize();
struct s_mb_entry* mb_getbound();