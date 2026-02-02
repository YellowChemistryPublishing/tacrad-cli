#include <Preamble.h>

#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <vector>

#include <module/sys>

#include <Clipboard.h>
#include <Config.h>
#include <Debug.h>
#include <Exec.h> // NOLINT(misc-include-cleaner)
#include <Screen.h>
#include <Style.h>
#include <components/Console.h>
#include <components/Playlist.h>
#include <components/StatusBar.h>
#include <components/Terminal.h>

int main()
{
    try
    {
        ui::ScreenInteractive& screen = Screen();
        screen.ForceHandleCtrlC(false);
        screen.ForceHandleCtrlZ(false);

        // Tab Selector

        const std::vector<std::string> tabValues = {
            "UI",
            "Console",
        };
        i32 tabSelected = 0;
        ui::MenuOption toggleOptions = ui::MenuOption::HorizontalAnimated();
        toggleOptions.underline.color_active = Config::FlavorEmphasizedColor;
        toggleOptions.underline.color_inactive = Config::FlavorUnemphasizedColor;
        toggleOptions.underline.leader_duration = Config::FlavorAnimationDuration;
        toggleOptions.underline.leader_function = ui::animation::easing::Linear;
        toggleOptions.underline.follower_duration = Config::FlavorAnimationDuration;
        toggleOptions.underline.follower_function = ui::animation::easing::Linear;
        ui::Component tabSelector = ui::Menu(&tabValues, &*tabSelected, toggleOptions);

        // UI

        i32 leftSize = 20_i32;  // NOLINT(readability-magic-numbers)
        i32 rightSize = 32_i32; // NOLINT(readability-magic-numbers)
        const ui::Component playlist = Playlist();
        const ui::Component left = ui::Renderer([] { return ui::text("> all") | ui::bold; });
        const ui::Component right = ui::Renderer([] { return ui::text("details here"); });
        ui::Component container =
            ui::ResizableSplit({ .main = left, .back = playlist, .direction = ui::Direction::Left, .main_size = &*leftSize, .separator_func = [] { return psep(); } });
        container = ui::ResizableSplit({ .main = right, .back = container, .direction = ui::Direction::Right, .main_size = &*rightSize, .separator_func = [] { return psep(); } });

        const std::shared_ptr<StatusBarImpl> statusBar = std::static_pointer_cast<StatusBarImpl>(StatusBar());

        // Console
        const ui::Component console = Console();

        // Combine into full page.

        ui::Component tabContainer = ui::Container::Tab({ ui::Container::Vertical({ ui::Renderer(container, [&]() -> ui::Element { return container->Render() | ui::yflex; }),
                                                                                    ui::Renderer([] { return ui::separatorStyled(UserSettings::border); }), statusBar }),
                                                          console },
                                                        &*tabSelected);

        ui::Component terminal = Terminal(statusBar);

        // Root

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
