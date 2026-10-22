#include "ltar_writer.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <map>
#include <utility>

namespace
{
constexpr uint32_t kInvalidIndex = 0xFFFFFFFFu;

std::string NormalizeSeparators(const std::string &path)
{
    std::string result(path);
    std::replace(result.begin(), result.end(), '/', '\\');
    return result;
}

std::string DirectoryOf(const std::string &path)
{
    const size_t position = path.find_last_of('\\');
    if (position == std::string::npos)
        return std::string();
    return path.substr(0, position);
}

std::string FileNameOf(const std::string &path)
{
    const size_t position = path.find_last_of('\\');
    if (position == std::string::npos)
        return path;
    return path.substr(position + 1);
}

void AppendU32(std::vector<uint8_t> &out, uint32_t value)
{
    out.push_back(static_cast<uint8_t>(value));
    out.push_back(static_cast<uint8_t>(value >> 8));
    out.push_back(static_cast<uint8_t>(value >> 16));
    out.push_back(static_cast<uint8_t>(value >> 24));
}

void AppendU64(std::vector<uint8_t> &out, uint64_t value)
{
    for (int shift = 0; shift < 64; shift += 8)
        out.push_back(static_cast<uint8_t>(value >> shift));
}

void SetError(std::string *error, const std::string &message)
{
    if (error != nullptr)
        *error = message;
}
}

LtarWriter::LtarWriter()
    : m_Flag(1)
{
    m_Directories.push_back(std::string());
    for (size_t i = 0; i < sizeof(m_Guid); ++i)
        m_Guid[i] = 0;
}

bool LtarWriter::AddDirectory(const std::string &path)
{
    std::string normalized = NormalizeSeparators(path);
    while (!normalized.empty() && normalized.back() == '\\')
        normalized.pop_back();
    if (normalized.empty())
        return true;
    if (normalized.find('\0') != std::string::npos)
        return false;
    if (m_Directories.size() >= std::numeric_limits<uint32_t>::max())
        return false;
    EnsureDirectory(normalized);
    return true;
}

bool LtarWriter::AddFile(const std::string &path, const std::vector<uint8_t> &data, uint32_t flags)
{
    return AddFile(path, data.empty() ? nullptr : data.data(), data.size(), flags);
}

bool LtarWriter::AddFile(const std::string &path, const void *data, size_t size, uint32_t flags)
{
    if (data == nullptr && size != 0)
        return false;
    if (path.find('\0') != std::string::npos)
        return false;
    const std::string normalized = NormalizeSeparators(path);
    const std::string name = FileNameOf(normalized);
    if (name.empty())
        return false;
    const std::string directory = DirectoryOf(normalized);
    if (m_Directories.size() >= std::numeric_limits<uint32_t>::max())
        return false;
    EnsureDirectory(directory);
    File file;
    file.m_Path = normalized;
    file.m_Flags = flags;
    if (size != 0)
    {
        const uint8_t *bytes = static_cast<const uint8_t *>(data);
        file.m_Data.assign(bytes, bytes + size);
    }
    m_Files.push_back(std::move(file));
    return true;
}

void LtarWriter::SetFlag(uint32_t flag)
{
    m_Flag = flag;
}

void LtarWriter::SetGuid(const uint8_t guid[16])
{
    if (guid == nullptr)
    {
        for (size_t i = 0; i < sizeof(m_Guid); ++i)
            m_Guid[i] = 0;
        return;
    }
    for (size_t i = 0; i < sizeof(m_Guid); ++i)
        m_Guid[i] = guid[i];
}

bool LtarWriter::Build(std::vector<uint8_t> &archive, std::string *error) const
{
    if (error != nullptr)
        error->clear();
    if (m_Directories.empty())
    {
        SetError(error, "no root directory");
        return false;
    }
    if (m_Directories.size() > std::numeric_limits<uint32_t>::max())
    {
        SetError(error, "too many directories");
        return false;
    }
    if (m_Files.size() > std::numeric_limits<uint32_t>::max())
    {
        SetError(error, "too many files");
        return false;
    }

    std::vector<std::vector<size_t>> filesByDirectory(m_Directories.size());
    for (size_t fileIndex = 0; fileIndex < m_Files.size(); ++fileIndex)
    {
        const std::string directory = DirectoryOf(m_Files[fileIndex].m_Path);
        size_t directoryIndex = m_Directories.size();
        for (size_t i = 0; i < m_Directories.size(); ++i)
        {
            if (m_Directories[i] == directory)
            {
                directoryIndex = i;
                break;
            }
        }
        if (directoryIndex == m_Directories.size())
        {
            SetError(error, "file has an unregistered directory: " + m_Files[fileIndex].m_Path);
            return false;
        }
        filesByDirectory[directoryIndex].push_back(fileIndex);
    }

    std::vector<uint8_t> namePool;
    std::map<std::string, uint32_t> nameOffsets;
    auto addName = [&](const std::string &name, uint32_t &offset) -> bool
    {
        const auto found = nameOffsets.find(name);
        if (found != nameOffsets.end())
        {
            offset = found->second;
            return true;
        }
        if (namePool.size() > std::numeric_limits<uint32_t>::max())
            return false;
        offset = static_cast<uint32_t>(namePool.size());
        namePool.insert(namePool.end(), name.begin(), name.end());
        namePool.push_back(0);
        if (namePool.size() > std::numeric_limits<uint32_t>::max())
            return false;
        nameOffsets[name] = offset;
        return true;
    };

    std::vector<uint32_t> directoryNameOffsets(m_Directories.size());
    for (size_t i = 0; i < m_Directories.size(); ++i)
    {
        if (!addName(m_Directories[i], directoryNameOffsets[i]))
        {
            SetError(error, "name pool exceeds 4 GiB");
            return false;
        }
    }

    std::vector<std::vector<uint32_t>> fileNameOffsets(m_Directories.size());
    for (size_t directoryIndex = 0; directoryIndex < m_Directories.size(); ++directoryIndex)
    {
        fileNameOffsets[directoryIndex].resize(filesByDirectory[directoryIndex].size());
        for (size_t i = 0; i < filesByDirectory[directoryIndex].size(); ++i)
        {
            const size_t fileIndex = filesByDirectory[directoryIndex][i];
            if (!addName(FileNameOf(m_Files[fileIndex].m_Path), fileNameOffsets[directoryIndex][i]))
            {
                SetError(error, "name pool exceeds 4 GiB");
                return false;
            }
        }
    }

    uint64_t totalDataSize = 0;
    for (const File &file : m_Files)
    {
        if (totalDataSize > std::numeric_limits<uint64_t>::max() - file.m_Data.size())
        {
            SetError(error, "archive data exceeds 64-bit range");
            return false;
        }
        totalDataSize += file.m_Data.size();
    }

    const uint64_t dataStart64 = 48ULL + namePool.size() + 32ULL * m_Files.size() + 16ULL * m_Directories.size();
    if (dataStart64 > std::numeric_limits<size_t>::max() || totalDataSize > std::numeric_limits<size_t>::max() - dataStart64)
    {
        SetError(error, "archive exceeds addressable memory");
        return false;
    }
    const size_t dataStart = static_cast<size_t>(dataStart64);

    std::vector<std::vector<uint32_t>> children(m_Directories.size());
    std::vector<uint32_t> nextSibling(m_Directories.size(), kInvalidIndex);
    for (size_t i = 1; i < m_Directories.size(); ++i)
    {
        const std::string parent = DirectoryOf(m_Directories[i]);
        for (size_t parentIndex = 0; parentIndex < m_Directories.size(); ++parentIndex)
        {
            if (m_Directories[parentIndex] == parent)
            {
                children[parentIndex].push_back(static_cast<uint32_t>(i));
                break;
            }
        }
    }
    for (const std::vector<uint32_t> &list : children)
    {
        for (size_t i = 0; i + 1 < list.size(); ++i)
            nextSibling[list[i]] = list[i + 1];
    }

    archive.clear();
    archive.reserve(dataStart + static_cast<size_t>(totalDataSize));
    archive.push_back('L');
    archive.push_back('T');
    archive.push_back('A');
    archive.push_back('R');
    AppendU32(archive, 3);
    AppendU32(archive, static_cast<uint32_t>(namePool.size()));
    AppendU32(archive, static_cast<uint32_t>(m_Directories.size()));
    AppendU32(archive, static_cast<uint32_t>(m_Files.size()));
    AppendU32(archive, 1);
    AppendU32(archive, 0);
    AppendU32(archive, m_Flag);
    archive.insert(archive.end(), m_Guid, m_Guid + sizeof(m_Guid));
    archive.insert(archive.end(), namePool.begin(), namePool.end());

    uint64_t dataOffset = dataStart64;
    for (size_t directoryIndex = 0; directoryIndex < m_Directories.size(); ++directoryIndex)
    {
        for (size_t i = 0; i < filesByDirectory[directoryIndex].size(); ++i)
        {
            const File &file = m_Files[filesByDirectory[directoryIndex][i]];
            AppendU32(archive, fileNameOffsets[directoryIndex][i]);
            AppendU64(archive, dataOffset);
            AppendU64(archive, file.m_Data.size());
            AppendU64(archive, file.m_Data.size());
            AppendU32(archive, file.m_Flags);
            dataOffset += file.m_Data.size();
        }
    }

    for (size_t i = 0; i < m_Directories.size(); ++i)
    {
        const uint32_t firstChild = children[i].empty() ? kInvalidIndex : children[i][0];
        AppendU32(archive, directoryNameOffsets[i]);
        AppendU32(archive, firstChild);
        AppendU32(archive, nextSibling[i]);
        AppendU32(archive, static_cast<uint32_t>(filesByDirectory[i].size()));
    }

    for (size_t directoryIndex = 0; directoryIndex < m_Directories.size(); ++directoryIndex)
    {
        for (size_t fileIndex : filesByDirectory[directoryIndex])
        {
            const std::vector<uint8_t> &data = m_Files[fileIndex].m_Data;
            archive.insert(archive.end(), data.begin(), data.end());
        }
    }

    if (archive.size() != dataStart + static_cast<size_t>(totalDataSize))
    {
        SetError(error, "internal layout mismatch");
        return false;
    }
    return true;
}

bool LtarWriter::Write(const std::string &filename, std::string *error) const
{
    std::vector<uint8_t> archive;
    if (!Build(archive, error))
        return false;
    std::ofstream stream(filename, std::ios::binary | std::ios::trunc);
    if (!stream)
    {
        SetError(error, "failed to open output file: " + filename);
        return false;
    }
    if (!archive.empty())
        stream.write(reinterpret_cast<const char *>(archive.data()), static_cast<std::streamsize>(archive.size()));
    if (!stream)
    {
        SetError(error, "failed to write output file: " + filename);
        return false;
    }
    stream.close();
    if (!stream)
    {
        SetError(error, "failed to close output file: " + filename);
        return false;
    }
    return true;
}

uint32_t LtarWriter::GetFileCount() const
{
    return static_cast<uint32_t>(m_Files.size());
}

uint32_t LtarWriter::GetDirectoryCount() const
{
    return static_cast<uint32_t>(m_Directories.size());
}

uint32_t LtarWriter::EnsureDirectory(const std::string &path)
{
    if (path.empty())
        return 0;
    for (size_t i = 0; i < m_Directories.size(); ++i)
    {
        if (m_Directories[i] == path)
            return static_cast<uint32_t>(i);
    }
    const std::string parent = DirectoryOf(path);
    if (!parent.empty())
        EnsureDirectory(parent);
    m_Directories.push_back(path);
    return static_cast<uint32_t>(m_Directories.size() - 1);
}
