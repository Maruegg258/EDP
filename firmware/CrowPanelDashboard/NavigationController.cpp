#include "NavigationController.h"

NavigationController::NavigationController(uint8_t pageCount)
    : _pageCount(pageCount == 0 ? 1 : pageCount) {
}

NavigationState NavigationController::state() const {
  return {
    _pageIndex,
    _pageCount,
    _mode
  };
}

bool NavigationController::handle(InputEvent event) {
  const uint8_t previousPage = _pageIndex;
  const NavigationMode previousMode = _mode;

  switch (event) {
    case InputEvent::UP:
      if (_mode == NavigationMode::PAGE) {
        moveUp();
      }
      break;

    case InputEvent::DOWN:
      if (_mode == NavigationMode::PAGE) {
        moveDown();
      }
      break;

    case InputEvent::MENU:
      if (_mode == NavigationMode::PAGE) {
        _mode = NavigationMode::DETAIL;
      }
      break;

    case InputEvent::EXIT:
      if (_mode == NavigationMode::DETAIL) {
        _mode = NavigationMode::PAGE;
      }
      break;

    case InputEvent::NONE:
      break;
  }

  return _pageIndex != previousPage ||
         _mode != previousMode;
}

void NavigationController::moveUp() {
  if (_pageIndex == 0) {
    _pageIndex = static_cast<uint8_t>(_pageCount - 1);
    return;
  }

  --_pageIndex;
}

void NavigationController::moveDown() {
  ++_pageIndex;

  if (_pageIndex >= _pageCount) {
    _pageIndex = 0;
  }
}
