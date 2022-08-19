#include "App.h"
#include <swal/error.h>
#include <iostream>

namespace clock_widget {

namespace {

HINSTANCE hInstance = GetModuleHandle(nullptr);
std::vector<std::string_view> args;

}

App::App()
{}

int App::Run()
{
    MSG msg;
    while (swal::winapi_call(GetMessage(&msg, NULL, 0, 0), swal::GetMessage_error_check)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return int(msg.wParam);
}

void App::Init(int argc, char* argv[])
{
    std::cout.sync_with_stdio(false);
    std::cerr.sync_with_stdio(false);
    for (int i = 0; i < argc; ++i) {
        args.push_back(argv[i]);
    }
}

HINSTANCE App::GetHInstance()
{
    return hInstance;
}

void App::Exit(int result)
{
    PostQuitMessage(result);
}

const swal::tstring& App::GetExeFileName()
{
    static const swal::tstring exeName = []{
        swal::tstring result(257, 0);
        while (true) {
            auto written = swal::winapi_call(GetModuleFileName(
                NULL, result.data(), result.size())
            );
            if (written == result.size()) {
                result.resize(result.size() * 2 - 1);
            } else {
                result.resize(written);
                break;
            }
        }
        result.shrink_to_fit();
        return result;
    }();
    return exeName;
}

} // namespace clock_widget
