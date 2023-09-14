#include "AsyncTask.h"
#include "BatchRenderer.h"
#include "ChromaKey.h"
#include "EmblemImport.h"
#include "Generator.h"
#include "ImageLoader.h"
#include "PlatformHelpers.h"
#include "RenderPrim.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include "imgui.h"
#include "json.h"
#include <Windows.h>
#include <cassert>
#include <d3d11.h>
#include <filesystem>
#include <fstream>
#include <shobjidl.h>
#include <wrl.h>

#pragma comment(lib, "d3d11")

using Microsoft::WRL::ComPtr;

using namespace libEmblem;

namespace {

    // Type defs
    using VertexT   = DirectX::VertexPositionColor;
    using Prim      = RenderPrim<VertexT>;
    using Ellipse   = EllipseRenderPrim<DirectX::VertexPositionColor, 6>;
    using Rectangle = RectangleRenderPrim<DirectX::VertexPositionColor>;

    // D3D state
    ComPtr<ID3D11Device> device                         = nullptr;
    ComPtr<ID3D11DeviceContext> context                 = nullptr;
    ComPtr<IDXGISwapChain> swapChain                    = nullptr;
    ComPtr<ID3D11RenderTargetView> mainRenderTargetView = nullptr;

    UINT g_ResizeWidth  = 0;
    UINT g_ResizeHeight = 0;

    std::unique_ptr<Geom::BatchRenderer> batchRenderer;

    uint32_t windowWidth  = 1024;
    uint32_t windowHeight = 512;

    struct GUIContext {
        bool showWindow                = true;
        char imagePath[MAX_PATH]       = {};
        char jsonPath[MAX_PATH]        = {};
        char sl2Path[MAX_PATH]         = {};
        bool chromaKeyEnabled          = false;
        int maxShapesCount             = 0;
        int numberOfShapesToSkip       = 0;
        bool drawCustomBackground      = false;
        float customBackgroundColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        bool drawFrame                 = true;
        std::string statusText         = {};
        ShapeGeneratorOptions generatorOptions{};
    } guiContext;

    AsyncTask<ErrorOr<>(void)> exportTask;

    std::vector<Prim> primitives;
    std::vector<int32_t> visiblePrimitives;
    nlohmann::json currentJson{};
    std::vector<uint8_t> imageData;
    ChromaKey chromaKey{};

    std::unique_ptr<ShapeGenerator> generator = nullptr;

    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    std::filesystem::path openFile();
    std::vector<Prim> loadPrimitivesfromJson(const nlohmann::json& json);
    nlohmann::json loadJsonFromFile(const wchar_t* path);
    nlohmann::json loadJsonFromString(const std::string& jsonString);
    void exportEmblemToGame();
    void setStatus(const char* status);


} // namespace

int main(int arc, char* argv[]) {

    ThrowIfFailed(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));

    WNDCLASSEXW wc = {
        sizeof(wc), CS_CLASSDC,         WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr,
        nullptr,    L"EmblemCreatorWC", nullptr
    };

    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Armored Core VI Emblem Creator", WS_OVERLAPPEDWINDOW, 100, 100,
                                windowWidth, windowHeight, nullptr, nullptr, wc.hInstance, nullptr);
    assert(hwnd);

    // Initialize Direct3D
    if(!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    batchRenderer = Geom::BatchRenderer::create(device.Get(), Geom::AAMode::None);

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device.Get(), context.Get());

    // Main loop
    bool done = false;
    while(!done) {

        MSG msg;
        while(::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if(msg.message == WM_QUIT)
                done = true;
        }
        if(done)
            break;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if(g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            swapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            windowWidth   = g_ResizeWidth;
            windowHeight  = g_ResizeHeight;
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Set viewport
        DirectX::SimpleMath::Viewport viewPort(0.0f, 0.0f, windowWidth / 2.f, windowHeight);
        context->RSSetViewports(1, viewPort.Get11());

        // Clear render target
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        context->ClearRenderTargetView(mainRenderTargetView.Get(), clearColor);
        context->OMSetRenderTargets(1, mainRenderTargetView.GetAddressOf(), NULL);

        ImGuiWindowFlags flags{};
        flags |= ImGuiWindowFlags_NoTitleBar;
        flags |= ImGuiWindowFlags_NoResize;
        flags |= ImGuiWindowFlags_NoMove;
        flags |= ImGuiWindowFlags_NoCollapse;
        flags |= ImGuiWindowFlags_NoBackground;
        // flags |= ImGuiWindowFlags_NoScrollbar;
        flags |= ImGuiWindowFlags_NoDecoration;

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_None, ImVec2(0.0f, 0.5f));

        ImGuiIO& io        = ImGui::GetIO();
        float windowWidth  = io.DisplaySize.x;
        float windowHeight = io.DisplaySize.y;
        ImVec2 windowSize  = ImVec2(windowWidth / 2, windowHeight);
        ImGui::SetNextWindowSize(windowSize);

        ImGui::Begin("Armored Core VI Emblem Creator", &guiContext.showWindow, flags);

        if(ImGui::Button("Load Json ")) {
            generator = nullptr; // Cancel generation in case any is ongoing
            imageData.clear();
            memset(guiContext.imagePath, 0, sizeof(guiContext.imagePath));

            auto jsonPath = openFile();

            if(!jsonPath.empty() || std::filesystem::is_regular_file(jsonPath) || jsonPath.has_extension() ||
               jsonPath.extension() == ".json") {
                currentJson = loadJsonFromFile(jsonPath.wstring().c_str());
                if(!currentJson.empty()) {
                    primitives                = loadPrimitivesfromJson(currentJson);
                    guiContext.maxShapesCount = primitives.size();
                    strcpy_s(guiContext.jsonPath, jsonPath.generic_string().c_str());
                } else {
                    setStatus("Failed to parse json");
                }
            }
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::BeginDisabled();
        ImGui::InputText("a", guiContext.jsonPath, MAX_PATH);
        ImGui::EndDisabled();
        ImGui::PopItemWidth();

        if(ImGui::Button("Load Image")) {
            memset(guiContext.jsonPath, 0, sizeof(guiContext.jsonPath));

            auto imagePath = openFile();

            if(!imagePath.empty() || std::filesystem::is_regular_file(imagePath) || imagePath.has_extension() ||
               (imagePath.extension() == ".png" || imagePath.extension() == ".jpg")) {
                std::vector<uint8_t> pixelData{};
                try {
                    imageData = loadWICImageAsRGBAPixelData(device.Get(), imagePath.wstring().c_str(), { 256, 256 });
                    strcpy_s(guiContext.imagePath, imagePath.generic_string().c_str());
                } catch(const bad_hr_exception& e) {
                    setStatus("Failed to load image data");
                    OutputDebugStringA(e.what());
                }
            }
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::BeginDisabled();
        ImGui::InputText("a", guiContext.imagePath, MAX_PATH);
        ImGui::EndDisabled();
        ImGui::PopItemWidth();

        if(imageData.size()) {
            ImGui::Indent();

            auto& opt = guiContext.generatorOptions;
            ImGui::BeginTable("tbl", 2);
            ImGui::TableNextColumn();
            ImGui::CheckboxFlags("Rectangle", (uint32_t*)&opt.shapes, std::to_underlying(GeneratorShapes::Rectangle));
            ImGui::CheckboxFlags("Rotated Rectangle", (uint32_t*)&opt.shapes, std::to_underlying(GeneratorShapes::RotatedRectangle));
            ImGui::CheckboxFlags("Ellipse", (uint32_t*)&opt.shapes, std::to_underlying(GeneratorShapes::Ellipse));
            ImGui::TableNextColumn();
            ImGui::CheckboxFlags("Rotated Ellipse", (uint32_t*)&opt.shapes, std::to_underlying(GeneratorShapes::RotatedEllipse));
            ImGui::CheckboxFlags("Circle", (uint32_t*)&opt.shapes, std::to_underlying(GeneratorShapes::Circle));
            ImGui::EndTable();

            ImGui::DragInt("Candidates per Shape", &opt.candidateCount, 1, 1, 300);
            ImGui::DragInt("Mutation per Shape  ", &opt.mutationCount, 1, 1, 300);
            ImGui::DragInt("Shape Count Limit   ", &opt.maxShapeCount, 1, 1, 1024);
            ImGui::DragInt("Shape Alpha         ", &opt.shapeAlpha, 1, 1, 255);

            if(!generator) {
                if(ImGui::Button("Generate!")) {
                    generator = std::make_unique<ShapeGenerator>(256, 256, imageData, guiContext.generatorOptions);
                    generator->run();
                }
            } else {
                if(ImGui::Button("Cancel Generation")) {
                    generator->cancel();
                }

                auto generatorDone = generator->done(); // has to checked before getting state or we might miss some at the end

                if(generator->hasNewState()) {
                    auto jsonString = generator->getJson();
                    currentJson     = loadJsonFromString(jsonString);
                    if(!currentJson.empty()) {
                        primitives                = loadPrimitivesfromJson(currentJson);
                        guiContext.maxShapesCount = primitives.size();
                    } else {
                        setStatus("Failed to parse json");
                        generator = nullptr;
                    }
                }
                if(generatorDone) {
                    generator = nullptr;
                }
            }

            ImGui::Unindent();
        }


        ImGui::Separator();

        if(ImGui::Button("Open .sl2 ")) {
            auto path = openFile();
            strcpy_s(guiContext.sl2Path, path.generic_string().c_str());
        }
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);
        ImGui::BeginDisabled();
        ImGui::InputText("b", guiContext.sl2Path, MAX_PATH);
        ImGui::EndDisabled();
        ImGui::PopItemWidth();

        ImGui::Checkbox("Enable Color Key", &guiContext.chromaKeyEnabled);
        if(guiContext.chromaKeyEnabled) {
            ImGui::Indent();
            ImGui::SliderFloat("Distance Threshold", &chromaKey.distanceThreshold, 0.0f, 1.0f);
            ImGui::DragFloat3("Component Thresholds", (float*)&chromaKey.elementThresholds, 0.002f, 0.f, 1.f);
            ImGui::ColorEdit3("Key Color", (float*)&chromaKey.key);
            ImGui::Unindent();
        }

        if(!guiContext.maxShapesCount)
            guiContext.maxShapesCount = primitives.size();
        ImGui::SliderInt("Shape Limit", &guiContext.maxShapesCount, 1, primitives.size(), "%d", ImGuiSliderFlags_AlwaysClamp);

        ImGui::InputInt("Skip Shapes", &guiContext.numberOfShapesToSkip, 1, primitives.size() / 20);

        batchRenderer->begin(context.Get());

        ImGui::Checkbox("Custom background (for reference only)", &guiContext.drawCustomBackground);
        if(guiContext.drawCustomBackground) {
            ImGui::Indent();

            ImGui::ColorEdit3("Background color", guiContext.customBackgroundColor);

            float sideLength = 2.0f;
            DirectX::VertexPositionColor vertices[] = { { DirectX::XMFLOAT3(-sideLength / 2.0f, sideLength / 2.0f, 0.0f),
                                                          DirectX::XMFLOAT4(guiContext.customBackgroundColor) },
                                                        { DirectX::XMFLOAT3(sideLength / 2.0f, sideLength / 2.0f, 0.0f),
                                                          DirectX::XMFLOAT4(guiContext.customBackgroundColor) },
                                                        { DirectX::XMFLOAT3(sideLength / 2.0f, -sideLength / 2.0f, 0.0f),
                                                          DirectX::XMFLOAT4(guiContext.customBackgroundColor) },
                                                        { DirectX::XMFLOAT3(-sideLength / 2.0f, -sideLength / 2.0f, 0.0f),
                                                          DirectX::XMFLOAT4(guiContext.customBackgroundColor) } };
            uint16_t indexBuffer[] = {
                0,
                1,
                3,
                2,
            };

            batchRenderer->drawIndexed(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, indexBuffer, 4, vertices, 4);
            ImGui::Unindent();
        }

        // Draw Emblem
        visiblePrimitives.clear();
        int skipCount = 0;
        for(int primIdx = 0; primIdx < primitives.size(); ++primIdx) {
            const auto& prim = primitives[primIdx];

            if(visiblePrimitives.size() >= guiContext.maxShapesCount)
                break;

            if(skipCount < guiContext.numberOfShapesToSkip) {
                skipCount++;
                continue;
            }

            if(guiContext.chromaKeyEnabled) {
                assert(prim.vertices.size());
                auto& color = prim.vertices[0].color; // All verts in a shape are the same color
                if(chromaKey.match(color))
                    continue;
            }

            batchRenderer->draw(prim.topology, prim.vertices.data(), prim.vertices.size());
            visiblePrimitives.push_back(primIdx);
        }

        // Draw Frame
        ImGui::Checkbox("Draw Frame", &guiContext.drawFrame);
        if(guiContext.drawFrame) {
            static float sideLength                        = 1.5f; // inner
            static float sidelength2                       = 2.0f; // outter
            static DirectX::VertexPositionColor vertices[] = {
                { DirectX::XMFLOAT3(-sideLength / 2.0f, sideLength / 2.0f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
                { DirectX::XMFLOAT3(sideLength / 2.0f, sideLength / 2.0f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
                { DirectX::XMFLOAT3(sideLength / 2.0f, -sideLength / 2.0f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
                { DirectX::XMFLOAT3(-sideLength / 2.0f, -sideLength / 2.0f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
                { DirectX::XMFLOAT3(-sidelength2 / 2.0f, sidelength2 / 2.0f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
                { DirectX::XMFLOAT3(sidelength2 / 2.0f, sidelength2 / 2.0f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
                { DirectX::XMFLOAT3(sidelength2 / 2.0f, -sidelength2 / 2.0f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
                { DirectX::XMFLOAT3(-sidelength2 / 2.0f, -sidelength2 / 2.0f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) }
            };
            static uint16_t indexBuffer[] = {
                0, 4, 3, 7, //
                0, 4, 1, 5, //
                5, 1, 6, 2, //
                6, 2, 7, 3,
            };
            batchRenderer->drawIndexed(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, indexBuffer, _countof(indexBuffer),
                                       vertices, _countof(vertices));
        }
        batchRenderer->end();

        auto drawnShapesCount = visiblePrimitives.size();
        ImGui::Text("Drawn Shape Count: %d", drawnShapesCount);
        auto emblemCount = drawnShapesCount / 128 + (drawnShapesCount % 128 ? 1 : 0);
        if(emblemCount > 1) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "(imported as %d emblems!)", emblemCount);
        }

        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

        ImGui::PushStyleColor(ImGuiCol_Button, { 0.81f, 0.35f, 0.32f, 1.f });
        if(ImGui::Button("Export to Game"))
            exportEmblemToGame();
        ImGui::PopStyleColor();

        if(exportTask.ready()) {
            auto result = exportTask.get();
            if(!result) {
                setStatus(result.error().string().c_str());
            } else {
                setStatus("Success!");
            }
        }

        ImGui::TextWrapped("%s", guiContext.statusText.c_str());

        ImGui::End();

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        swapChain->Present(1, 0);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {

    bool CreateDeviceD3D(HWND hWnd) {

        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount                        = 2;
        sd.BufferDesc.Width                   = 0;
        sd.BufferDesc.Height                  = 0;
        sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator   = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow                       = hWnd;
        sd.SampleDesc.Count                   = 1;
        sd.SampleDesc.Quality                 = 0;
        sd.Windowed                           = TRUE;
        sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

        UINT createDeviceFlags = 0;
#ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG

        D3D_FEATURE_LEVEL featureLevel;
        const D3D_FEATURE_LEVEL featureLevelArray[2] = {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_0,
        };
        // TODO: Bad error handling here  :/
        ThrowIfFailed(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
                                                    featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
                                                    swapChain.ReleaseAndGetAddressOf(), device.ReleaseAndGetAddressOf(),
                                                    &featureLevel, context.ReleaseAndGetAddressOf()));

        CreateRenderTarget();
        return true;
    }

    void CleanupDeviceD3D() {
        CleanupRenderTarget();
        swapChain = nullptr;
        context   = nullptr;
        device    = nullptr;
    }

    void CreateRenderTarget() {
        ComPtr<ID3D11Texture2D> backBuffer;
        swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.ReleaseAndGetAddressOf()));
        device->CreateRenderTargetView(backBuffer.Get(), nullptr, mainRenderTargetView.GetAddressOf());
    }

    void CleanupRenderTarget() {
        mainRenderTargetView = nullptr;
    }

    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if(ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        switch(msg) {
        case WM_SIZING: { // TODO: Should take the drag edge into account
            RECT* rect   = (RECT*)lParam;
            auto w       = rect->right - rect->left;
            auto h       = w / 2;
            rect->bottom = rect->top + h;

        } break;
        case WM_SIZE:
            if(wParam == SIZE_MINIMIZED)
                return 0;
            g_ResizeWidth  = (UINT)LOWORD(lParam); // Queue resize
            g_ResizeHeight = (UINT)HIWORD(lParam);
            return 0;
        case WM_SYSCOMMAND:
            if((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
                return 0;
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
        }
        return ::DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    std::vector<Prim> loadPrimitivesfromJson(const nlohmann::json& json) {
        auto shapeDescs = fromGeomtrizeJson(currentJson);

        std::vector<Prim> primitives;
        for(int i = 0; i < shapeDescs.size(); ++i) {
            // TODO: Make visit
            if(std::holds_alternative<EllipseDesc>(shapeDescs[i])) {
                primitives.push_back(Ellipse::create(std::get<EllipseDesc>(shapeDescs[i])));
            } else if(std::holds_alternative<RectangleDesc>(shapeDescs[i])) {
                primitives.push_back(Rectangle::create(std::get<RectangleDesc>(shapeDescs[i])));
            }
        }
        return primitives;
    }

    nlohmann::json loadJsonFromFile(const wchar_t* path) {
        std::ifstream ifs(path);
        assert(ifs.is_open());
        try {
            return nlohmann::json::parse(ifs);
        } catch(nlohmann::json::parse_error& ex) {
        }
        return {};
    }

    nlohmann::json loadJsonFromString(const std::string& jsonString) {
        try {
            return nlohmann::json::parse(jsonString);
        } catch(nlohmann::json::parse_error& ex) {
            OutputDebugStringA(ex.what());
        }
        return {};
    }


    std::filesystem::path openFile() {
        Microsoft::WRL::ComPtr<IFileOpenDialog> pFileOpen;
        CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, (void**)pFileOpen.ReleaseAndGetAddressOf());

        pFileOpen->Show(NULL);

        Microsoft::WRL::ComPtr<IShellItem> pItem;
        pFileOpen->GetResult(&pItem);

        if(!pItem)
            return {};

        PWSTR pszFilePath;
        pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

        return std::filesystem::path{ pszFilePath };
    }

    nlohmann::json buildJsonFromVisiblePrimitives() {
        nlohmann::json json = nlohmann::json::array();

        auto bbLayerCulled = std::find(visiblePrimitives.begin(), visiblePrimitives.end(), 0) == visiblePrimitives.end();
        if(bbLayerCulled) { // TODO: Injecting the bb like this is a bit ugly, we can do it better by taking it into account during render time.
            json.push_back(currentJson[0]);
            json[0]["color"][3] = 0;

            for(int i = 0; i < visiblePrimitives.size() - 1; ++i) {
                json.push_back(currentJson[visiblePrimitives[i]]);
            }
        } else {
            for(const auto& i : visiblePrimitives) {
                json.push_back(currentJson[i]);
            }
        }

        return json;
    }

    void exportEmblemToGame() {
        if(exportTask.valid()) // Already exporting
            return;

        if(currentJson.empty()) {
            setStatus("No geometry data loaded or generated");
            return;
        }

        std::filesystem::path sl2Path{ guiContext.sl2Path };
        if(sl2Path.empty() || !std::filesystem::is_regular_file(sl2Path) || !sl2Path.has_extension() || sl2Path.extension() != ".sl2") {
            setStatus("Invalid .sl2 path");
            return;
        }

        // Build and save final json to temp folder
        auto finalJson = buildJsonFromVisiblePrimitives();
        auto jsonPath  = std::filesystem::temp_directory_path() / "emblem.json";

        std::ofstream ofs(jsonPath);
        assert(ofs.is_open());
        ofs << finalJson;
        ofs.close();

        auto emblemExportProc = [sl2Path, jsonPath]() -> ErrorOr<> {
            auto ret = importEmblems(sl2Path, jsonPath);
            if(std::filesystem::exists(jsonPath))
                std::filesystem::remove(jsonPath);
            return ret;
        };

        exportTask = { std::move(emblemExportProc) };
        exportTask.run();

        setStatus("Exporting...");
    }

    void setStatus(const char* status) {
        guiContext.statusText = status;
    }

} // namespace
