#include <Preamble.h>

#include <cstdlib>
#include <exception>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <memory>

#include <Clipboard.h>
#include <Config.h>
#include <Debug.h>
#include <Screen.h>
#include <Style.h>
#include <components/Console.h>
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
        toggleOptions.underline.leader_duration = Config::FlavorAnimationDuration;
        toggleOptions.underline.leader_function = ui::animation::easing::Linear;
        toggleOptions.underline.follower_duration = Config::FlavorAnimationDuration;
        toggleOptions.underline.follower_function = ui::animation::easing::Linear;
        ui::Component tabSelector = ui::Menu(&tabValues, &*tabSelected, toggleOptions);

        // UI

        i32 leftSize = 10_i32;
        i32 rightSize = 10_i32;
        ui::Component middle = ui::Renderer([] { return ui::text("Middle") | ui::center; });
        ui::Component left = ui::Renderer([] { return ui::text("Left"); });
        ui::Component right = ui::Renderer([] { return ui::text("Right"); });
        ui::Component container = ui::ResizableSplit({ .main = left,
                                                       .back = middle,
                                                       .direction = ui::Direction::Left,
                                                       .main_size = &*leftSize,
                                                       .separator_func = [] { return ui::separatorStyled(UserSettings::border); } });
        container = ui::ResizableSplit({ .main = right,
                                         .back = container,
                                         .direction = ui::Direction::Right,
                                         .main_size = &*rightSize,
                                         .separator_func = [] { return ui::separatorStyled(UserSettings::border); } });

        std::shared_ptr<StatusBarImpl> statusBar = std::static_pointer_cast<StatusBarImpl>(StatusBar());

        // Console
        ui::Component console = Console();

        // Combine into full page.

        ui::Component tabContainer = ui::Container::Tab({ ui::Container::Vertical({ ui::Renderer(container, [&]() -> ui::Element { return container->Render() | ui::yflex_grow; }),
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

        ui::Component uiRoot = ui::Renderer(rootContainer, [&]() -> ui::Element { return rootContainer->Render() | ui::borderStyled(UserSettings::border); }) |
            TerminalQuickActionHandler(terminal) | Clipboard::clipboardHandler();

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
