#include "Crypto.h"
#include "Emblem.h"
#include "Error.h"
#include "ScopeExit.h"
#include "UserData7.h"
#include "aes.hpp"
#include <Windows.h>
#include <cassert>
#include <cinttypes>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

    constexpr uint8_t userDataKey[AES_KEYLEN] = { 0xB1, 0x56, 0x87, 0x9F, 0x13, 0x48, 0x97, 0x98,
                                                  0x70, 0x05, 0xC4, 0x87, 0x00, 0xAE, 0xF8, 0x79 };

    ErrorOr<std::vector<uint8_t>> readBinaryFile(const std::filesystem::path& path) {
        if(!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
            return Error{ "File not found: {}", path.generic_string() };

        std::ifstream ifs(path.c_str(), std::ios::binary);
        if(!ifs.is_open())
            return Error{ "Failed to open file: {}", path.generic_string() };

        auto fileSize = std::filesystem::file_size(path);

        std::vector<uint8_t> data;
        data.resize(fileSize);
        ifs.read(reinterpret_cast<char*>(data.data()), fileSize);
        ifs.close();

        return data;
    }

    ErrorOr<void> writeBinaryFile(const std::filesystem::path& path, const std::vector<uint8_t>& data) {

        std::ofstream ofs(path.c_str(), std::ios::binary);
        if(!ofs.is_open())
            return Error{ "Failed to open file: {}", path.generic_string() };

        ofs.write(reinterpret_cast<const char*>(data.data()), data.size());

        return {};
    }

    ErrorOr<void> openProcessSync(const wchar_t* process, const wchar_t* arguments) noexcept {
        SHELLEXECUTEINFO ShExecInfo = { 0 };
        ShExecInfo.cbSize           = sizeof(SHELLEXECUTEINFO);
        ShExecInfo.fMask            = SEE_MASK_NOCLOSEPROCESS;
        ShExecInfo.hwnd             = NULL;
        ShExecInfo.lpVerb           = NULL;
        ShExecInfo.lpFile           = process;
        ShExecInfo.lpParameters     = arguments;
        ShExecInfo.lpDirectory      = NULL;
        ShExecInfo.nShow            = SW_HIDE;
        ShExecInfo.hInstApp         = NULL;
        if(!ShellExecuteEx(&ShExecInfo))
            return Error{ "ShellExecuteEx failed with error: {}", GetLastError() };

        assert(ShExecInfo.hProcess);
        auto waitResult = WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
        assert(waitResult == WAIT_OBJECT_0);

        DWORD exitCode{};
        GetExitCodeProcess(ShExecInfo.hProcess, &exitCode);
        if(exitCode != 0) {
            CloseHandle(ShExecInfo.hProcess);
            return Error{ "ShellExecuteEx target process exited with code: {}", exitCode };
        }
        CloseHandle(ShExecInfo.hProcess);

        return {};
    }

    ErrorOr<void> packBinder(const std::filesystem::path& path) {
        auto yabberPath = path.parent_path() / "Yabber/Yabber.exe";
        if(!std::filesystem::exists(yabberPath))
            return Error{ "Yabber.exe was not found in the current working directory." };

        return openProcessSync(yabberPath.wstring().c_str(), path.wstring().c_str());
    }

    ErrorOr<void> unpackBinder(const std::filesystem::path& path) {
        auto yabberPath = path.parent_path() / "Yabber/Yabber.exe";
        if(!std::filesystem::exists(yabberPath))
            return Error{ "Yabber.exe was not found in the current working directory." };

        PROPAGATE_IF_ERROR(openProcessSync(yabberPath.wstring().c_str(), path.wstring().c_str()));

        // Sanity check unpacking result
        auto unpackedSl2Dir = (path.parent_path() / path.stem()).concat("-sl2");
        if(!std::filesystem::exists(unpackedSl2Dir) || !std::filesystem::is_directory(unpackedSl2Dir))
            return Error{ "Unexpected Yabber unpacking result" };

        return {};
    }

    ErrorOr<void> Main(const std::vector<std::string_view>& args) {

        // Validate arguments
        if(args.size() < 3)
            return Error{ "Insufficient number of arguments.\n"
                          "EmblemCreator is not a GUI app, drag and drop json emblem files together with a sl2 save "
                          "file on this exe to pack them." };

        std::vector<std::filesystem::path> emblemJsonFilePaths{};
        std::filesystem::path sl2Path{};

        for(int i = 1; i < args.size(); ++i) {
            std::filesystem::path path{ args[i] };
            if(!std::filesystem::is_regular_file(path) || !path.has_extension())
                return Error{ "Invalid argument: {}", path.generic_string() };

            if(path.extension() == ".sl2") {
                sl2Path = path;
            } else if(path.extension() == ".json") {
                emblemJsonFilePaths.push_back(path);
            } else {
                return Error{ "Invalid argument: {}", path.generic_string() };
            }
        }

        if(emblemJsonFilePaths.empty())
            return Error{ "Invalid arguments: No json emblem definitions provided" };
        if(sl2Path.empty())
            return Error{ "Invalid arguments: No sl2 file provided" };

        // Create backup save
        {
            auto backupFilePath = sl2Path;
            backupFilePath.concat(".bak");
            std::filesystem::copy_file(sl2Path, backupFilePath, std::filesystem::copy_options::overwrite_existing);
        }

        // Unpack sl2 binder
        PROPAGATE_IF_ERROR(unpackBinder(sl2Path));

        auto unpackedSl2DirPath = sl2Path.parent_path() / sl2Path.stem().concat("-sl2");
        SCOPE_EXIT {
            if(std::filesystem::exists(unpackedSl2DirPath))
                std::filesystem::remove_all(unpackedSl2DirPath);
        };

        // Load and decypt UserData007
        auto userData7FilePath = unpackedSl2DirPath / "USER_DATA007";
        UNWRAP_OR_PROPAGATE(userData7Data, readBinaryFile(userData7FilePath));

        crypto::decryptInplace(userData7Data.data() + AES_BLOCKLEN, userData7Data.size() - AES_BLOCKLEN,
                               userData7Data.data(), userDataKey);

        // Deserialize UserData007
        UNWRAP_OR_PROPAGATE(userData7, UserDataContainer::deserialize(userData7Data));

        // Insert EMBCs
        for(const auto& emblemJsonFilePath : emblemJsonFilePaths) {
            std::ifstream ifs(emblemJsonFilePath.c_str());
            if(!ifs.is_open())
                return Error{ "Failed to open json file {} for reading", emblemJsonFilePath.generic_string() };

            auto json = nlohmann::json::parse(ifs, nullptr, false);
            if(json.is_discarded())
                return Error{ "Failed in json::parse" };

            ifs.close();

            UNWRAP_OR_PROPAGATE(emblem, Emblem::fromJson(json));
            auto serializedEmblem = emblem.serialize();

            UNWRAP_OR_PROPAGATE(userDataFile, UserDataFile::create("EMBC", serializedEmblem));
            userData7.insertFile(std::move(userDataFile));
        }

        // Serialize UserData007
        userData7Data = userData7.serialize();
        crypto::encryptInplace(userData7Data.data() + AES_BLOCKLEN, userData7Data.size() - AES_BLOCKLEN,
                               userData7Data.data(), userDataKey);
        PROPAGATE_IF_ERROR(writeBinaryFile(userData7FilePath, userData7Data));

        // Pack sl2
        PROPAGATE_IF_ERROR(packBinder(unpackedSl2DirPath));

        return {};
    }

} // namespace

int main(int argc, char* argv[]) {
    std::vector<std::string_view> arguments;
    for(int i = 0; i < argc; ++i)
        arguments.emplace_back(argv[i], strlen(argv[i]));

    auto result = Main(arguments);
    if(!result) {
        std::cout << "EmblemImporter failed with error:\n\n";
        std::cout << "\t" << result.error().string() << "\n\n";
        std::cout << "Press any key to exit..." << std::endl;

        std::cin.ignore();
        return 1;
    }

    return 0;
}