set(SOURCES ContextMenus.cpp
            GUIViewStateFavourites.cpp
            GUIWindowFavourites.cpp
            FavouritesGUIInfo.cpp
            FavouritesService.cpp
            FavouritesURL.cpp
            FavouritesUtils.cpp)

set(HEADERS ContextMenus.h
            GUIViewStateFavourites.h
            GUIWindowFavourites.h
            FavouritesGUIInfo.h
            FavouritesService.h
            FavouritesURL.h
            FavouritesUtils.h)

core_add_library(favourites)
