#include "stdafx.h"
#include <stdio.h>
#include "wubi.h"

static unsigned char bs_buffer[12288];

static wchar_t err_buf[128];
static wchar_t err_msg_buf[256];
static HANDLE hFileMapMB;
static HANDLE hFileMB;



static void* mb_header_view;
static void* mb_content_view;

struct s_mb_hdr {
	char magic[4];
	unsigned int n_of_entries;
	unsigned int size_of_appendix;
	unsigned int lookup_map[27][27][2];
} *mbHeader;
struct s_mb_entry* mbEntries;
wchar_t* appendices;

static void wPerrorW(LPCWSTR errmsg) {
	DWORD error = GetLastError();
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, err_buf, 128, NULL);
	wsprintfW(err_msg_buf, L"%s: %s", errmsg, err_buf);
}

int mb_init(LPCWSTR mb_bin_path, PutsWCallback callback) {
	hFileMB = CreateFileW(mb_bin_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFileMB == INVALID_HANDLE_VALUE) {
		wPerrorW(L"无法打开文件");
		callback(err_msg_buf);
		return 1;
	}
	hFileMapMB = CreateFileMappingW(hFileMB, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hFileMapMB == NULL) {
		wPerrorW(L"无法创建文件映像");
		callback(err_msg_buf);
		CloseHandle(hFileMB);
		hFileMB = NULL;
		return 2;
	}

	mb_header_view = MapViewOfFile(hFileMapMB, FILE_MAP_READ, 0, 0, sizeof(struct s_mb_hdr));
	if (mb_header_view == NULL) {
		wPerrorW(L"无法映射文件头");
		callback(err_msg_buf);
		CloseHandle(hFileMapMB);
		CloseHandle(hFileMB);
		hFileMapMB = hFileMB = NULL;
		return 3;
	}

	mbHeader = (struct s_mb_hdr*)mb_header_view;
	if (memcmp(mbHeader->magic, "WBMB", 4)) {
		callback(L"MAGIC错误！");
		mb_finalize();
		return 4;
	}
	
	if (sizeof(mbHeader->lookup_map) != 27 * 27 * 8 || ((char *) & mbHeader->lookup_map - (char *)mbHeader) != 12) {
		callback(L"结构体格式错误，请联系开发者！");
		mb_finalize();
		return 5;
	}

	mb_content_view = MapViewOfFile(hFileMapMB, FILE_MAP_READ, 0, 0, sizeof(struct s_mb_hdr) + 16 * mbHeader->n_of_entries + mbHeader->size_of_appendix * 2);
	if (mb_content_view == NULL) {
		wPerrorW(L"无法映射文件内容");
		mb_finalize();
		callback(err_msg_buf);
		return 3;
	}

	mbEntries = (struct s_mb_entry *)((char *)mb_content_view+sizeof(struct s_mb_hdr));
	appendices = (wchar_t *)((char*)mb_content_view + sizeof(struct s_mb_hdr) + 16 * mbHeader->n_of_entries);

	return 0;
}

void mb_finalize() {
	if (mb_header_view) {
		UnmapViewOfFile(mb_header_view);
		mb_header_view = NULL;
	}
	if (mb_content_view) {
		UnmapViewOfFile(mb_content_view);
		mb_content_view = NULL;
	}
	mbHeader =  NULL;
	mbEntries = NULL;
	appendices = NULL;
	CloseHandle(hFileMapMB);
	CloseHandle(hFileMB);
	hFileMB = hFileMapMB = NULL;
}

struct s_mb_entry* mb_findfirst(const char* four_byte_code) {
	int pf1 = four_byte_code[0], pf2 = four_byte_code[1];
	if (pf1 < 'a' || pf1 > 'z') pf1 = 26; else pf1 -= 'a';
	if (pf2 < 'a' || pf2 > 'z') pf2 = 26; else pf2 -= 'a';
	int offset = mbHeader->lookup_map[pf1][pf2][0];
	int len = mbHeader->lookup_map[pf1][pf2][1];
	if (len <= 0) return NULL;
	int a = offset, b = offset + len;
	int good_index = -1;
	while (a < b) {
		int c = (a + b) / 2;
		int cmp = memcmp(mbEntries[c].code, four_byte_code, 4);
		if (cmp < 0) {
			// printf("< Traversing: %c%c%c%c\n", mbEntries[c].code[0], mbEntries[c].code[1], mbEntries[c].code[2], mbEntries[c].code[3]);
			a = c + 1;
		}
		else if (cmp > 0) {
			// printf("> Traversing: %c%c%c%c\n", mbEntries[c].code[0], mbEntries[c].code[1], mbEntries[c].code[2], mbEntries[c].code[3]);
			b = c;
		}
		else {
			// printf("= HIT!!\n");
			good_index = c;
			break;
		}
	}
	if (good_index < 0) return NULL;
	// Find first
	while (good_index >= 0 && !memcmp(mbEntries[good_index].code, four_byte_code, 4)) {
		good_index--;
	}
	good_index++;
	return &mbEntries[good_index];
}

struct s_mb_entry* mb_getbound() {
	return &mbEntries[mbHeader->n_of_entries];
}

LPCWSTR mb_getconv(struct s_mb_entry* ent) {
	wchar_t first = ((wchar_t*)ent->conversion)[0];
	if (first == 0) {
		int xOffset;
		char *ptr = (char *)&xOffset;
		ptr[0] = ent->conversion[2];
		ptr[1] = ent->conversion[3];
		ptr[2] = ent->conversion[4];
		ptr[3] = ent->conversion[5];
		return &appendices[xOffset];
	}
	else {
		return (LPCWSTR)ent->conversion;
	}
}