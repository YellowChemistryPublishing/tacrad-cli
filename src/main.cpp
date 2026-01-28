#include <Preamble.h>

#include <cstdlib>
#include <exception>

#include <module/sys.Threading>

#include <Clipboard.h>
#include <Config.h>
#include <Debug.h>
#include <Screen.h>
#include <components/Terminal.h>

int main()
{
    try
    {
        ui::ScreenInteractive& screen = Screen();
        screen.ForceHandleCtrlC(false);
        screen.ForceHandleCtrlZ(false);

        const ui::Component terminal = Terminal();
        const ui::Component uiRoot = ui::Renderer(terminal,
                                                  [&]
        {
            return ui::vbox({
                       ui::filler(),
                       ui::hbox({ ui::text(" "), terminal->Render() | ui::flex, ui::text(" ") }),
                   }) |
                ui::borderStyled(UserSettings::border);
        }) | TerminalQuickActionHandler(terminal) |
            Clipboard::clipboardHandler();

        const sys::platform::thread_pool threadPool;
        screen.Loop(uiRoot);

        return EXIT_SUCCESS;
    }
    catch (const std::exception& ex)
    {
        Debug::log("Uncaught exception: {}", ex.what());
    }
    catch (...)
    {
        Debug::log("Uncaught exception.");
    }
    return EXIT_FAILURE;
}
