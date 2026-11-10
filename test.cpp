#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <iostream>

int main() {
  using namespace ftxui;

  // Track the current active page (0, 1, or 2)
  int page_index = 0;

  // Define the layout container. We use CatchEvent to intercept keypresses.
  auto renderer = Renderer([&] {
    // 1. Define the content for each of your 3 pages
    Element page_0 = vbox({
      text("--- PAGE 1 ---") | bold | color(Color::Green),
      separator(),
      text("Welcome to the first screen!"),
      text("Press [Right Arrow] or [2] to go to Page 2."),
    }) | border;

    Element page_1 = vbox({
      text("--- PAGE 2 ---") | bold | color(Color::Yellow),
      separator(),
      text("This is the middle screen."),
      text("Press [Left/Right Arrows] to navigate."),
    }) | border;

    Element page_2 = vbox({
      text("--- PAGE 3 ---") | bold | color(Color::Blue),
      separator(),
      text("You reached the final screen!"),
      text("Press [Left Arrow] or [1] to go back, or [q] to exit."),
    }) | border;

    // 2. Select which page element to render based on the current index
    Element active_page = page_0;
    if (page_index == 1) active_page = page_1;
    if (page_index == 2) active_page = page_2;

    // 3. Wrap it in a nice master layout with instructions at the bottom
    return vbox({
      active_page | flex,
      separator(),
      hbox({
        text(" Navigation: [1][2][3] or Arrow Keys ") | dim,
        filler(),
        text(" Press [q] to Quit ") | inverted
      })
    });
  });

  // 4. Handle input events to switch pages
  auto component = CatchEvent(renderer, [&](Event event) {
    if (event == Event::Character('q') || event == Event::Escape) {
      ScreenInteractive::Active()->ExitLoopClosure()();
      return true;
    }
    
    // Switch using direct number keys
    if (event == Event::Character('1')) { page_index = 0; return true; }
    if (event == Event::Character('2')) { page_index = 1; return true; }
    if (event == Event::Character('3')) { page_index = 2; return true; }

    // Switch using Arrow Keys
    if (event == Event::ArrowRight) {
      if (page_index < 2) page_index++;
      return true;
    }
    if (event == Event::ArrowLeft) {
      if (page_index > 0) page_index--;
      return true;
    }

    return false;
  });

  // 5. Run the interactive loop fullscreen
  auto screen = ScreenInteractive::Fullscreen();
  screen.Loop(component);

  return 0;
}
