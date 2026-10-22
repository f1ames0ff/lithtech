#ifndef LTAR_H
#define LTAR_H

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

class CLtarArchive
{
public:
    struct Entry
    {
        std::string m_Path;
        uint64_t m_DataOffset;
        uint64_t m_PackedSize;
        uint64_t m_UnpackedSize;
        uint32_t m_Flags;
    };

    CLtarArchive();
    ~CLtarArchive();

    CLtarArchive(const CLtarArchive &) = delete;
    CLtarArchive &operator=(const CLtarArchive &) = delete;

    bool Open(const char *pszFilename, std::string *pError = nullptr);
    void Close();
    bool IsOpen() const;

    uint32_t GetVersion() const;
    uint32_t GetDirCount() const;
    uint32_t GetFileCount() const;
    size_t GetEntryCount() const;

    const Entry *Find(const char *pszPath) const;
    const Entry &GetEntry(size_t index) const;

    bool Read(const Entry &entry, uint64_t offset, uint64_t size, void *pBuffer, uint64_t *pRead) const;
    bool ReadAll(const Entry &entry, std::vector<uint8_t> &data) const;
    bool Verify(std::string *pError = nullptr) const;

    static std::string NormalizePath(const char *pszPath);

private:
    size_t IndexOf(const Entry &entry) const;

    std::FILE *m_pFile;
    uint64_t m_FileSize;
    uint32_t m_Version;
    uint32_t m_DirCount;
    uint32_t m_FileCount;
    std::vector<Entry> m_Entries;
    std::unordered_map<std::string, size_t> m_Lookup;
    std::unordered_map<std::string, size_t> m_FileNameLookup;
};

std::string LtarNormalizePath(const char *pszPath);

#endif
