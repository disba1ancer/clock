#include "App.h"
#include "Widget.h"

int main(int argc, char* argv[])
{
    using namespace clock_widget;
    App::Init(argc, argv);
    Widget widget;
    return App::Run();
}
