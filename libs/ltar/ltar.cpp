#if !defined(_WIN32)
#ifdef _FILE_OFFSET_BITS
#undef _FILE_OFFSET_BITS
#endif
#define _FILE_OFFSET_BITS 64
#endif

#include "ltar.h"

#include <cerrno>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

#if !defined(_WIN32)
#include <stdio.h>
#include <sys/types.h>
#endif

namespace
{
constexpr size_t kHeaderSize = 48;
constexpr size_t kRecordSize = 32;
constexpr size_t kDirectorySize = 16;
constexpr size_t kReadChunkSize = 1024 * 1024;

enum class NameResult
{
    Ok,
    OutOfRange,
    NotTerminated
};

struct RawRecord
{
    uint32_t m_NameOffset;
    uint64_t m_DataOffset;
    uint64_t m_PackedSize;
    uint64_t m_UnpackedSize;
    uint32_t m_Flags;
    std::string m_Name;
};

struct DirRecord
{
    std::string m_Name;
    uint32_t m_FileCount;
};

class FileGuard
{
public:
    explicit FileGuard(std::FILE *pFile)
        : m_pFile(pFile)
    {
    }

    ~FileGuard()
    {
        if (m_pFile != nullptr)
            std::fclose(m_pFile);
    }

    FileGuard(const FileGuard &) = delete;
    FileGuard &operator=(const FileGuard &) = delete;

    std::FILE *Release()
    {
        std::FILE *pFile = m_pFile;
        m_pFile = nullptr;
        return pFile;
    }

private:
    std::FILE *m_pFile;
};

uint32_t ReadU32LE(const unsigned char *pData)
{
    return static_cast<uint32_t>(pData[0])
        | (static_cast<uint32_t>(pData[1]) << 8)
        | (static_cast<uint32_t>(pData[2]) << 16)
        | (static_cast<uint32_t>(pData[3]) << 24);
}

uint64_t ReadU64LE(const unsigned char *pData)
{
    return static_cast<uint64_t>(ReadU32LE(pData))
        | (static_cast<uint64_t>(ReadU32LE(pData + 4)) << 32);
}

bool GetFileSize(std::FILE *pFile, uint64_t &size)
{
#if defined(_WIN32)
    if (_fseeki64(pFile, 0, SEEK_END) != 0)
        return false;
    const long long end = _ftelli64(pFile);
    if (end < 0)
        return false;
    size = static_cast<uint64_t>(end);
    return true;
#else
    if (fseeko(pFile, 0, SEEK_END) != 0)
        return false;
    const off_t end = ftello(pFile);
    if (end < 0)
        return false;
    size = static_cast<uint64_t>(end);
    return true;
#endif
}

bool SeekFile(std::FILE *pFile, uint64_t position)
{
#if defined(_WIN32)
    if (position > static_cast<uint64_t>(std::numeric_limits<long long>::max()))
        return false;
    return _fseeki64(pFile, static_cast<long long>(position), SEEK_SET) == 0;
#else
    if (position > static_cast<uint64_t>(std::numeric_limits<off_t>::max()))
        return false;
    return fseeko(pFile, static_cast<off_t>(position), SEEK_SET) == 0;
#endif
}

bool ReadExact(std::FILE *pFile, void *pBuffer, size_t size)
{
    unsigned char *pBytes = static_cast<unsigned char *>(pBuffer);
    size_t total = 0;
    while (total < size)
    {
        const size_t count = std::fread(pBytes + total, 1, size - total, pFile);
        if (count == 0)
            return false;
        total += count;
    }
    return true;
}

NameResult ReadPoolName(const std::vector<char> &namePool, uint32_t nameOffset, std::string &name)
{
    if (static_cast<size_t>(nameOffset) >= namePool.size())
        return NameResult::OutOfRange;
    const char *pName = namePool.data() + static_cast<size_t>(nameOffset);
    const size_t maxLength = namePool.size() - static_cast<size_t>(nameOffset);
    const void *pTerminator = std::memchr(pName, 0, maxLength);
    if (pTerminator == nullptr)
        return NameResult::NotTerminated;
    const size_t length = static_cast<const char *>(pTerminator) - pName;
    name.assign(pName, length);
    return NameResult::Ok;
}
}

CLtarArchive::CLtarArchive()
    : m_pFile(nullptr)
    , m_FileSize(0)
    , m_Version(0)
    , m_DirCount(0)
    , m_FileCount(0)
{
}

CLtarArchive::~CLtarArchive()
{
    Close();
}

bool CLtarArchive::Open(const char *pszFilename, std::string *pError)
{
    Close();
    if (pError != nullptr)
        pError->clear();

    if (pszFilename == nullptr || pszFilename[0] == '\0')
    {
        if (pError != nullptr)
            *pError = "archive filename is empty";
        return false;
    }

    std::FILE *pFile = std::fopen(pszFilename, "rb");
    if (pFile == nullptr)
    {
        const int openError = errno;
        if (pError != nullptr)
            *pError = std::string("unable to open archive: ") + std::strerror(openError);
        return false;
    }

    FileGuard fileGuard(pFile);
    auto fail = [pError](const std::string &message) -> bool
    {
        if (pError != nullptr)
            *pError = message;
        return false;
    };

    try
    {
        uint64_t fileSize = 0;
        if (!GetFileSize(pFile, fileSize))
            return fail("unable to determine archive file size");
        if (fileSize < static_cast<uint64_t>(kHeaderSize))
            return fail("file is too small to contain an LTAR header");
        if (!SeekFile(pFile, 0))
            return fail("unable to seek to the archive header");

        unsigned char header[kHeaderSize];
        if (!ReadExact(pFile, header, sizeof(header)))
            return fail("unable to read the archive header");
        if (std::memcmp(header, "LTAR", 4) != 0)
            return fail("invalid LTAR magic");

        const uint32_t version = ReadU32LE(header + 4);
        if (version != 3)
            return fail("unsupported LTAR version " + std::to_string(version) + " (expected 3)");

        const uint32_t namePoolSize = ReadU32LE(header + 8);
        const uint32_t dirCount = ReadU32LE(header + 12);
        const uint32_t fileCount = ReadU32LE(header + 16);

        if (static_cast<uint64_t>(namePoolSize) > fileSize - static_cast<uint64_t>(kHeaderSize))
            return fail("name pool extends past the end of the file");

        const uint64_t recordStart = static_cast<uint64_t>(kHeaderSize) + static_cast<uint64_t>(namePoolSize);
        const uint64_t recordBytes = static_cast<uint64_t>(kRecordSize) * static_cast<uint64_t>(fileCount);
        if (recordBytes > fileSize - recordStart)
            return fail("record table extends past the end of the file");

        const uint64_t directoryStart = recordStart + recordBytes;
        const uint64_t directoryBytes = static_cast<uint64_t>(kDirectorySize) * static_cast<uint64_t>(dirCount);
        if (directoryBytes > fileSize - directoryStart)
            return fail("directory table extends past the end of the file");

        if (static_cast<uint64_t>(namePoolSize) > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
            return fail("name pool is too large for this platform");

        std::vector<char> namePool(static_cast<size_t>(namePoolSize));
        if (!ReadExact(pFile, namePool.data(), namePool.size()))
            return fail("unable to read the name pool");

        std::vector<RawRecord> records;
        records.reserve(static_cast<size_t>(fileCount));
        for (uint32_t i = 0; i < fileCount; ++i)
        {
            unsigned char raw[kRecordSize];
            if (!ReadExact(pFile, raw, sizeof(raw)))
                return fail("unable to read the record table");

            RawRecord record;
            record.m_NameOffset = ReadU32LE(raw);
            record.m_DataOffset = ReadU64LE(raw + 4);
            record.m_PackedSize = ReadU64LE(raw + 12);
            record.m_UnpackedSize = ReadU64LE(raw + 20);
            record.m_Flags = ReadU32LE(raw + 28);

            const NameResult nameResult = ReadPoolName(namePool, record.m_NameOffset, record.m_Name);
            if (nameResult == NameResult::OutOfRange)
                return fail("record " + std::to_string(i) + " name offset is outside the name pool");
            if (nameResult == NameResult::NotTerminated)
                return fail("record " + std::to_string(i) + " name is not NUL-terminated");
            if (record.m_Name.empty())
                return fail("record " + std::to_string(i) + " has an empty name");

            records.push_back(std::move(record));
        }

        std::vector<DirRecord> directories;
        directories.reserve(static_cast<size_t>(dirCount));
        uint64_t assignedFiles = 0;
        for (uint32_t i = 0; i < dirCount; ++i)
        {
            unsigned char raw[kDirectorySize];
            if (!ReadExact(pFile, raw, sizeof(raw)))
                return fail("unable to read the directory table");

            DirRecord directory;
            const uint32_t nameOffset = ReadU32LE(raw);
            directory.m_FileCount = ReadU32LE(raw + 12);

            const NameResult nameResult = ReadPoolName(namePool, nameOffset, directory.m_Name);
            if (nameResult == NameResult::OutOfRange)
                return fail("directory " + std::to_string(i) + " name offset is outside the name pool");
            if (nameResult == NameResult::NotTerminated)
                return fail("directory " + std::to_string(i) + " name is not NUL-terminated");

            assignedFiles += static_cast<uint64_t>(directory.m_FileCount);
            if (assignedFiles > static_cast<uint64_t>(fileCount))
                return fail("directory file counts exceed the header file count");
            directories.push_back(std::move(directory));
        }

        if (assignedFiles != static_cast<uint64_t>(fileCount))
        {
            return fail("directory file counts total " + std::to_string(assignedFiles)
                + " but the header declares " + std::to_string(fileCount));
        }

        std::vector<Entry> entries;
        entries.reserve(records.size());
        std::unordered_map<std::string, size_t> lookup;
        lookup.reserve(records.size());
        std::unordered_map<std::string, size_t> fileNameLookup;
        fileNameLookup.reserve(records.size());
        static const size_t kAmbiguousIndex = static_cast<size_t>(-1);

        size_t recordIndex = 0;
        for (size_t d = 0; d < directories.size(); ++d)
        {
            const std::string &directoryName = directories[d].m_Name;
            for (uint32_t k = 0; k < directories[d].m_FileCount; ++k)
            {
                if (recordIndex >= records.size())
                    return fail("directory file counts exceed the record count");

                const RawRecord &record = records[recordIndex];
                std::string fullPath;
                if (!directoryName.empty())
                {
                    fullPath.reserve(directoryName.size() + 1 + record.m_Name.size());
                    fullPath = directoryName;
                    fullPath.push_back('\\');
                }
                fullPath += record.m_Name;

                std::string normalized = NormalizePath(fullPath.c_str());
                if (normalized.empty())
                    return fail("record " + std::to_string(recordIndex) + " path is invalid after normalization");

                Entry entry;
                entry.m_Path = std::move(normalized);
                entry.m_DataOffset = record.m_DataOffset;
                entry.m_PackedSize = record.m_PackedSize;
                entry.m_UnpackedSize = record.m_UnpackedSize;
                entry.m_Flags = record.m_Flags;

                const size_t entryIndex = entries.size();
                entries.push_back(std::move(entry));
                lookup.emplace(entries.back().m_Path, entryIndex);

                const std::string &entryPath = entries.back().m_Path;
                const size_t slash = entryPath.find_last_of('/');
                const std::string fileName = (slash == std::string::npos) ? entryPath : entryPath.substr(slash + 1);
                const std::unordered_map<std::string, size_t>::iterator fileNameIt = fileNameLookup.find(fileName);
                if (fileNameIt == fileNameLookup.end())
                    fileNameLookup.emplace(fileName, entryIndex);
                else
                    fileNameIt->second = kAmbiguousIndex;

                ++recordIndex;
            }
        }

        if (recordIndex != records.size())
            return fail("directory file counts do not cover all records");

        m_FileSize = fileSize;
        m_Version = version;
        m_DirCount = dirCount;
        m_FileCount = fileCount;
        m_Entries = std::move(entries);
        m_Lookup = std::move(lookup);
        m_FileNameLookup = std::move(fileNameLookup);
        m_pFile = fileGuard.Release();
        return true;
    }
    catch (const std::bad_alloc &)
    {
        return fail("out of memory while reading the archive");
    }
    catch (const std::length_error &)
    {
        return fail("archive table is too large to load");
    }
}

void CLtarArchive::Close()
{
    if (m_pFile != nullptr)
    {
        std::fclose(m_pFile);
        m_pFile = nullptr;
    }
    m_FileSize = 0;
    m_Version = 0;
    m_DirCount = 0;
    m_FileCount = 0;
    m_Entries.clear();
    m_Lookup.clear();
    m_FileNameLookup.clear();
}

bool CLtarArchive::IsOpen() const
{
    return m_pFile != nullptr;
}

uint32_t CLtarArchive::GetVersion() const
{
    return m_Version;
}

uint32_t CLtarArchive::GetDirCount() const
{
    return m_DirCount;
}

uint32_t CLtarArchive::GetFileCount() const
{
    return m_FileCount;
}

size_t CLtarArchive::GetEntryCount() const
{
    return m_Entries.size();
}

const CLtarArchive::Entry *CLtarArchive::Find(const char *pszPath) const
{
    const std::string key = NormalizePath(pszPath);
    if (key.empty())
        return nullptr;
    const std::unordered_map<std::string, size_t>::const_iterator it = m_Lookup.find(key);
    if (it != m_Lookup.end())
        return &m_Entries[it->second];

    const size_t slash = key.find_last_of('/');
    if (slash != std::string::npos)
        return nullptr;
    const std::string &fileName = key;
    const std::unordered_map<std::string, size_t>::const_iterator fileNameIt = m_FileNameLookup.find(fileName);
    if (fileNameIt == m_FileNameLookup.end() || fileNameIt->second == static_cast<size_t>(-1))
        return nullptr;
    return &m_Entries[fileNameIt->second];
}

const CLtarArchive::Entry &CLtarArchive::GetEntry(size_t index) const
{
    static const Entry emptyEntry = Entry();
    if (index >= m_Entries.size())
        return emptyEntry;
    return m_Entries[index];
}

size_t CLtarArchive::IndexOf(const Entry &entry) const
{
    if (m_Entries.empty())
        return 0;

    const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(m_Entries.data());
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(&entry);
    if (address < base)
        return m_Entries.size();

    const std::uintptr_t delta = address - base;
    const std::uintptr_t bytes = static_cast<std::uintptr_t>(m_Entries.size() * sizeof(Entry));
    if (delta >= bytes)
        return m_Entries.size();
    if ((delta % sizeof(Entry)) != 0)
        return m_Entries.size();

    return static_cast<size_t>(delta / sizeof(Entry));
}

bool CLtarArchive::Read(const Entry &entry, uint64_t offset, uint64_t size, void *pBuffer, uint64_t *pRead) const
{
    if (pRead != nullptr)
        *pRead = 0;
    if (m_pFile == nullptr)
        return false;

    const size_t index = IndexOf(entry);
    if (index >= m_Entries.size())
        return false;

    const Entry &stored = m_Entries[index];
    if (size != 0 && pBuffer == nullptr)
        return false;
    if (offset > stored.m_PackedSize)
        return false;
    if (size > stored.m_PackedSize - offset)
        return false;
    if (stored.m_DataOffset > m_FileSize)
        return false;

    const uint64_t available = m_FileSize - stored.m_DataOffset;
    if (offset > available)
        return false;
    if (size > available - offset)
        return false;
    if (size == 0)
        return true;

    const uint64_t absolute = stored.m_DataOffset + offset;
    if (!SeekFile(m_pFile, absolute))
        return false;

    unsigned char *pDestination = static_cast<unsigned char *>(pBuffer);
    uint64_t total = 0;
    while (total < size)
    {
        const uint64_t remaining = size - total;
        const size_t request = remaining > static_cast<uint64_t>(kReadChunkSize)
            ? kReadChunkSize
            : static_cast<size_t>(remaining);
        const size_t count = std::fread(pDestination, 1, request, m_pFile);
        total += static_cast<uint64_t>(count);
        pDestination += count;
        if (count != request)
        {
            if (pRead != nullptr)
                *pRead = total;
            return false;
        }
    }

    if (pRead != nullptr)
        *pRead = total;
    return true;
}

bool CLtarArchive::ReadAll(const Entry &entry, std::vector<uint8_t> &data) const
{
    data.clear();
    if (m_pFile == nullptr)
        return false;

    const size_t index = IndexOf(entry);
    if (index >= m_Entries.size())
        return false;

    const Entry &stored = m_Entries[index];
    if (stored.m_DataOffset > m_FileSize || stored.m_PackedSize > m_FileSize - stored.m_DataOffset)
        return false;

    const uint64_t size = stored.m_PackedSize;
    if (size > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
        return false;

    try
    {
        data.resize(static_cast<size_t>(size));
    }
    catch (const std::bad_alloc &)
    {
        data.clear();
        return false;
    }
    catch (const std::length_error &)
    {
        data.clear();
        return false;
    }

    if (size == 0)
        return true;

    uint64_t read = 0;
    if (!Read(entry, 0, size, data.data(), &read))
    {
        data.clear();
        return false;
    }
    if (read != size)
    {
        data.clear();
        return false;
    }
    return true;
}

bool CLtarArchive::Verify(std::string *pError) const
{
    if (pError != nullptr)
        pError->clear();
    if (m_pFile == nullptr)
    {
        if (pError != nullptr)
            *pError = "archive is not open";
        return false;
    }
    if (m_Entries.size() != static_cast<size_t>(m_FileCount))
    {
        if (pError != nullptr)
            *pError = "entry table size does not match the header file count";
        return false;
    }

    for (size_t i = 0; i < m_Entries.size(); ++i)
    {
        const Entry &entry = m_Entries[i];
        if (entry.m_DataOffset > m_FileSize || entry.m_PackedSize > m_FileSize - entry.m_DataOffset)
        {
            if (pError != nullptr)
            {
                *pError = "entry " + std::to_string(i) + " (" + entry.m_Path + ")"
                    + " data range " + std::to_string(entry.m_DataOffset)
                    + " + " + std::to_string(entry.m_PackedSize)
                    + " exceeds file size " + std::to_string(m_FileSize);
            }
            return false;
        }
    }
    return true;
}

std::string CLtarArchive::NormalizePath(const char *pszPath)
{
    if (pszPath == nullptr)
        return std::string();

    std::string result;
    const char *p = pszPath;
    while (*p != '\0')
    {
        while (*p == '/' || *p == '\\')
            ++p;

        const char *pSegment = p;
        while (*p != '\0' && *p != '/' && *p != '\\')
            ++p;

        const size_t length = static_cast<size_t>(p - pSegment);
        if (length == 0)
            continue;
        if (length == 1 && pSegment[0] == '.')
            continue;
        if (length == 2 && pSegment[0] == '.' && pSegment[1] == '.')
            return std::string();

        if (!result.empty())
            result.push_back('/');
        for (size_t i = 0; i < length; ++i)
        {
            char c = pSegment[i];
            if (c >= 'A' && c <= 'Z')
                c = static_cast<char>(c - 'A' + 'a');
            result.push_back(c);
        }
    }
    return result;
}

std::string LtarNormalizePath(const char *pszPath)
{
    return CLtarArchive::NormalizePath(pszPath);
}
