#ifndef CLOCK_WIDGET_APP_H
#define CLOCK_WIDGET_APP_H

#include <swal/win_headers.h>
#include <swal/strconv.h>
#include <string_view>
#include <vector>

namespace clock_widget {

class App
{
public:
    static int Run();
    static void Init(int argc, char* argv[]);
    static auto GetHInstance() -> HINSTANCE;
    static void Exit(int result);
    static auto GetExeFileName() -> const swal::tstring& ;
private:
    App();
    App(App&&) = delete;
    App& operator=(App&&) = delete;
};

} // namespace clock_widget

#endif // CLOCK_WIDGET_APP_H
