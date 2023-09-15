#include "EmblemImport.h"
#include "Crypto.h"
#include "Emblem.h"
#include "EmblemImporter.h"
#include "Error.h"
#include "ScopeExit.h"
#include "Serialization.h"
#include "UserData.h"
#include "json.h"
#include <Windows.h>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

using namespace libEmblem;

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
#ifdef _DEBUG
        auto waitResult = WaitForSingleObject(ShExecInfo.hProcess, 30000);
#else
        auto waitResult = WaitForSingleObject(ShExecInfo.hProcess, 3000); // Waiting 3 seconds should be plenty
#endif // _DEBUG

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
        auto result = openProcessSync(yabberPath.wstring().c_str(), binderPath.wstring().c_str());
        if(!result)
            return result.error();

        // Sanity check unpacking result
        auto unpackedSl2Dir = (binderPath.parent_path() / binderPath.stem()).concat("-sl2");
        if(!std::filesystem::exists(unpackedSl2Dir) || !std::filesystem::is_directory(unpackedSl2Dir))
            return Error{ "Unexpected Yabber unpacking result" };

        return {};
    }

    // Container class representing an unpacked BND4
    class UnpackedBinder {
        MAKE_NONCOPYABLE(UnpackedBinder);

    public:
        static ErrorOr<UnpackedBinder> open(const std::filesystem::path& path) {
            UnpackedBinder binder{ path };

            auto yabberPathOr = UnpackedBinder::yabberPath();
            PROPAGATE_IF_ERROR(yabberPathOr);

            auto unpackResult = unpackBinder(*yabberPathOr, path);
            if(!unpackResult)
                return unpackResult.error();

            binder.unpackedDirPath =
            path.parent_path() / path.stem().concat(std::format("-{}", path.extension().string().erase(0, 1)));

            return binder;
        }

        UnpackedBinder(UnpackedBinder&& other) noexcept : path_(other.path_) {
            std::swap(unpackedDirPath, other.unpackedDirPath);
        };

        UnpackedBinder& operator=(UnpackedBinder&& other) noexcept {
            this->path_ = other.path_;
            std::swap(unpackedDirPath, other.unpackedDirPath);
        };

        ~UnpackedBinder() {
            if(unpackPath().empty() || !std::filesystem::exists(unpackPath()))
                return;

            // Create backup save
            {
                auto binderPath     = path();
                auto backupFilePath = binderPath;
                backupFilePath.concat(".backup");
                std::filesystem::copy_file(binderPath, backupFilePath, std::filesystem::copy_options::overwrite_existing);
            }

            // Pack container
            {
                auto r = packBinder(yabberPath().value(), unpackedDirPath);
                assert(r);
            }

            // Delete backup created by Yabber to avoid confusion with our own backup
            {
                auto yabberBackupPath = path();
                yabberBackupPath.concat(".bak");
                if(std::filesystem::exists(yabberBackupPath) && std::filesystem::is_regular_file(yabberBackupPath))
                    std::filesystem::remove(yabberBackupPath);
            }

            if(std::filesystem::exists(unpackedDirPath))
                std::filesystem::remove_all(unpackedDirPath);
        }

        const std::filesystem::path& unpackPath() const {
            return unpackedDirPath;
        }

        const std::filesystem::path& path() const {
            return path_;
        }

    private:
        UnpackedBinder(const std::filesystem::path& path) : path_(path){};

        static ErrorOr<std::filesystem::path> yabberPath() {
            static std::filesystem::path path;
            if(path.empty()) {
                WCHAR baseModulePath[MAX_PATH];
                GetModuleFileName(NULL, baseModulePath, MAX_PATH);
                path = std::filesystem::path{ baseModulePath }.parent_path() / "Yabber\\Yabber.exe";
                if(!std::filesystem::exists(path))
                    return Error{ "Yabber.exe was not found in the current working directory." };
            }
            return path;
        }

        std::filesystem::path path_;
        std::filesystem::path unpackedDirPath;
    };

    ErrorOr<UserDataContainer> loadUserDataFromBinder(const UnpackedBinder& binder) {

        // Load and decypt UserData
        auto userDataFilePath = binder.unpackPath() / "USER_DATA007";
        UNWRAP_OR_PROPAGATE(userDataBytes, readBinaryFile(userDataFilePath));

        crypto::decryptInplace(userDataBytes.data() + crypto::AESBlockLen, userDataBytes.size() - crypto::AESBlockLen,
                               userDataBytes.data(), userDataKey);

        // Deserialize UserData
        std::basic_ispanstream ss{ std::span{ userDataBytes.data(), userDataBytes.size() } };
        BinaryStreamReader reader{ ss };
        UNWRAP_OR_PROPAGATE(userData, UserDataContainer::deserialize(reader));

        return userData;
    }

    ErrorOr<void> saveUserDataToBinder(const UnpackedBinder& binder, const UserDataContainer& userData) {
        // Serialize UserData
        auto userDataBytes = serializeToVector(userData);
        crypto::encryptInplace(userDataBytes.data() + crypto::AESBlockLen, userDataBytes.size() - crypto::AESBlockLen,
                               userDataBytes.data(), userDataKey);

        auto userDataFilePath = binder.unpackPath() / "USER_DATA007";
        auto writeResult      = writeBinaryFile(userDataFilePath, userDataBytes);
        PROPAGATE_IF_ERROR(writeResult);

        return {};
    }

} // namespace

ErrorOr<> importEmblems(const std::filesystem::path& sl2Path, const std::filesystem::path& jsonPath) {
    auto jsonPaths = std::vector{ jsonPath };
    return importEmblems(sl2Path, jsonPaths);
}

libEmblem::ErrorOr<> transferDownloadedEmblemsToUser2Tab(const std::filesystem::path& sl2Path) {
    if(sl2Path.empty())
        return Error{ "Invalid arguments: No sl2 file provided" };

    auto binderOr = UnpackedBinder::open(sl2Path);
    PROPAGATE_IF_ERROR(binderOr);

    auto userData = loadUserDataFromBinder(binderOr.value());
    PROPAGATE_IF_ERROR(userData);

    auto& extraFiles = userData->extraFiles();

    for(const auto& file : extraFiles) {
        if(file.type == "EMBC") {
            // StreamView view(file.data);
            std::basic_ispanstream<uint8_t> spanStream{ std::span{ file.data.data(), file.data.size() } };
            BinaryStreamReader reader{ spanStream };
            auto embc = EMBC::deserialize(reader);
            assert(embc);
            embc->category = 2;

            auto data = serializeToVector(*embc);
            UNWRAP_OR_PROPAGATE(userDataFile, UserDataFile::create("EMBC", data));
            userData->insertFile(std::move(userDataFile));
        }
    }

    auto writeResult = saveUserDataToBinder(*binderOr, *userData);
    PROPAGATE_IF_ERROR(writeResult);

    return {};
}

ErrorOr<> importEmblems(const std::filesystem::path& sl2Path, const std::vector<std::filesystem::path>& jsonPaths) {

    if(jsonPaths.empty())
        return Error{ "Invalid arguments: No json emblem definitions provided" };
    if(sl2Path.empty())
        return Error{ "Invalid arguments: No sl2 file provided" };

    auto binderOr = UnpackedBinder::open(sl2Path);
    PROPAGATE_IF_ERROR(binderOr);

    auto userData = loadUserDataFromBinder(binderOr.value());
    PROPAGATE_IF_ERROR(userData);

    // Insert EMBCs
    for(const auto& emblemJsonFilePath : jsonPaths) {
        std::ifstream ifs(emblemJsonFilePath.c_str());
        if(!ifs.is_open())
            return Error{ "Failed to open json file {} for reading", emblemJsonFilePath.generic_string() };

        auto json = nlohmann::json::parse(ifs, nullptr, false);
        if(json.is_discarded())
            return Error{ "Failed in json::parse" };

        ifs.close();

        UNWRAP_OR_PROPAGATE(emblems, GeometrizeImporter::fromJson(json));
        for(auto& emblem : emblems) {

            EMBC embc;
            embc.category = 1;
            embc.ugcId    = L"";
            embc.dateTime = DateTime::fromCurrentTime();
            embc.image    = std::move(emblem);

            auto data = serializeToVector(embc);
            UNWRAP_OR_PROPAGATE(userDataFile, UserDataFile::create("EMBC", data));
            userData->insertFile(std::move(userDataFile));
        }
    }

    auto writeResult = saveUserDataToBinder(*binderOr, *userData);
    PROPAGATE_IF_ERROR(writeResult);

    return {};
}
