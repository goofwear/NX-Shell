#include <string>
#include <switch.h>

extern FsFileSystem *fs;
extern FsFileSystem devices[4];

bool FS_FileExists(const char path[FS_MAX_PATH]);
bool FS_DirExists(const char path[FS_MAX_PATH]);
s64 FS_GetDirList(char path[FS_MAX_PATH], FsDirectoryEntry **entriesp);
void FS_FreeDirEntries(FsDirectoryEntry **entries, s64 entry_count);
s64 FS_RefreshEntries(FsDirectoryEntry **entries, s64 entry_count);
s64 FS_ChangeDirNext(const char path[FS_MAX_PATH], FsDirectoryEntry **entries, s64 entry_count);
s64 FS_ChangeDirPrev(FsDirectoryEntry **entries, s64 entry_count);
Result FS_GetTimeStamp(FsDirectoryEntry *entry, FsTimeStampRaw *timestamp);
Result FS_Rename(FsDirectoryEntry *entry, const char filename[FS_MAX_PATH]);
Result FS_Delete(FsDirectoryEntry *entry);
Result FS_SetArchiveBit(FsDirectoryEntry *entry);
void FS_Copy(FsDirectoryEntry *entry);
Result FS_Paste(void);
Result FS_Move(void);
Result FS_GetFileSize(const char path[FS_MAX_PATH], s64 *size);
Result FS_ReadFile(const char path[FS_MAX_PATH], void *buf, u64 size);
Result FS_WriteFile(const char path[FS_MAX_PATH], const void *buf, u64 size);
