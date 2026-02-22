#include <cstdlib>
#include <exception>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <memory>
#include <utility>

#include <Clipboard.h>
#include <CmdInv.h> // NOLINT(misc-include-cleaner): Need function definitions.
#include <Debug.h>
#include <Screen.h>
#include <Style.h>
#include <components/Console.h>
#include <components/TabContainer.h>
#include <components/TabSelect.h>
#include <components/Terminal.h>
#include <components/UI.h>

namespace ui = ftxui;

int main()
{
    try
    {
        Screen().ForceHandleCtrlC(false);
        Screen().ForceHandleCtrlZ(false);

        const std::shared_ptr<ConsoleImpl> console = std::static_pointer_cast<ConsoleImpl>(Console());
        const std::shared_ptr<UIImpl> ui = std::static_pointer_cast<UIImpl>(UI(console));

        std::shared_ptr<TabContainerImpl> tabContainer = std::static_pointer_cast<TabContainerImpl>(TabContainer({ ui, console }));
        ui::Component tabSelector = TabSelect(tabContainer);
        const std::shared_ptr<TerminalImpl> terminal = std::static_pointer_cast<TerminalImpl>(Terminal(console, ui->statusBar()));

        ui::Component rootContainer = ui::Container::Vertical({
                                          std::move(tabSelector),
                                          std::move(tabContainer) | ui::yflex,
                                          terminal,
                                      }) |
            hpad;

        const ui::Component uiRoot = ui::Renderer(rootContainer, [&rootContainer]() -> ui::Element { return rootContainer->Render() | bordered; }) |
            TerminalSpaceToFocusHandler(terminal) | TerminalQuickActionHandler(terminal) | ClipboardHandler();

        Screen().Loop(uiRoot);

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
