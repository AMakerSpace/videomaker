# SPDX-License-Identifier: GPL-3.0-only
# SPDX-FileCopyrightText: 2026 AMakerSpace

set(kdenlive_SRCS
  ${kdenlive_SRCS}
    videomaker/amakertheme.cpp
    videomaker/simplemode.cpp
  PARENT_SCOPE)

# Farbschemata, Menüeintrag und Icon
install(FILES ${CMAKE_SOURCE_DIR}/videomaker/theme/AMakerLight.colors ${CMAKE_SOURCE_DIR}/videomaker/theme/AMakerDark.colors
        DESTINATION ${KDE_INSTALL_DATADIR}/color-schemes)
install(FILES ${CMAKE_SOURCE_DIR}/videomaker/de.amakerspace.VideoMaker.desktop DESTINATION ${KDE_INSTALL_APPDIR})
foreach(size 48 64 128 256 512)
  install(FILES ${CMAKE_SOURCE_DIR}/videomaker/icons/videomaker-${size}.png
          DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/${size}x${size}/apps RENAME de.amakerspace.VideoMaker.png)
endforeach()
