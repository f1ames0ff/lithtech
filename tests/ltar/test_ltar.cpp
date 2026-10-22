#include "ltar.h"
#include "ltar_writer.h"

#include <chrono>
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
int g_Failures = 0;
bool g_Skipped = false;

struct TestContext
{
    std::filesystem::path m_Directory;
};

using TestFunction = void (*)(const TestContext &);

void Check(bool condition, const char *expression, const char *file, int line)
{
    if (!condition)
    {
        ++g_Failures;
        std::cout << "  FAIL " << expression << " (" << file << ":" << line << ")\n";
    }
}

void CheckEqual(uint64_t actual, uint64_t expected, const char *expression, const char *file, int line)
{
    if (actual != expected)
    {
        ++g_Failures;
        std::cout << "  FAIL " << expression << ": expected " << expected << ", got " << actual
                  << " (" << file << ":" << line << ")\n";
    }
}

void CheckString(const std::string &actual, const std::string &expected, const char *expression, const char *file, int line)
{
    if (actual != expected)
    {
        ++g_Failures;
        std::cout << "  FAIL " << expression << ": expected \"" << expected << "\", got \"" << actual
                  << "\" (" << file << ":" << line << ")\n";
    }
}

void CheckBytes(const std::vector<uint8_t> &actual, const std::vector<uint8_t> &expected, const char *expression, const char *file, int line)
{
    if (actual.size() != expected.size())
    {
        ++g_Failures;
        std::cout << "  FAIL " << expression << ": expected " << expected.size() << " bytes, got "
                  << actual.size() << " (" << file << ":" << line << ")\n";
        return;
    }
    for (size_t i = 0; i < actual.size(); ++i)
    {
        if (actual[i] != expected[i])
        {
            ++g_Failures;
            std::cout << "  FAIL " << expression << ": byte " << i << " expected "
                      << static_cast<unsigned>(expected[i]) << ", got " << static_cast<unsigned>(actual[i])
                      << " (" << file << ":" << line << ")\n";
            return;
        }
    }
}

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)
#define CHECK_EQ(actual, expected) CheckEqual(static_cast<uint64_t>(actual), static_cast<uint64_t>(expected), #actual, __FILE__, __LINE__)
#define CHECK_STRING(actual, expected) CheckString((actual), (expected), #actual, __FILE__, __LINE__)
#define CHECK_BYTES(actual, expected) CheckBytes((actual), (expected), #actual, __FILE__, __LINE__)

void RunCase(const char *name, TestFunction function, const TestContext &context)
{
    const int failuresBefore = g_Failures;
    g_Skipped = false;
    std::cout << "RUN  " << name << "\n";
    function(context);
    if (g_Failures != failuresBefore)
        std::cout << "FAIL " << name << "\n";
    else if (g_Skipped)
        std::cout << "SKIP " << name << "\n";
    else
        std::cout << "PASS " << name << "\n";
}

bool WriteFile(const std::filesystem::path &path, const std::vector<uint8_t> &data)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream)
        return false;
    if (!data.empty())
        stream.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(stream);
}

std::vector<uint8_t> Prefix(const std::vector<uint8_t> &data, size_t size)
{
    if (size > data.size())
        size = data.size();
    return std::vector<uint8_t>(data.begin(), data.begin() + static_cast<std::ptrdiff_t>(size));
}

uint32_t ReadU32(const std::vector<uint8_t> &data, size_t offset)
{
    return static_cast<uint32_t>(data[offset]) |
           (static_cast<uint32_t>(data[offset + 1]) << 8) |
           (static_cast<uint32_t>(data[offset + 2]) << 16) |
           (static_cast<uint32_t>(data[offset + 3]) << 24);
}

void TestEmptyVolume(const TestContext &context)
{
    LtarWriter writer;
    const std::filesystem::path path = context.m_Directory / "empty.Arch00";
    const std::string pathString = path.string();
    std::string error;
    CHECK(writer.Write(pathString, &error));
    CHECK(error.empty());

    CLtarArchive archive;
    CHECK(archive.Open(pathString.c_str(), &error));
    CHECK(archive.IsOpen());
    CHECK_EQ(archive.GetVersion(), 3);
    CHECK_EQ(archive.GetDirCount(), 1);
    CHECK_EQ(archive.GetFileCount(), 0);
    CHECK_EQ(archive.GetEntryCount(), 0);
    CHECK(archive.Find("missing.bin") == nullptr);

    error.clear();
    CHECK(archive.Verify(&error));
    CHECK(error.empty());

    archive.Close();
    CHECK(!archive.IsOpen());
}

void TestNestedDirectories(const TestContext &context)
{
    const std::vector<uint8_t> alpha = {0x10, 0x20, 0x30, 0x40, 0x50};
    const std::vector<uint8_t> beta = {0x42, 0x45, 0x54, 0x41, 0x00, 0x58};
    std::vector<uint8_t> gamma(128);
    for (size_t i = 0; i < gamma.size(); ++i)
        gamma[i] = static_cast<uint8_t>(i);

    LtarWriter writer;
    CHECK(writer.AddFile("data/alpha.bin", alpha));
    CHECK(writer.AddFile("data/nested/beta.bin", beta));
    CHECK(writer.AddFile("data\\nested\\gamma.bin", gamma));

    const std::filesystem::path path = context.m_Directory / "nested.Arch00";
    const std::string pathString = path.string();
    std::string error;
    CHECK(writer.Write(pathString, &error));
    CHECK(error.empty());

    CLtarArchive archive;
    CHECK(archive.Open(pathString.c_str(), &error));
    CHECK(archive.IsOpen());
    CHECK_EQ(archive.GetVersion(), 3);
    CHECK_EQ(archive.GetDirCount(), 3);
    CHECK_EQ(archive.GetFileCount(), 3);
    CHECK_EQ(archive.GetEntryCount(), 3);

    const CLtarArchive::Entry *alphaEntry = archive.Find("data/alpha.bin");
    CHECK(alphaEntry != nullptr);
    if (alphaEntry != nullptr)
    {
        CHECK_EQ(alphaEntry->m_PackedSize, alpha.size());
        CHECK_EQ(alphaEntry->m_UnpackedSize, alpha.size());
        CHECK_EQ(alphaEntry->m_Flags, 0);
        std::vector<uint8_t> data;
        CHECK(archive.ReadAll(*alphaEntry, data));
        CHECK_BYTES(data, alpha);
    }

    const CLtarArchive::Entry *betaEntry = archive.Find("data\\nested\\beta.bin");
    CHECK(betaEntry != nullptr);
    if (betaEntry != nullptr)
    {
        std::vector<uint8_t> data;
        CHECK(archive.ReadAll(*betaEntry, data));
        CHECK_BYTES(data, beta);
    }

    const CLtarArchive::Entry *gammaEntry = archive.Find("data\\nested\\gamma.bin");
    CHECK(gammaEntry != nullptr);
    if (gammaEntry != nullptr)
    {
        std::vector<uint8_t> data;
        CHECK(archive.ReadAll(*gammaEntry, data));
        CHECK_BYTES(data, gamma);
    }

    error.clear();
    CHECK(archive.Verify(&error));
    CHECK(error.empty());
}

void TestDuplicateNormalizedPaths(const TestContext &context)
{
    const std::vector<uint8_t> first = {0xA1, 0xA2, 0xA3};
    const std::vector<uint8_t> second = {0xB1, 0xB2, 0xB3, 0xB4};

    LtarWriter writer;
    CHECK(writer.AddFile("a/x.bin", first));
    CHECK(writer.AddFile("A/X.BIN", second));

    const std::filesystem::path path = context.m_Directory / "duplicate.Arch00";
    const std::string pathString = path.string();
    std::string error;
    CHECK(writer.Write(pathString, &error));
    CHECK(error.empty());

    CLtarArchive archive;
    CHECK(archive.Open(pathString.c_str(), &error));
    CHECK(archive.IsOpen());
    CHECK_EQ(archive.GetDirCount(), 3);
    CHECK_EQ(archive.GetFileCount(), 2);
    CHECK_EQ(archive.GetEntryCount(), 2);

    const std::string firstNormalized = CLtarArchive::NormalizePath("a/x.bin");
    const std::string secondNormalized = CLtarArchive::NormalizePath("A\\X.BIN");
    CHECK(!firstNormalized.empty());
    CHECK_STRING(firstNormalized, secondNormalized);

    CHECK(archive.Find("a\\x.bin") == &archive.GetEntry(0));
    CHECK(archive.Find("A\\X.BIN") == &archive.GetEntry(0));

    std::vector<uint8_t> data;
    CHECK(archive.ReadAll(archive.GetEntry(0), data));
    CHECK_BYTES(data, first);
    data.clear();
    CHECK(archive.ReadAll(archive.GetEntry(1), data));
    CHECK_BYTES(data, second);

    error.clear();
    CHECK(archive.Verify(&error));
    CHECK(error.empty());
}

void TestZeroLengthFile(const TestContext &context)
{
    const std::vector<uint8_t> empty;
    const std::vector<uint8_t> after = {9, 8, 7, 6, 5};

    LtarWriter writer;
    CHECK(writer.AddFile("zero/empty.bin", empty));
    CHECK(writer.AddFile("zero/after.bin", after));

    const std::filesystem::path path = context.m_Directory / "zero.Arch00";
    const std::string pathString = path.string();
    std::string error;
    CHECK(writer.Write(pathString, &error));
    CHECK(error.empty());

    CLtarArchive archive;
    CHECK(archive.Open(pathString.c_str(), &error));
    CHECK(archive.IsOpen());
    CHECK_EQ(archive.GetEntryCount(), 2);

    const CLtarArchive::Entry *emptyEntry = archive.Find("zero\\empty.bin");
    CHECK(emptyEntry != nullptr);
    if (emptyEntry != nullptr)
    {
        CHECK_EQ(emptyEntry->m_PackedSize, 0);
        CHECK_EQ(emptyEntry->m_UnpackedSize, 0);
        std::vector<uint8_t> data = {0xFF};
        CHECK(archive.ReadAll(*emptyEntry, data));
        CHECK(data.empty());
    }

    const CLtarArchive::Entry *afterEntry = archive.Find("zero\\after.bin");
    CHECK(afterEntry != nullptr);
    if (afterEntry != nullptr)
    {
        std::vector<uint8_t> data;
        CHECK(archive.ReadAll(*afterEntry, data));
        CHECK_BYTES(data, after);
    }

    error.clear();
    CHECK(archive.Verify(&error));
    CHECK(error.empty());
}

void TestTruncatedArchives(const TestContext &context)
{
    LtarWriter writer;
    CHECK(writer.AddFile("d/one.bin", std::vector<uint8_t>{1, 2, 3, 4, 5, 6, 7, 8}));
    CHECK(writer.AddFile("d/two.bin", std::vector<uint8_t>{9, 10, 11, 12}));

    std::vector<uint8_t> valid;
    std::string error;
    CHECK(writer.Build(valid, &error));
    CHECK(error.empty());
    CHECK(valid.size() >= 48);
    if (valid.size() < 48)
        return;

    const uint32_t namePoolSize = ReadU32(valid, 8);
    const uint32_t dirCount = ReadU32(valid, 12);
    const uint32_t fileCount = ReadU32(valid, 16);
    CHECK_EQ(fileCount, 2);
    const size_t recordStart = 48 + namePoolSize;
    const size_t dataStart = recordStart + 32 * static_cast<size_t>(fileCount) + 16 * static_cast<size_t>(dirCount);
    CHECK(valid.size() > dataStart);
    if (valid.size() <= dataStart)
        return;

    const std::filesystem::path recordPath = context.m_Directory / "truncated_record.Arch00";
    const std::string recordPathString = recordPath.string();
    CHECK(WriteFile(recordPath, Prefix(valid, recordStart + 8)));
    CLtarArchive recordArchive;
    error.clear();
    const bool recordOpened = recordArchive.Open(recordPathString.c_str(), &error);
    CHECK(!recordOpened);
    CHECK(!error.empty());

    const std::filesystem::path dataPath = context.m_Directory / "truncated_data.Arch00";
    const std::string dataPathString = dataPath.string();
    CHECK(WriteFile(dataPath, Prefix(valid, dataStart + 3)));
    CLtarArchive dataArchive;
    error.clear();
    const bool dataOpened = dataArchive.Open(dataPathString.c_str(), &error);
    const std::string openError = error;
    error.clear();
    const bool dataVerified = dataArchive.Verify(&error);
    CHECK(!dataOpened || !dataVerified);
    CHECK(!openError.empty() || !error.empty());
}

void TestPathTraversalEntry(const TestContext &context)
{
    const std::vector<uint8_t> payload = {0x45, 0x56, 0x49, 0x4C};

    LtarWriter writer;
    CHECK(writer.AddFile("..\\evil.bin", payload));

    const std::filesystem::path path = context.m_Directory / "traversal.Arch00";
    const std::string pathString = path.string();
    std::string error;
    CHECK(writer.Write(pathString, &error));
    CHECK(error.empty());

    const std::string normalized = CLtarArchive::NormalizePath("..\\evil.bin");
    const std::string alias = LtarNormalizePath("..\\evil.bin");
    CHECK_STRING(normalized, alias);

    CLtarArchive archive;
    error.clear();
    const bool opened = archive.Open(pathString.c_str(), &error);
    bool rejected = normalized.empty();
    if (opened)
        rejected = rejected || archive.Find("..\\evil.bin") == nullptr;
    else
        rejected = rejected || !error.empty();
    CHECK(rejected);
}

void TestRetailSmoke(const TestContext &)
{
    const std::string retailPath = "D:\\SteamLibrary\\steamapps\\common\\FEAR Ultimate Shooter Edition\\FEAR.Arch00";
    std::error_code existsError;
    if (!std::filesystem::exists(retailPath, existsError) || existsError)
    {
        g_Skipped = true;
        std::cout << "  SKIP retail archive not found: " << retailPath << "\n";
        return;
    }

    CLtarArchive archive;
    std::string error;
    const bool opened = archive.Open(retailPath.c_str(), &error);
    CHECK(opened);
    if (!opened)
    {
        std::cout << "  error: " << error << "\n";
        return;
    }

    CHECK_EQ(archive.GetEntryCount(), 10571);
    const CLtarArchive::Entry *entry = archive.Find("Streets.World00p");
    if (entry == nullptr)
    {
        std::cout << "  WARNING: Find(\"Streets.World00p\") returned null because the library requires "
                     "the full archive path; using Worlds\\ReleaseMultiplayer\\Streets.World00p\n";
        entry = archive.Find("Worlds\\ReleaseMultiplayer\\Streets.World00p");
    }
    CHECK(entry != nullptr);
    if (entry != nullptr)
    {
        CHECK_EQ(entry->m_DataOffset, 3817537327ULL);
        CHECK_EQ(entry->m_PackedSize, 28980611ULL);
        CHECK_EQ(entry->m_UnpackedSize, 28980611ULL);
        uint8_t bytes[4] = {};
        uint64_t read = 0;
        CHECK(archive.Read(*entry, 0, sizeof(bytes), bytes, &read));
        CHECK_EQ(read, 4);
        CHECK(bytes[0] == 0x71);
        CHECK(bytes[1] == 0x00);
        CHECK(bytes[2] == 0x00);
        CHECK(bytes[3] == 0x00);
    }
}
}

int main()
{
    std::error_code errorCode;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    TestContext context;
    context.m_Directory = std::filesystem::temp_directory_path(errorCode);
    if (errorCode)
    {
        std::cout << "FAIL unable to resolve temp directory: " << errorCode.message() << "\n";
        return 1;
    }
    context.m_Directory /= "ltar_tests_" + std::to_string(stamp);
    std::filesystem::create_directories(context.m_Directory, errorCode);
    if (errorCode)
    {
        std::cout << "FAIL unable to create temp directory: " << errorCode.message() << "\n";
        return 1;
    }

    std::cout << "LTAR test directory: " << context.m_Directory.string() << "\n";
    RunCase("empty volume", TestEmptyVolume, context);
    RunCase("nested directories", TestNestedDirectories, context);
    RunCase("duplicate normalized paths", TestDuplicateNormalizedPaths, context);
    RunCase("zero-length file", TestZeroLengthFile, context);
    RunCase("truncated archives", TestTruncatedArchives, context);
    RunCase("path traversal entry", TestPathTraversalEntry, context);
    RunCase("retail smoke", TestRetailSmoke, context);

    std::filesystem::remove_all(context.m_Directory, errorCode);
    if (g_Failures != 0)
    {
        std::cout << "RESULT: FAIL (" << g_Failures << " failed checks)\n";
        return 1;
    }
    std::cout << "RESULT: PASS\n";
    return 0;
}
