#include <Preamble.h>

#include <cstdlib>
#include <exception>
#include <memory>

#include <module/sys.Threading>

#include <Clipboard.h>
#include <Config.h>
#include <Debug.h>
#include <Screen.h>
#include <Style.h>
#include <components/StatusBar.h>
#include <components/Terminal.h>

int main()
{
    try
    {
        ui::ScreenInteractive& screen = Screen();
        screen.ForceHandleCtrlC(false);
        screen.ForceHandleCtrlZ(false);

        std::shared_ptr<StatusBarImpl> statusBar = std::static_pointer_cast<StatusBarImpl>(StatusBar());
        const ui::Component terminal = Terminal(statusBar);
        const ui::Component uiRoot = ui::Renderer(terminal,
                                                  [&]
        {
            return ui::vbox({
                       ui::filler(),
                       statusBar->Render(),
                       hpad(terminal->Render()),
                   }) |
                ui::borderStyled(UserSettings::border);
        }) | TerminalQuickActionHandler(terminal) |
            Clipboard::clipboardHandler();

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
