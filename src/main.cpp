#include <Preamble.h>

#include <cstdlib>
#include <exception>
#include <memory>

#include <Clipboard.h>
#include <Config.h>
#include <Debug.h>
#include <Exec.h> // NOLINT(misc-include-cleaner)
#include <Screen.h>
#include <Style.h>
#include <components/Console.h>
#include <components/TabContainer.h>
#include <components/TabSelect.h>
#include <components/Terminal.h>
#include <components/UI.h>

int main()
{
    try
    {
        ui::ScreenInteractive& screen = Screen();
        screen.ForceHandleCtrlC(false);
        screen.ForceHandleCtrlZ(false);

        const std::shared_ptr<UIImpl> ui = std::static_pointer_cast<UIImpl>(UI());
        const ui::Component console = Console();

        std::shared_ptr<TabContainerImpl> tabContainer = std::static_pointer_cast<TabContainerImpl>(TabContainer({ ui, console }));
        ui::Component tabSelector = TabSelect(tabContainer);

        ui::Component terminal = Terminal(ui->statusBar());

        ui::Component rootContainer = ui::Container::Vertical({
            ui::Renderer(tabSelector, [&]() -> ui::Element { return hpad(tabSelector->Render()); }),
            ui::Renderer(tabContainer, [&]() -> ui::Element { return hpad(tabContainer->Render()) | ui::flex; }),
            ui::Renderer(terminal, [&]() -> ui::Element { return hpad(terminal->Render()); }),
        });

        const ui::Component uiRoot = ui::Renderer(rootContainer, [&]() -> ui::Element { return rootContainer->Render() | ui::borderStyled(UserSettings::border); }) |
            TerminalSpaceToFocusHandler(terminal) | TerminalQuickActionHandler(terminal) | ClipboardHandler();

        screen.Loop(uiRoot);

        return EXIT_SUCCESS;
    }
    catch (const std::exception& ex)
    {
        debugLog("Uncaught exception: {}", ex.what());
    }
    catch (...)
    {
        debugLog("Uncaught exception.");
    }

    return EXIT_FAILURE;
}
