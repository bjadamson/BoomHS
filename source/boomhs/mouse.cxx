#include <boomhs/mouse.hpp>
#include <common/algorithm.hpp>

namespace boomhs
{

CursorManager::CursorManager()
{
  for (size_t i = CURSOR_INDEX_BEGIN; i < CURSOR_INDEX_END; ++i) {
    cursors[i] = SDL_CreateSystemCursor((SDL_SystemCursor)i);
    assert(nullptr != cursors[i]);
  }
}

CursorManager::~CursorManager()
{
  FOR(i, cursors.size()) {
    auto *cursor = cursors[i];
    SDL_FreeCursor(cursor);
  }
}

bool
CursorManager::contains(SDL_SystemCursor const id) const
{
  return id >= CURSOR_INDEX_BEGIN && id < CURSOR_INDEX_END;
}

void
CursorManager::set_active(SDL_SystemCursor const id)
{
  active_ = id;

  assert(contains(active_));
  SDL_SetCursor(cursors[active_]);
}

SDL_Cursor*
CursorManager::active() const
{
  return cursors[active_];
}

} // namespace boomhs
