#include "ltar.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace
{
void PrintUsage(std::ostream &stream)
{
    stream << "ltar_dump - inspect and extract LTAR v3 archives\n"
           << "\n"
           << "Usage:\n"
           << "  ltar_dump list <archive>\n"
           << "  ltar_dump find <archive> <path>\n"
           << "  ltar_dump stat <archive>\n"
           << "  ltar_dump extract <archive> <path> <outdir>\n"
           << "  ltar_dump verify <archive>\n"
           << "  ltar_dump --help\n"
           << "\n"
           << "Commands:\n"
           << "  list     Print index, unpacked size and path for every entry.\n"
           << "  find     Print details for one entry, or exit 1 when it is missing.\n"
           << "  stat     Print archive metadata and entry size summary.\n"
           << "  extract  Write one entry below outdir, creating directories as needed.\n"
           << "  verify   Validate the archive and print OK, or the error.\n"
           << "\n"
           << "Exit codes:\n"
           << "  0  success\n"
           << "  1  runtime failure or entry not found\n"
           << "  2  invalid arguments\n";
}

std::string ErrorText(const std::string &error)
{
    return error.empty() ? std::string("unknown error") : error;
}

int Fail(const std::string &message)
{
    std::cerr << "ltar_dump: " << message << '\n';
    return 1;
}

std::string NormalizedEntryPath(const CLtarArchive::Entry &entry)
{
    const std::string normalized = CLtarArchive::NormalizePath(entry.m_Path.c_str());
    return normalized.empty() ? entry.m_Path : normalized;
}

bool SplitOutputPath(const std::string &path, std::vector<std::string> &parts, std::string &error)
{
    parts.clear();
    std::string segment;
    for (size_t i = 0; i <= path.size(); ++i)
    {
        const char character = i < path.size() ? path[i] : '\\';
        if (character == '\\' || character == '/')
        {
            if (segment.empty() || segment == ".")
            {
                segment.clear();
                continue;
            }
            if (segment == "..")
            {
                error = "refusing path traversal component in output path: " + path;
                return false;
            }
            if (segment.find(':') != std::string::npos)
            {
                error = "refusing drive or stream component in output path: " + path;
                return false;
            }
            parts.push_back(segment);
            segment.clear();
        }
        else
        {
            segment.push_back(character);
        }
    }
    if (parts.empty())
    {
        error = "entry path has no file name: " + path;
        return false;
    }
    return true;
}

std::filesystem::path BuildOutputPath(const std::filesystem::path &outDir, const std::vector<std::string> &parts)
{
    std::filesystem::path result = outDir;
    for (const std::string &part : parts)
        result /= part;
    return result;
}

int CommandList(const std::string &archivePath)
{
    CLtarArchive archive;
    std::string error;
    if (!archive.Open(archivePath.c_str(), &error))
        return Fail("failed to open " + archivePath + ": " + ErrorText(error));

    for (size_t i = 0; i < archive.GetEntryCount(); ++i)
    {
        const CLtarArchive::Entry &entry = archive.GetEntry(i);
        std::cout << i << '\t' << entry.m_UnpackedSize << '\t' << entry.m_Path << '\n';
    }
    return 0;
}

int CommandFind(const std::string &archivePath, const std::string &path)
{
    CLtarArchive archive;
    std::string error;
    if (!archive.Open(archivePath.c_str(), &error))
        return Fail("failed to open " + archivePath + ": " + ErrorText(error));

    const CLtarArchive::Entry *entry = archive.Find(path.c_str());
    if (entry == nullptr)
    {
        std::cerr << "ltar_dump: not found: " << path << '\n';
        return 1;
    }

    size_t index = 0;
    while (index < archive.GetEntryCount() && &archive.GetEntry(index) != entry)
        ++index;
    if (index == archive.GetEntryCount())
    {
        std::cerr << "ltar_dump: entry index not found for " << path << '\n';
        return 1;
    }

    std::cout << "index: " << index << '\n';
    std::cout << "path: " << NormalizedEntryPath(*entry) << '\n';
    std::cout << "data offset: " << entry->m_DataOffset << '\n';
    std::cout << "packed size: " << entry->m_PackedSize << '\n';
    std::cout << "unpacked size: " << entry->m_UnpackedSize << '\n';
    std::cout << "flags: " << entry->m_Flags << '\n';
    return 0;
}

int CommandStat(const std::string &archivePath)
{
    CLtarArchive archive;
    std::string error;
    if (!archive.Open(archivePath.c_str(), &error))
        return Fail("failed to open " + archivePath + ": " + ErrorText(error));

    const size_t entryCount = archive.GetEntryCount();
    uint64_t minSize = 0;
    uint64_t maxSize = 0;
    uint64_t entriesOver2GB = 0;
    for (size_t i = 0; i < entryCount; ++i)
    {
        const CLtarArchive::Entry &entry = archive.GetEntry(i);
        if (i == 0)
        {
            minSize = entry.m_UnpackedSize;
            maxSize = entry.m_UnpackedSize;
        }
        else
        {
            minSize = std::min(minSize, entry.m_UnpackedSize);
            maxSize = std::max(maxSize, entry.m_UnpackedSize);
        }
        if (entry.m_DataOffset > (1ULL << 31))
            ++entriesOver2GB;
    }

    std::error_code sizeError;
    const std::uintmax_t fileSize = std::filesystem::file_size(archivePath, sizeError);
    std::cout << "version: " << archive.GetVersion() << '\n';
    std::cout << "dirCount: " << archive.GetDirCount() << '\n';
    std::cout << "fileCount: " << archive.GetFileCount() << '\n';
    std::cout << "entryCount: " << entryCount << '\n';
    if (sizeError)
        std::cout << "fileSize: unknown\n";
    else
        std::cout << "fileSize: " << fileSize << '\n';
    std::cout << "minEntrySize: " << minSize << '\n';
    std::cout << "maxEntrySize: " << maxSize << '\n';
    std::cout << "entriesOver2GB: " << entriesOver2GB << '\n';
    return 0;
}

int CommandExtract(const std::string &archivePath, const std::string &path, const std::string &outDir)
{
    CLtarArchive archive;
    std::string error;
    if (!archive.Open(archivePath.c_str(), &error))
        return Fail("failed to open " + archivePath + ": " + ErrorText(error));

    const CLtarArchive::Entry *entry = archive.Find(path.c_str());
    if (entry == nullptr)
    {
        std::cerr << "ltar_dump: not found: " << path << '\n';
        return 1;
    }

    std::vector<std::string> parts;
    if (!SplitOutputPath(entry->m_Path, parts, error))
        return Fail(error);

    const std::filesystem::path outputRoot(outDir);
    std::error_code fsError;
    std::filesystem::create_directories(outputRoot, fsError);
    if (fsError)
        return Fail("failed to create output directory " + outDir + ": " + fsError.message());

    const std::filesystem::path outputPath = BuildOutputPath(outputRoot, parts);
    const std::filesystem::path parentPath = outputPath.parent_path();
    if (!parentPath.empty())
    {
        std::filesystem::create_directories(parentPath, fsError);
        if (fsError)
            return Fail("failed to create output directory " + parentPath.string() + ": " + fsError.message());
    }

    std::vector<uint8_t> data;
    if (!archive.ReadAll(*entry, data))
        return Fail("failed to read entry: " + entry->m_Path);

    std::ofstream stream(outputPath, std::ios::binary | std::ios::trunc);
    if (!stream)
        return Fail("failed to open output file: " + outputPath.string());
    if (!data.empty())
        stream.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!stream)
        return Fail("failed to write output file: " + outputPath.string());
    stream.close();
    if (!stream)
        return Fail("failed to close output file: " + outputPath.string());

    std::cout << "extracted " << data.size() << " bytes to " << outputPath.string() << '\n';
    return 0;
}

int CommandVerify(const std::string &archivePath)
{
    CLtarArchive archive;
    std::string error;
    if (!archive.Open(archivePath.c_str(), &error))
        return Fail("failed to open " + archivePath + ": " + ErrorText(error));

    error.clear();
    if (!archive.Verify(&error))
    {
        std::cerr << "ltar_dump: " << ErrorText(error) << '\n';
        return 1;
    }

    std::cout << "OK\n";
    return 0;
}
}

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        const std::string command = argv[1];
        if (command == "--help" || command == "-h" || command == "help")
        {
            PrintUsage(std::cout);
            return 0;
        }
    }

    if (argc < 3)
    {
        PrintUsage(std::cerr);
        return 2;
    }

    const std::string command = argv[1];
    const std::string archivePath = argv[2];
    if (command == "list" && argc == 3)
        return CommandList(archivePath);
    if (command == "find" && argc == 4)
        return CommandFind(archivePath, argv[3]);
    if (command == "stat" && argc == 3)
        return CommandStat(archivePath);
    if (command == "extract" && argc == 5)
        return CommandExtract(archivePath, argv[3], argv[4]);
    if (command == "verify" && argc == 3)
        return CommandVerify(archivePath);

    std::cerr << "ltar_dump: invalid arguments\n";
    PrintUsage(std::cerr);
    return 2;
}
