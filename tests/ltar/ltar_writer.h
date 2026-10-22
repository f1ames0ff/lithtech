#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class LtarWriter
{
public:
    LtarWriter();

    bool AddFile(const std::string &path, const std::vector<uint8_t> &data, uint32_t flags = 0);
    bool AddFile(const std::string &path, const void *data, size_t size, uint32_t flags = 0);
    bool AddDirectory(const std::string &path);

    void SetFlag(uint32_t flag);
    void SetGuid(const uint8_t guid[16]);

    bool Build(std::vector<uint8_t> &archive, std::string *error = nullptr) const;
    bool Write(const std::string &filename, std::string *error = nullptr) const;

    uint32_t GetFileCount() const;
    uint32_t GetDirectoryCount() const;

private:
    struct File
    {
        std::string m_Path;
        std::vector<uint8_t> m_Data;
        uint32_t m_Flags;
    };

    uint32_t EnsureDirectory(const std::string &path);

    std::vector<File> m_Files;
    std::vector<std::string> m_Directories;
    uint32_t m_Flag;
    uint8_t m_Guid[16];
};
