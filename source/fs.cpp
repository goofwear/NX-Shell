#include <cstdio>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "fs.h"

// Global vars
FsFileSystem *fs;
FsFileSystem devices[4];
static int PREVIOUS_BROWSE_STATE = 0;

typedef struct {
	char copy_path[FS_MAX_PATH];
	char copy_filename[FS_MAX_PATH];
	bool is_dir = false;
} FS_Copy_Struct;

FS_Copy_Struct fs_copy_struct;

bool FS_FileExists(const char path[FS_MAX_PATH]) {
	FsFile file;
	if (R_SUCCEEDED(fsFsOpenFile(fs, path, FsOpenMode_Read, &file))) {
		fsFileClose(&file);
		return true;
	}

	return false;
}

bool FS_DirExists(const char path[FS_MAX_PATH]) {
	FsDir dir;
	if (R_SUCCEEDED(fsFsOpenDirectory(fs, path, FsDirOpenMode_ReadDirs, &dir))) {
		fsDirClose(&dir);
		return true;
	}

	return false;
}

static int sort(const void *p1, const void *p2) {
	FsDirectoryEntry *entryA = (FsDirectoryEntry *)p1;
	FsDirectoryEntry *entryB = (FsDirectoryEntry *)p2;
	u64 sizeA = 0, sizeB = 0;
	
	if ((entryA->type == FsDirEntryType_Dir) && !(entryB->type == FsDirEntryType_Dir))
		return -1;
	else if (!(entryA->type == FsDirEntryType_Dir) && (entryB->type == FsDirEntryType_Dir))
		return 1;
	else {
		switch(config.sort) {
			case 0: // Sort alphabetically (ascending - A to Z)
				return strcasecmp(entryA->name, entryB->name);
				break;
			
			case 1: // Sort alphabetically (descending - Z to A)
				return strcasecmp(entryB->name, entryA->name);
				break;
			
			case 2: // Sort by file size (largest first)
				sizeA = entryA->file_size;
				sizeB = entryB->file_size;
				return sizeA > sizeB? -1 : sizeA < sizeB? 1 : 0;
				break;
			
			case 3: // Sort by file size (smallest first)
				sizeA = entryA->file_size;
				sizeB = entryB->file_size;
				return sizeB > sizeA? -1 : sizeB < sizeA? 1 : 0;
				break;
		}
	}
	
	return 0;
}

s64 FS_GetDirList(char path[FS_MAX_PATH], FsDirectoryEntry **entriesp) {
	FsDir dir;
	Result ret = 0;

	if (R_FAILED(ret = fsFsOpenDirectory(fs, path, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles, &dir)))
		return ret;
	
	s64 entry_count = 0;
	if (R_FAILED(ret = fsDirGetEntryCount(&dir, &entry_count)))
		return ret;
	
	FsDirectoryEntry *entries = (FsDirectoryEntry *)malloc(entry_count * sizeof(*entries));
	if (R_FAILED(ret = fsDirRead(&dir, NULL, (size_t)entry_count, entries))) {
		free(entries);
		return ret;
	}
	
	qsort(entries, entry_count, sizeof(FsDirectoryEntry), sort);
	fsDirClose(&dir);
	*entriesp = entries;
	return entry_count;
}

void FS_FreeDirEntries(FsDirectoryEntry **entries, s64 entry_count) {
	if (entry_count > 0)
		free(*entries);
	
	*entries = NULL;
}

s64 FS_RefreshEntries(FsDirectoryEntry **entries, s64 entry_count) {
	FS_FreeDirEntries(entries, entry_count);
	return FS_GetDirList(config.cwd, entries);
}

static s64 FS_ChangeDir(char path[FS_MAX_PATH], FsDirectoryEntry **entries, s64 entry_count) {
	FsDirectoryEntry *new_entries;

	s64 num_entries = FS_GetDirList(path, &new_entries);
	if (num_entries < 0)
		return -1;

	// Apply cd after successfully listing new directory
	FS_FreeDirEntries(entries, entry_count);
	strncpy(config.cwd, path, FS_MAX_PATH);
	Config_Save(config);
	*entries = new_entries;
	return num_entries;
}

static int path_cd_up(char path[FS_MAX_PATH]) {
	if (strlen(config.cwd) <= 1 && config.cwd[0] == '/')
		return -1;

	// Remove upmost directory
	bool copy = false;
	int len = 0;
	for (ssize_t i = (ssize_t)strlen(config.cwd); i >= 0; i--) {
		if (config.cwd[i] == '/')
			copy = true;
		if (copy) {
			path[i] = config.cwd[i];
			len++;
		}
	}
	// remove trailing slash
	if (len > 1 && path[len - 1] == '/')
		len--;
	
	path[len] = '\0';
	return 0;
}

s64 FS_ChangeDirNext(const char path[FS_MAX_PATH], FsDirectoryEntry **entries, s64 entry_count) {
	char new_cwd[FS_MAX_PATH];
	const char *sep = !strncmp(config.cwd, "/", 2) ? "" : "/"; // Don't append / if at /
	snprintf(new_cwd, FS_MAX_PATH, "%s%s%s", config.cwd, sep, path);
	return FS_ChangeDir(new_cwd, entries, entry_count);
}

s64 FS_ChangeDirPrev(FsDirectoryEntry **entries, s64 entry_count) {
	char new_cwd[FS_MAX_PATH];
	if (path_cd_up(new_cwd) < 0)
		return -1;

	return FS_ChangeDir(new_cwd, entries, entry_count);
}

static int FS_GetPathEntry(FsDirectoryEntry *entry, char path[FS_MAX_PATH]) {
	int length = snprintf(path, FS_MAX_PATH, "%s%s%s", config.cwd, !strncmp(config.cwd, "/", 2) ? "" : "/", entry != NULL? entry->name : "");
	return length;
}

Result FS_GetTimeStamp(FsDirectoryEntry *entry, FsTimeStampRaw *timestamp) {
	Result ret = 0;

	char path[FS_MAX_PATH];
	if (FS_GetPathEntry(entry, path) <= 0)
		return -1;

	if (R_FAILED(ret = fsFsGetFileTimeStampRaw(fs, path, timestamp)))
		return ret;
	
	return 0;
}

Result FS_Rename(FsDirectoryEntry *entry, const char filename[FS_MAX_PATH]) {
	Result ret = 0;

	char path[FS_MAX_PATH];
	if (FS_GetPathEntry(entry, path) <= 0)
		return -1;

	char new_path[FS_MAX_PATH];
	snprintf(new_path, FS_MAX_PATH, "%s%s%s", config.cwd, !strncmp(config.cwd, "/", 2) ? "" : "/", filename);
	
	if (entry->type == FsDirEntryType_Dir) {
		if (R_FAILED(ret = fsFsRenameDirectory(fs, path, new_path)))
			return ret;
	}
	else {
		if (R_FAILED(ret = fsFsRenameFile(fs, path, new_path)))
			return ret;
	}
	
	return 0;
}

Result FS_Delete(FsDirectoryEntry *entry) {
	Result ret = 0;
	
	char path[FS_MAX_PATH];
	if (FS_GetPathEntry(entry, path) <= 0)
		return -1;
	
	if (entry->type == FsDirEntryType_Dir) {
		if (R_FAILED(ret = fsFsDeleteDirectoryRecursively(fs, path)))
			return ret;
	}
	else {
		if (R_FAILED(ret = fsFsDeleteFile(fs, path)))
			return ret;
	}
	
	return 0;
}

Result FS_SetArchiveBit(FsDirectoryEntry *entry) {
	Result ret = 0;

	char path[FS_MAX_PATH];
	if (FS_GetPathEntry(entry, path) <= 0)
		return -1;
	
	if (R_FAILED(ret = fsdevSetConcatenationFileAttribute(path)))
		return ret;
	
	return 0;
}

static Result FS_CopyFile(char src_path[FS_MAX_PATH], char dest_path[FS_MAX_PATH]) {
	Result ret = 0;
	FsFile src_handle, dest_handle;
	
	if (R_FAILED(ret = fsFsOpenFile(&devices[PREVIOUS_BROWSE_STATE], src_path, FsOpenMode_Read, &src_handle))) {
		fsFileClose(&src_handle);
		return ret;
	}
	
	s64 size = 0;
	if (R_FAILED(ret = fsFileGetSize(&src_handle, &size)))
		return ret;
		
	if (!FS_FileExists(dest_path))
		fsFsCreateFile(fs, dest_path, size, 0);
		
	if (R_FAILED(ret = fsFsOpenFile(fs, dest_path,  FsOpenMode_Write, &dest_handle))) {
		fsFileClose(&src_handle);
		fsFileClose(&dest_handle);
		return ret;
	}
	
	u64 bytes_read = 0;
	s64 offset = 0;
	u64 buf_size = 0x10000;
	void *buf = malloc(buf_size); // Chunk size
	
	do {
		memset(buf, 0, buf_size);
		
		if (R_FAILED(ret = fsFileRead(&src_handle, offset, buf, buf_size, FsReadOption_None, &bytes_read))) {
			free(buf);
			fsFileClose(&src_handle);
			fsFileClose(&dest_handle);
			return ret;
		}
		if (R_FAILED(ret = fsFileWrite(&dest_handle, offset, buf, bytes_read, FsWriteOption_Flush))) {
			free(buf);
			fsFileClose(&src_handle);
			fsFileClose(&dest_handle);
			return ret;
		}
		
		offset += bytes_read;
	} while(offset < size);
	
	free(buf);
	fsFileClose(&src_handle);
	fsFileClose(&dest_handle);
	return 0;
}

static Result FS_CopyDir(char src_path[FS_MAX_PATH], char dest_path[FS_MAX_PATH]) {
	Result ret = 0;
	FsDir dir;
	
	if (R_FAILED(ret = fsFsOpenDirectory(&devices[PREVIOUS_BROWSE_STATE], src_path, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles, &dir)))
		return ret;

	// This may fail or not, but we don't care -> make the dir if it doesn't exist otherwise it fails.
	fsFsCreateDirectory(fs, dest_path);

	s64 entry_count = 0;
	if (R_FAILED(ret = fsDirGetEntryCount(&dir, &entry_count)))
		return ret;

	FsDirectoryEntry *entries = (FsDirectoryEntry *)malloc(entry_count * sizeof(*entries));
	if (R_FAILED(ret = fsDirRead(&dir, NULL, (size_t)entry_count, entries))) {
		free(entries);
		return ret;
	}

	for (s64 i = 0; i < entry_count; i++) {
		if (strlen(entries[i].name) > 0) {
			// Calculate Buffer Size
			int insize = strlen(src_path) + strlen(entries[i].name) + 2;
			int outsize = strlen(dest_path) + strlen(entries[i].name) + 2;
			
			// Allocate Buffer
			char *inbuffer = (char *)malloc(insize);
			char *outbuffer = (char *)malloc(outsize);
			
			// Puzzle Input Path
			strcpy(inbuffer, src_path);
			inbuffer[strlen(inbuffer) + 1] = 0;
			inbuffer[strlen(inbuffer)] = '/';
			strcpy(inbuffer + strlen(inbuffer), entries[i].name);
			
			// Puzzle Output Path
			strcpy(outbuffer, dest_path);
			outbuffer[strlen(outbuffer) + 1] = 0;
			outbuffer[strlen(outbuffer)] = '/';
			strcpy(outbuffer + strlen(outbuffer), entries[i].name);

			if (entries[i].type == FsDirEntryType_Dir)
				FS_CopyDir(inbuffer, outbuffer); // Copy Folder (via recursion)
			else
				FS_CopyFile(inbuffer, outbuffer); // Copy File
			
			free(inbuffer);
			free(outbuffer);
		}
	}
	
	free(entries);
	fsDirClose(&dir);
	return 0;
}

void FS_Copy(FsDirectoryEntry *entry) {
	char path[FS_MAX_PATH];
	if (FS_GetPathEntry(entry, path) <= 0)
		return;
	
	strcpy(fs_copy_struct.copy_path, path);
	strcpy(fs_copy_struct.copy_filename, entry->name);
	
	if (entry->type == FsDirEntryType_Dir)
		fs_copy_struct.is_dir = true;
}

Result FS_Paste(void) {
	Result ret = 0;

	char path[FS_MAX_PATH];
	snprintf(path, FS_MAX_PATH, "%s%s%s", config.cwd, !strncmp(config.cwd, "/", 2) ? "" : "/", fs_copy_struct.copy_filename);
	
	if (fs_copy_struct.is_dir) // Copy folder recursively
		ret = FS_CopyDir(fs_copy_struct.copy_path, path);
	else // Copy file
		ret = FS_CopyFile(fs_copy_struct.copy_path, path);
	
	memset(fs_copy_struct.copy_path, 0, FS_MAX_PATH);
	memset(fs_copy_struct.copy_filename, 0, FS_MAX_PATH);
	fs_copy_struct.is_dir = false;
	return ret;
}

Result FS_Move(void) {
	Result ret = 0;
	char path[FS_MAX_PATH];
	snprintf(path, FS_MAX_PATH, "%s%s%s", config.cwd, !strncmp(config.cwd, "/", 2) ? "" : "/", fs_copy_struct.copy_filename);

	if (fs_copy_struct.is_dir) {
		if (R_FAILED(ret = fsFsRenameDirectory(fs, fs_copy_struct.copy_path, path)))
			return ret;
	}
	else {
		if (R_FAILED(ret = fsFsRenameFile(fs, fs_copy_struct.copy_path, path)))
			return ret;
	}

	memset(fs_copy_struct.copy_path, 0, FS_MAX_PATH);
	memset(fs_copy_struct.copy_filename, 0, FS_MAX_PATH);
	fs_copy_struct.is_dir = false;
	return 0;
}

Result FS_GetFileSize(const char path[FS_MAX_PATH], s64 *size) {
	Result ret = 0;
	
	FsFile file;
	if (R_FAILED(ret = fsFsOpenFile(fs, path, FsOpenMode_Read, &file))) {
		fsFileClose(&file);
		return ret;
	}
	
	if (R_FAILED(ret = fsFileGetSize(&file, size))) {
		fsFileClose(&file);
		return ret;
	}
	
	fsFileClose(&file);
	return 0;
}

Result FS_ReadFile(const char path[FS_MAX_PATH], void *buf, u64 size) {
	Result ret = 0;
	
	FsFile file;
	if (R_FAILED(ret = fsFsOpenFile(fs, path, FsOpenMode_Read, &file))) {
		fsFileClose(&file);
		return ret;
	}
	
	u64 bytes_read = 0;
	if (R_FAILED(ret = fsFileRead(&file, 0, buf, size, FsReadOption_None, &bytes_read))) {
		fsFileClose(&file);
		return ret;
	}
	
	fsFileClose(&file);
	return 0;
}

Result FS_WriteFile(const char path[FS_MAX_PATH], const void *buf, u64 size) {
	Result ret = 0;
	
	if (FS_FileExists(path)) {
		if (R_FAILED(ret = fsFsDeleteFile(fs, path)))
			return ret;
	}

	// Attempy to create the file regardless if it exists or not, we don't care about the return value here.
	fsFsCreateFile(fs, path, size, 0);
	
	FsFile file;
	if (R_FAILED(ret = fsFsOpenFile(fs, path, FsOpenMode_Write, &file))) {
		fsFileClose(&file);
		return ret;
	}

	if (R_FAILED(ret = fsFileWrite(&file, 0, buf, size, FsWriteOption_Flush))) {
		fsFileClose(&file);
		return ret;
	}

	fsFileClose(&file);
	return 0;
}
