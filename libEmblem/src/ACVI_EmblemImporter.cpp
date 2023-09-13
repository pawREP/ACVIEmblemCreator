#include "Crypto.h"
#include "Emblem.h"
#include "EmblemImporter.h"
#include "Error.h"
#include "ScopeExit.h"
#include "Serialization.h"
#include "UserData.h"
#include "json.h"
#include "libEmblem.h"
#include <Windows.h>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

    constexpr uint8_t userDataKey[crypto::AESBlockLen] = { 0xB1, 0x56, 0x87, 0x9F, 0x13, 0x48, 0x97, 0x98,
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

        SCOPE_EXIT {
            CloseHandle(ShExecInfo.hProcess);
        };

        assert(ShExecInfo.hProcess);
        auto waitResult = WaitForSingleObject(ShExecInfo.hProcess, 3000); // Waiting 3 seconds should be plenty

        switch(waitResult) {
        case WAIT_OBJECT_0:
            break;
        default:
            TerminateProcess(ShExecInfo.hProcess, 1);
            return Error{ "Save file (un)packing failed with wait result {}. This is likely caused by a lack of write "
                          "permissions in the "
                          "save file folder or because the save file itself is not overwritable. Try copying the sl2 "
                          "file to another location before trying again.",
                          waitResult };
        }

        DWORD exitCode{};
        GetExitCodeProcess(ShExecInfo.hProcess, &exitCode);
        if(exitCode != 0)
            return Error{ "ShellExecuteEx target process exited unexpectedly with code: {}", exitCode };

        return {};
    }

    ErrorOr<void> packBinder(const std::filesystem::path& yabberPath, const std::filesystem::path& binderPath) {
        return openProcessSync(yabberPath.wstring().c_str(), binderPath.wstring().c_str());
    }

    ErrorOr<void> unpackBinder(const std::filesystem::path& yabberPath, const std::filesystem::path& binderPath) {
        PROPAGATE_IF_ERROR(openProcessSync(yabberPath.wstring().c_str(), binderPath.wstring().c_str()));

        // Sanity check unpacking result
        auto unpackedSl2Dir = (binderPath.parent_path() / binderPath.stem()).concat("-sl2");
        if(!std::filesystem::exists(unpackedSl2Dir) || !std::filesystem::is_directory(unpackedSl2Dir))
            return Error{ "Unexpected Yabber unpacking result" };

        return {};
    }

    ErrorOr<void> Main(const std::vector<std::wstring_view>& args) {

        // Validate arguments
        if(args.size() < 2)
            return Error{ "Insufficient number of arguments.\n"
                          "At least one .json and one .sl2 required" };

        std::vector<std::filesystem::path> emblemJsonFilePaths{};
        std::filesystem::path sl2Path{};

        for(int i = 0; i < args.size(); ++i) {
            std::filesystem::path path{ args[i] };
            if(path.empty() || !std::filesystem::is_regular_file(path) || !path.has_extension())
                continue;

            if(path.extension() == ".sl2") {
                sl2Path = path;
            } else if(path.extension() == ".json") {
                emblemJsonFilePaths.push_back(path);
            }
        }

        if(emblemJsonFilePaths.empty())
            return Error{ "Invalid arguments: No json emblem definitions provided" };
        if(sl2Path.empty())
            return Error{ "Invalid arguments: No sl2 file provided" };

        WCHAR baseModulePath[MAX_PATH];
        GetModuleFileName(NULL, baseModulePath, MAX_PATH);
        std::filesystem::path yabberPath = std::filesystem::path{ baseModulePath }.parent_path() / "Yabber\\Yabber.exe";
        if(!std::filesystem::exists(yabberPath))
            return Error{ "Yabber.exe was not found in the current working directory." };

        // Create backup save
        {
            auto backupFilePath = sl2Path;
            backupFilePath.concat(".backup");
            std::filesystem::copy_file(sl2Path, backupFilePath, std::filesystem::copy_options::overwrite_existing);
        }

        // Unpack sl2 binder
        PROPAGATE_IF_ERROR(unpackBinder(yabberPath, sl2Path));

        auto unpackedSl2DirPath = sl2Path.parent_path() / sl2Path.stem().concat("-sl2");
        SCOPE_EXIT {
            if(std::filesystem::exists(unpackedSl2DirPath))
                std::filesystem::remove_all(unpackedSl2DirPath);
        };

        // Load and decypt UserData
        auto userDataFilePath = unpackedSl2DirPath / "USER_DATA007";
        UNWRAP_OR_PROPAGATE(userDataBytes, readBinaryFile(userDataFilePath));

        crypto::decryptInplace(userDataBytes.data() + crypto::AESBlockLen, userDataBytes.size() - crypto::AESBlockLen,
                               userDataBytes.data(), userDataKey);

        // Deserialize UserData
        UNWRAP_OR_PROPAGATE(userData, UserDataContainer::deserialize(userDataBytes));

        // Insert EMBCs
        for(const auto& emblemJsonFilePath : emblemJsonFilePaths) {
            std::ifstream ifs(emblemJsonFilePath.c_str());
            if(!ifs.is_open())
                return Error{ "Failed to open json file {} for reading", emblemJsonFilePath.generic_string() };

            auto json = nlohmann::json::parse(ifs, nullptr, false);
            if(json.is_discarded())
                return Error{ "Failed in json::parse" };

            ifs.close();

            UNWRAP_OR_PROPAGATE(emblems, GeometrizeImporter::fromJson(json));
            for(const auto& emblem : emblems) {

                EMBC embc;
                embc.category = 1;
                embc.ugcId    = L"";
                embc.dateTime = DateTime::fromCurrentTime();
                embc.image    = std::move(emblem);

                auto data = serializeToVector(embc);
                UNWRAP_OR_PROPAGATE(userDataFile, UserDataFile::create("EMBC", data));
                userData.insertFile(std::move(userDataFile));
            }
        }


        // Serialize UserData
        userDataBytes = userData.serialize();
        crypto::encryptInplace(userDataBytes.data() + crypto::AESBlockLen, userDataBytes.size() - crypto::AESBlockLen,
                               userDataBytes.data(), userDataKey);
        PROPAGATE_IF_ERROR(writeBinaryFile(userDataFilePath, userDataBytes));

        // Pack sl2
        PROPAGATE_IF_ERROR(packBinder(yabberPath, unpackedSl2DirPath));

        // Delete backup created by Yabber to avoid confusion with our own backup
        {
            auto yabberBackupPath = sl2Path;
            yabberBackupPath.concat(".bak");
            if(std::filesystem::exists(yabberBackupPath) && std::filesystem::is_regular_file(yabberBackupPath))
                std::filesystem::remove(yabberBackupPath);
        }

        return {};
    }


#ifdef LIB_EMBLEM
    std::string g_lastError;
#endif

} // namespace

#ifdef LIB_EMBLEM
char* embGetLastError() {
    auto err = new char[g_lastError.size() + 1]{};
    memcpy(err, g_lastError.data(), g_lastError.size());
    return err;
}
#endif

int embImportEmblems(int argc, const wchar_t* argv[]) {
    std::vector<std::wstring_view> arguments;
    for(int i = 0; i < argc; ++i)
        arguments.emplace_back(argv[i], wcslen(argv[i]));

    auto result = Main(arguments);

    if(!result) {
#ifdef LIB_EMBLEM
        g_lastError = result.error().string();
        return 1;
#else
        std::cout << "EmblemImporter failed with error:\n\n";
        std::cout << "\t" << result.error().string() << "\n\n";
        std::cout << "Press any key to exit..." << std::endl;

        std::cin.ignore();
#endif
        return 1;
    }
    return 0;
}

int wmain(int argc, wchar_t* argv[]) {
    return embImportEmblems(argc, const_cast<const wchar_t**>(argv));
}