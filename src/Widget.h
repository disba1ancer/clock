#ifndef CLOCK_WIDGET_H
#define CLOCK_WIDGET_H

#include <swal/window.h>
#include <swal/menu.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <unordered_map>
#include "ITextureDataProvider.h"

namespace clock_widget {

class Widget
{
public:
    Widget();
    ~Widget();
private:
    static ATOM GetClass();
    auto WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT;
    auto OnPaint(WPARAM, LPARAM) -> LRESULT;
    auto OnTimer(WPARAM, LPARAM) -> LRESULT;
    auto OnClose(WPARAM, LPARAM) -> LRESULT;
    auto OnResize(WPARAM, LPARAM) -> LRESULT;
    auto OnHitTest(WPARAM, LPARAM) -> LRESULT;
    auto OnCommand(WPARAM, LPARAM) -> LRESULT;
    auto OnNotifyIcon(WPARAM, LPARAM) -> LRESULT;
    auto OnExitSizeMove(WPARAM, LPARAM) -> LRESULT;
    void SetupWindow();
    void SetupRender();
    void CreateRenderDevice();
    void RebuildRenderTargetView();
    void RebuildSwapChain(int width, int height);
    void PrepareShadersAndLayout();
    void MakeBuffers();
    void MakeBlendState();
    auto LoadTexture(dse::scn::ITextureDataProvider *prov)
         -> Microsoft::WRL::ComPtr<ID3D11Texture2D>;
    void LoadTextures();
    void EnterMoveMode();
    void ExitMoveMode();
    void PlayAppearAnimation();
    void PlayDisappearAnimation();
    bool ToggleMenuItemCheck(UINT item);
    void CheckMenuItem(UINT item, bool value = true);
    void SetupMenu();

    static LRESULT MouseHook(int code, WPARAM wParam, LPARAM lParam);

    static const std::unordered_map<UINT, LRESULT (Widget::*)(WPARAM, LPARAM)> message_map;

    static constexpr UINT_PTR TimerId = 0;
    static constexpr UINT_PTR AnimTimerId = 1;
    static constexpr long HideDistance = 64;
    static constexpr long HideDistanceHyst = 32;
    static constexpr int FadeSpeed = 15;
    static constexpr UINT NotifyIconId = 0;
    static constexpr UINT NotifyIconMsg = WM_USER + 16;

    swal::Window window;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtView;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inLayout;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> fragShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> idxBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> cBuffer;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blendState;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderTexture;
    HHOOK mouseHook;
    int alphaValue;
    int alphaStep;
    swal::Menu iconMenu;
    swal::tstring exeName;
};

} // namespace clock_widget

#endif // CLOCK_WIDGET_H
