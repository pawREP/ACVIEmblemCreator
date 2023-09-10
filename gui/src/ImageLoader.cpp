#include "ImageLoader.h"
#include "PlatformHelpers.h"
#include <Windows.h>
#include <cassert>
#include <combaseapi.h>
#include <wincodec.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

namespace {
    namespace detail {

        ComPtr<IWICImagingFactory2> wicImagingFactory = nullptr; // TODO: just parking this here for now.

        void createWicImagingFactory() {

            HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER,
                                          IID_PPV_ARGS(wicImagingFactory.ReleaseAndGetAddressOf()));

            assert(SUCCEEDED(hr));
        }

    } // namespace detail

    IWICImagingFactory2* getWic() {
        if(!detail::wicImagingFactory)
            detail::createWicImagingFactory();

        return detail::wicImagingFactory.Get();
    }

} // namespace

std::vector<uint8_t> loadWICImageAsRGBAPixelData(ID3D11Device* device, const std::wstring& filePath, ImageDimensions resizeDimensions) {

    ComPtr<IWICBitmapDecoder> decoder;
    ThrowIfFailed(getWic()->CreateDecoderFromFilename(filePath.c_str(), NULL, GENERIC_READ,
                                                      WICDecodeMetadataCacheOnDemand, decoder.ReleaseAndGetAddressOf()));

    ComPtr<IWICBitmapFrameDecode> frame;
    ThrowIfFailed(decoder->GetFrame(0, frame.ReleaseAndGetAddressOf()));

    IWICBitmapSource* source = frame.Get();

    ComPtr<IWICBitmapScaler> scaler;
    if(resizeDimensions) {
        ThrowIfFailed(getWic()->CreateBitmapScaler(scaler.ReleaseAndGetAddressOf()));
        ThrowIfFailed(scaler->Initialize(frame.Get(), resizeDimensions.width, resizeDimensions.height, WICBitmapInterpolationModeFant));
        source = scaler.Get();
    }

    ComPtr<IWICFormatConverter> converter;
    ThrowIfFailed(getWic()->CreateFormatConverter(converter.ReleaseAndGetAddressOf()));
    ThrowIfFailed(converter->Initialize(source, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.,
                                        WICBitmapPaletteTypeCustom));

    UINT width{}, height{};
    ThrowIfFailed(converter->GetSize(&width, &height));

    ComPtr<IWICComponentInfo> cinfo;
    ThrowIfFailed(getWic()->CreateComponentInfo(GUID_WICPixelFormat32bppRGBA, &cinfo));

    WICComponentType type;
    ThrowIfFailed(cinfo->GetComponentType(&type));

    ComPtr<IWICPixelFormatInfo> pfinfo;
    ThrowIfFailed(cinfo->QueryInterface(__uuidof(IWICPixelFormatInfo), reinterpret_cast<void**>(pfinfo.GetAddressOf())));

    UINT bpp;
    ThrowIfFailed(pfinfo->GetBitsPerPixel(&bpp));
    assert(bpp % 8 == 0);

    auto bytesPp       = bpp / 8;
    auto pixelCount    = width * height;
    auto rowPitch      = width * bytesPp;
    auto pixelDataSize = pixelCount * bytesPp;

    std::vector<uint8_t> pixelData(pixelDataSize);
    converter->CopyPixels(nullptr, rowPitch, pixelDataSize, pixelData.data());
    return pixelData;
}

ImageDimensions::operator bool() {
    return width && height;
}
