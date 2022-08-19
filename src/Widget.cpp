#include "Widget.h"
#include <dwmapi.h>
#include <shellapi.h>
#include "App.h"
#include "shaders/hlsl.h"
#include <d3dcompiler.h>
#include <iostream>
#include <cstdint>
#include <cstring>
#include "BasicBitmapLoader.h"
#include "res.h"
#include <swal/reg.h>
#include <filesystem>

namespace {

constexpr float Pi = 3.14159265f;

struct VertexIn {
    float posUV[4];
    alignas(16)
    std::uint32_t index;
} buffer[] = {
    { {-.125f,  1.f, .9375f, 0.f}, 3 },
    { { .125f,  1.f, 1.f,    0.f}, 3 },
    { {-.125f, -1.f, .9375f, 1.f}, 3 },
    { { .125f, -1.f, 1.f,    1.f}, 3 },
    { {-.125f,  1.f, .875f,  0.f}, 2 },
    { { .125f,  1.f, .9375f, 0.f}, 2 },
    { {-.125f, -1.f, .875f,  1.f}, 2 },
    { { .125f, -1.f, .9375f, 1.f}, 2 },
    { {-.125f,  1.f, .8125f, 0.f}, 1 },
    { { .125f,  1.f, .875f,  0.f}, 1 },
    { {-.125f, -1.f, .8125f, 1.f}, 1 },
    { { .125f, -1.f, .875f,  1.f}, 1 },
    { {-1.f,    3.f, 0.f,   -1.f}, 0 },
    { { 3.f,   -1.f, 1.f,    1.f}, 0 },
    { {-1.f,   -1.f, 0.f,    1.f}, 0 },
};

unsigned int elements[] = {
     0,  1,  2,  3, 0 - 1U,
     4,  5,  6,  7, 0 - 1U,
     8,  9, 10, 11, 0 - 1U,
    12, 13, 14, 0 - 1U,
};

struct ConstantBuffer {
    alignas(16)
    float values[4];
};

clock_widget::Widget* widget;

const TCHAR runKeyName[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
const TCHAR appName[] = TEXT("Clock");
const TCHAR appRegKey[] = TEXT("Software\\Clock");
const TCHAR posXValueName[] = TEXT("PosX");
const TCHAR posYValueName[] = TEXT("PosY");

}

namespace clock_widget {

const std::unordered_map<UINT, LRESULT (Widget::*)(WPARAM, LPARAM)> Widget::message_map {
    std::pair{ WM_PAINT, &Widget::OnPaint },
    std::pair{ WM_TIMER, &Widget::OnTimer },
    std::pair{ WM_CLOSE, &Widget::OnClose },
    std::pair{ WM_SIZE, &Widget::OnResize },
    std::pair{ WM_NCHITTEST, &Widget::OnHitTest },
    std::pair{ NotifyIconMsg, &Widget::OnNotifyIcon },
    std::pair{ WM_COMMAND, &Widget::OnCommand },
    std::pair{ WM_EXITSIZEMOVE, &Widget::OnExitSizeMove },
};

Widget::Widget() :
    window(GetClass(), App::GetHInstance(), this),
    alphaValue(255),
    alphaStep(FadeSpeed),
    iconMenu(App::GetHInstance(), IDM_CLOCK_ICON_MENU)
{
    SetupRender();
    SetupWindow();
    SetupMenu();
    swal::winapi_call(SetTimer(window, TimerId, 1000, nullptr));
    widget = this;
    mouseHook = swal::winapi_call(SetWindowsHookEx(WH_MOUSE_LL, &Widget::MouseHook, NULL, 0));
    NOTIFYICONDATA nid;
    nid.cbSize = sizeof(nid);
    nid.hWnd = window;
    nid.uID = NotifyIconId;
    nid.uFlags = NIF_MESSAGE | NIF_TIP | NIF_ICON;
    nid.hIcon = LoadIcon(App::GetHInstance(), MAKEINTRESOURCE(IDI_CLOCK_ICON));
    nid.uCallbackMessage = NotifyIconMsg;
    swal::winapi_call(
        LoadString(App::GetHInstance(), IDS_CLOCK_TIP, nid.szTip, std::size(nid.szTip))
    );
    swal::winapi_call(Shell_NotifyIcon(NIM_ADD, &nid));
    nid.uVersion = NOTIFYICON_VERSION_4;
    swal::winapi_call(Shell_NotifyIcon(NIM_SETVERSION, &nid));
}

Widget::~Widget()
{
    NOTIFYICONDATA nid;
    nid.cbSize = sizeof(nid);
    nid.hWnd = window;
    nid.uID = NotifyIconId;
    nid.uFlags = 0;
    Shell_NotifyIcon(NIM_DELETE, &nid);
    UnhookWindowsHookEx(mouseHook);
    KillTimer(window, TimerId);
}

LRESULT Widget::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto it = message_map.find(message);
    if (it == message_map.end()) {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return (this->*(it->second))(wParam, lParam);
}

LRESULT Widget::OnPaint(WPARAM, LPARAM)
{
	ConstantBuffer hands = {};
	{
		SYSTEMTIME time/* = {
			.wHour = 23,
			.wMinute = 5,
			.wSecond = 27,
		}*/;
		GetLocalTime(&time);
		hands.values[3] = time.wSecond;
		hands.values[2] = time.wMinute * 60 + time.wSecond;
		hands.values[1] = time.wHour * 3600 + hands.values[2];
		hands.values[3] = hands.values[3] * Pi / 30.f;
		hands.values[2] = hands.values[2] * Pi / 1800.f;
		hands.values[1] = hands.values[1] * Pi / 21600.f;
	}

	D3D11_MAPPED_SUBRESOURCE subres = {};
	context->Map(cBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subres);
	std::memcpy(subres.pData, &hands, sizeof(hands));
	context->Unmap(cBuffer.Get(), 0);

	UINT stride[] = {sizeof(VertexIn), sizeof(VertexIn)};
	UINT offset[] = {0, offsetof(VertexIn, index)};
	ID3D11Buffer *buffers[] = {vertBuffer.Get(), vertBuffer.Get()};

    float color[4] = { .0f, .0f, .0f, .0f };
    context->ClearRenderTargetView(rtView.Get(), color);

	context->OMSetRenderTargets(1, rtView.GetAddressOf(), nullptr);
	context->OMSetBlendState(blendState.Get(), nullptr, 0xffffffff);
	context->IASetVertexBuffers(0, std::size(buffers), buffers, stride, offset);
	context->IASetIndexBuffer(idxBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetInputLayout(inLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	context->VSSetShader(vertShader.Get(), nullptr, 0);
	context->VSSetConstantBuffers(0, 1, cBuffer.GetAddressOf());
	context->PSSetShader(fragShader.Get(), nullptr, 0);
	context->PSSetShaderResources(0, 1, shaderTexture.GetAddressOf());
	context->DrawIndexed(std::size(elements), 0, 0);

    swal::com_call(swapChain->Present(0, 0));
    window.ValidateRect();
    return 0;
}

LRESULT Widget::OnTimer(WPARAM wParam, LPARAM)
{
    switch (UINT_PTR(wParam)) {
        case AnimTimerId:
            alphaValue = std::clamp(alphaValue + alphaStep, 0, 255);
            swal::winapi_call(SetLayeredWindowAttributes(window, 0, alphaValue, LWA_ALPHA));
            if (alphaValue == 0 || alphaValue == 255) {
                KillTimer(window, AnimTimerId);
            }
            [[fallthrough]];
        case TimerId:
            window.InvalidateRect(false);
            break;
    }
    return 0;
}

LRESULT Widget::OnClose(WPARAM, LPARAM)
{
    App::Exit(0);
    return 0;
}

LRESULT Widget::OnResize(WPARAM, LPARAM lParam)
{
    RebuildSwapChain(LOWORD(lParam), HIWORD(lParam));
    return 0;
}

LRESULT Widget::OnHitTest(WPARAM, LPARAM)
{
    return HTCAPTION;
}

LRESULT Widget::OnCommand(WPARAM wParam, LPARAM)
{
    switch(LOWORD(wParam)) {
        case IDM_CLOCK_EXIT:
            PostQuitMessage(0);
            break;
        case IDM_CLOCK_MOVE_MODE: {
            if (ToggleMenuItemCheck(LOWORD(wParam))) {
                EnterMoveMode();
            } else {
                ExitMoveMode();
            }
        } break;
        case IDM_CLOCK_RUN_ON_START: {
            auto runKey = swal::RegKey_CurrentUser().OpenKey(
                runKeyName, KEY_WRITE
            );
            if (ToggleMenuItemCheck(LOWORD(wParam))) {
                runKey.SetString(appName, App::GetExeFileName());
            } else {
                runKey.DeleteValue(appName);
            }
        } break;
    }
    return 0;
}

LRESULT Widget::OnNotifyIcon(WPARAM wParam, LPARAM lParam)
{
    using TPM = swal::TrackPopupFlags;
    switch (LOWORD(lParam)) {
        case NIN_SELECT:
            swal::winapi_call(SetForegroundWindow(window));
            break;
        case WM_CONTEXTMENU: {
            auto popup = iconMenu.GetSubMenu(0);
            swal::winapi_call(SetForegroundWindow(window));
            popup.TrackPopup(TPM(0), GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam), window);
        } break;
    }
    return 0;
}

LRESULT Widget::OnExitSizeMove(WPARAM, LPARAM)
{
    auto rc = window.GetRect();
    auto appKey = swal::RegKey_CurrentUser().CreateKey(appRegKey, KEY_WRITE | KEY_READ);
    appKey.SetDWORD(posXValueName, DWORD(rc.left));
    appKey.SetDWORD(posYValueName, DWORD(rc.top));
    return 0;
}

ATOM Widget::GetClass()
{
    static ATOM cls = []{
        WNDCLASSEX wcex;
        wcex.cbSize = sizeof(wcex);
        wcex.style = 0;
        wcex.lpfnWndProc = swal::ClsWndProc<Widget, &Widget::WndProc, 0>;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = sizeof(Widget*);
        wcex.hInstance = App::GetHInstance();
        wcex.hIcon = LoadIcon(App::GetHInstance(), MAKEINTRESOURCE(IDI_CLOCK_ICON));
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        wcex.lpszMenuName = nullptr;
        wcex.lpszClassName = TEXT("clock_widget");
        wcex.hIconSm = wcex.hIcon;

        return swal::winapi_call(RegisterClassEx(&wcex));
    }();
    return cls;
}

void Widget::SetupWindow()
{
    using SWP = swal::SetPosFlags;
    using SW = swal::ShowCmd;
    MARGINS mr = { -1, -1, -1, -1 };
    swal::com_call(DwmExtendFrameIntoClientArea(window, &mr));
    window.SetLongPtr(GWL_STYLE, window.GetLongPtr(GWL_STYLE) &
        ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX));
    window.SetLongPtr(GWL_EXSTYLE, window.GetLongPtr(GWL_EXSTYLE) |
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
    window.SetPos(NULL, 0, 0, 128, 128, SWP::NoMove | SWP::NoZOrder | SWP::FrameChanged);
    try {
        auto appKey = swal::RegKey_CurrentUser().OpenKey(appRegKey, KEY_READ);
        window.SetPos(
            NULL,
            int(appKey.GetDWORD(posXValueName)),
            int(appKey.GetDWORD(posYValueName)),
            0, 0,
            SWP::NoZOrder | SWP::NoSize
        );
    } catch (std::system_error& e) {
        std::cerr << e.what();
    }
    SetLayeredWindowAttributes(window, 0, 255, LWA_ALPHA);
    window.Show(SW::ShowNormal);
    window.SetPos(HWND_TOPMOST, 0, 0, 0, 0, SWP::NoMove | SWP::NoSize);
}

void Widget::SetupRender()
{
    CreateRenderDevice();
    RebuildRenderTargetView();
    PrepareShadersAndLayout();
    MakeBuffers();
    MakeBlendState();
    LoadTextures();
}

void Widget::CreateRenderDevice()
{
    Microsoft::WRL::ComPtr<IDXGIFactory1> dxgiFactory;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgiAdapter;

    swal::com_call(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));

    static const D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_10_0 };

    auto rc = window.GetRect();

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferDesc.Width = rc.right - rc.left;
    sd.BufferDesc.Height = rc.bottom - rc.top;
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 1;
    sd.OutputWindow = window;
    sd.Windowed = TRUE;

    for (
        unsigned i = 0;
        (dxgiFactory->EnumAdapters1(i, &dxgiAdapter)) != DXGI_ERROR_NOT_FOUND;
        ++i
    ) {
        D3D_FEATURE_LEVEL FeatureLevel;

        if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(
            dxgiAdapter.Get(),
            D3D_DRIVER_TYPE_UNKNOWN,
            nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            FeatureLevels,
            std::size(FeatureLevels),
            D3D11_SDK_VERSION,
            &sd,
            &swapChain,
            &device,
            &FeatureLevel,
            &context
        ))) {
            DXGI_ADAPTER_DESC1 desc;
            std::fill(std::begin(desc.Description), std::end(desc.Description), 0);
            dxgiAdapter->GetDesc1(&desc);
            DWORD written;
            auto stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
            WriteConsole(stdoutHandle, desc.Description, wcslen(desc.Description), &written, nullptr);
            WriteConsole(stdoutHandle, L"\n", 1, &written, nullptr);
            dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
            return;
        }
    }
    throw std::runtime_error("No adapter with d3d11 support");
}

void Widget::RebuildRenderTargetView()
{
    Microsoft::WRL::ComPtr<ID3D11Texture2D> renderTarget;
    swal::com_call(swapChain->GetBuffer(0, IID_PPV_ARGS(&renderTarget)));
    swal::com_call(device->CreateRenderTargetView(renderTarget.Get(), nullptr, &rtView));
}

void Widget::RebuildSwapChain(int width, int height)
{
    if (device == nullptr) {
        return;
    }
    context->ClearState();
    rtView.Reset();
    swal::com_call(swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0));
    RebuildRenderTargetView();
    D3D11_VIEWPORT viewport = {
        .Width = float(width),
        .Height = float(height),
        .MinDepth = 0,
        .MaxDepth = 1
    };
    context->RSSetViewports(1, &viewport);
}

void Widget::PrepareShadersAndLayout()
{
    Microsoft::WRL::ComPtr<ID3DBlob> shaderCode;
    Microsoft::WRL::ComPtr<ID3DBlob> log;

    auto result = D3DCompile(
        generated::shaders::shader_frag_hlsl,
        std::strlen(generated::shaders::shader_frag_hlsl),
        nullptr, nullptr, nullptr,
        "fragmentMain",
        "ps_4_0",
        0, 0,
        &shaderCode,
        &log
    );
    if (FAILED(result)) {
        auto str = reinterpret_cast<char*>(log->GetBufferPointer());
        std::cerr << str << std::endl;
        throw std::runtime_error(str);
    }
    swal::com_call(device->CreatePixelShader(
        shaderCode->GetBufferPointer(),
        shaderCode->GetBufferSize(),
        nullptr,
        &fragShader
    ));

    result = D3DCompile(
        generated::shaders::shader_vert_hlsl,
        std::strlen(generated::shaders::shader_vert_hlsl),
        nullptr, nullptr, nullptr,
        "vertexMain",
        "vs_4_0",
        0, 0,
        &shaderCode,
        &log
    );
    if (FAILED(result)) {
        auto str = reinterpret_cast<char*>(log->GetBufferPointer());
        std::cerr << str << std::endl;
        throw std::runtime_error(str);
    }
    swal::com_call(device->CreateVertexShader(
        shaderCode->GetBufferPointer(),
        shaderCode->GetBufferSize(),
        nullptr,
        &vertShader
    ));

    D3D11_INPUT_ELEMENT_DESC layout[] = {{
        .SemanticName = "POSITION",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
        .InputSlot = 0,
        .AlignedByteOffset = 0,
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    }, {
        .SemanticName = "BLENDINDICES",
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT_R32G32B32A32_UINT,
        .InputSlot = 1,
        .AlignedByteOffset = 0,
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0
    }};
    swal::com_call(device->CreateInputLayout(
        layout,
        std::size(layout),
        shaderCode->GetBufferPointer(),
        shaderCode->GetBufferSize(),
        &inLayout
    ));
}

void Widget::MakeBuffers()
{
    auto bufDesc = CD3D11_BUFFER_DESC(
        sizeof(buffer),
        D3D11_BIND_VERTEX_BUFFER,
        D3D11_USAGE_IMMUTABLE,
        0
    );
    D3D11_SUBRESOURCE_DATA subResDesc = { .pSysMem = buffer };
    swal::com_call(device->CreateBuffer(&bufDesc, &subResDesc, &vertBuffer));

    bufDesc = CD3D11_BUFFER_DESC(
        sizeof(elements),
        D3D11_BIND_INDEX_BUFFER,
        D3D11_USAGE_IMMUTABLE,
        0
    );
    subResDesc = { .pSysMem = elements };
    swal::com_call(device->CreateBuffer(&bufDesc, &subResDesc, &idxBuffer));

    bufDesc = CD3D11_BUFFER_DESC(
        sizeof(ConstantBuffer),
        D3D11_BIND_CONSTANT_BUFFER,
        D3D11_USAGE_DYNAMIC,
        D3D11_CPU_ACCESS_WRITE
    );
    swal::com_call(device->CreateBuffer(&bufDesc, nullptr, &cBuffer));
}

void Widget::MakeBlendState()
{
    D3D11_BLEND_DESC blendDesc = {
        .AlphaToCoverageEnable = FALSE,
        .IndependentBlendEnable = FALSE
    };
    blendDesc.RenderTarget[0] = {
        .BlendEnable = TRUE,
        .SrcBlend = D3D11_BLEND_INV_DEST_ALPHA,
        .DestBlend = D3D11_BLEND_ONE,
        .BlendOp = D3D11_BLEND_OP_ADD,
        .SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA,
        .DestBlendAlpha = D3D11_BLEND_ONE,
        .BlendOpAlpha = D3D11_BLEND_OP_ADD,
        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL
    };
    swal::com_call(device->CreateBlendState(&blendDesc, &blendState));
}

Microsoft::WRL::ComPtr<ID3D11Texture2D> Widget::LoadTexture(dse::scn::ITextureDataProvider* prov)
{
    Microsoft::WRL::ComPtr<ID3D11Texture2D> result;
    using namespace dse::scn;
    using TDP = ITextureDataProvider;
    TDP::TextureParameters params;
    prov->LoadParameters(&params);
    DXGI_FORMAT format;
    switch (params.format) {
        case TDP::BGRA8sRGB:
            format = DXGI_FORMAT_B8G8R8A8_UNORM;
            break;
        case TDP::BGRA8:
            format = DXGI_FORMAT_B8G8R8A8_UNORM;
            break;
        case TDP::RGBA8sRGB:
            format = DXGI_FORMAT_R8G8B8A8_UNORM;
            break;
        case TDP::RGBA8:
            format = DXGI_FORMAT_R8G8B8A8_UNORM;
            break;
        default:
            throw std::runtime_error("Unsupported format");
    }

    D3D11_TEXTURE2D_DESC desc = {
        .Width = UINT(params.width),
        .Height = UINT(params.height),
        .MipLevels = UINT(params.lodCount),
        .ArraySize = UINT(params.depth),
        .Format = format,
        .SampleDesc = {
            .Count = 1,
            .Quality = 0
        },
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags = 0
    };
    swal::com_call(device->CreateTexture2D(&desc, nullptr, &result));
    D3D11_MAPPED_SUBRESOURCE subres = {};
    swal::com_call(context->Map(result.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subres));
    prov->LoadData(subres.pData, 0);
    context->Unmap(result.Get(), 0);
    return result;
}

void Widget::LoadTextures()
{
    using namespace dse::scn;
    using std::filesystem::path;

    BasicBitmapLoader loader1(path(App::GetExeFileName()).replace_filename("res.bmp").string().c_str());
    texture = LoadTexture(&loader1);
    device->CreateShaderResourceView(texture.Get(), nullptr, &shaderTexture);
}

void Widget::EnterMoveMode()
{
    using SWP = swal::SetPosFlags;
    UnhookWindowsHookEx(mouseHook);
    mouseHook = NULL;
    auto style = window.GetLongPtr(GWL_EXSTYLE) & ~WS_EX_TRANSPARENT;
    window.SetLongPtr(GWL_EXSTYLE, style);
    window.SetPos(NULL, 0, 0, 0, 0, SWP::NoMove | SWP::NoSize | SWP::NoZOrder | SWP::FrameChanged);
    PlayAppearAnimation();
}

void Widget::ExitMoveMode()
{
    using SWP = swal::SetPosFlags;
    auto style = window.GetLongPtr(GWL_EXSTYLE) | WS_EX_TRANSPARENT;
    window.SetLongPtr(GWL_EXSTYLE, style);
    window.SetPos(NULL, 0, 0, 0, 0, SWP::NoMove | SWP::NoSize | SWP::NoZOrder | SWP::FrameChanged);
    mouseHook = swal::winapi_call(SetWindowsHookEx(WH_MOUSE_LL, &Widget::MouseHook, NULL, 0));
}

void Widget::PlayAppearAnimation()
{
    alphaStep = FadeSpeed;
    SetTimer(window, AnimTimerId, 16, nullptr);
}

void Widget::PlayDisappearAnimation()
{
    alphaStep = -FadeSpeed;
    SetTimer(window, AnimTimerId, 16, nullptr);
}

bool Widget::ToggleMenuItemCheck(UINT item)
{
    MENUITEMINFO info;
    info.cbSize = sizeof(info);
    info.fMask = MIIM_STATE;
    iconMenu.GetItemInfo(item, false, &info);
    info.fState ^= MFS_CHECKED;
    bool checked = info.fState & MFS_CHECKED;
    iconMenu.SetItemInfo(item, false, &info);
    return checked;
}

void Widget::CheckMenuItem(UINT item, bool value)
{
    MENUITEMINFO info;
    info.cbSize = sizeof(info);
    info.fMask = MIIM_STATE;
    iconMenu.GetItemInfo(item, false, &info);
    info.fState &= ~UINT(MFS_CHECKED);
    info.fState |= MFS_CHECKED * value;
    iconMenu.SetItemInfo(item, false, &info);
}

void Widget::SetupMenu()
{
    auto runReg = swal::RegKey_CurrentUser().OpenKey(runKeyName, KEY_READ);
    try {
        runReg.QueryValue(appName, nullptr, nullptr, nullptr);
        CheckMenuItem(IDM_CLOCK_RUN_ON_START);
    }  catch (std::system_error& e) {}
}

LRESULT Widget::MouseHook(int code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0 || widget == nullptr) {
        return CallNextHookEx(NULL, code, wParam, lParam);
    }

    auto ms = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
    auto rc = widget->window.GetRect();

    switch (wParam) {
        case WM_MOUSEMOVE: {
            POINT pt = { (rc.right + rc.left) / 2, (rc.bottom + rc.top) / 2 };
            pt = { pt.x - ms->pt.x, pt.y - ms->pt.y };
            auto qDist = pt.x * pt.x + pt.y * pt.y;
            auto minDist = HideDistance * HideDistance;
            auto maxDist = (HideDistance + HideDistanceHyst) * (HideDistance + HideDistanceHyst);
            if (qDist < minDist && widget->alphaStep == FadeSpeed) {
                widget->PlayDisappearAnimation();
            }
            if (qDist > maxDist && widget->alphaStep == -FadeSpeed) {
                widget->PlayAppearAnimation();
            }
        } break;
    }

    return CallNextHookEx(NULL, code, wParam, lParam);
}

} // namespace clock_widget
