# VideoMaker-Fork

Kdenlive 25.12 mit iMovie-artigem **Einfach-Modus** im AMaker-Stil. Zielbild: `../../docs/ziel-imovie.html`.

| Wo | Was |
|---|---|
| `src/videomaker/amakertheme.*` | Schrift, Hell/Dunkel/Auto, Stylesheet, Sprache (Deutsch Standard) |
| `src/videomaker/simplemode.*` | Einfach-Modus: Kopfleiste, Werkzeuge über der Vorschau, Film-Leiste, Umschalten auf Erweitert |
| `videomaker/` (dieser Ordner) | Thema, Schrift, Icon als Qt-Ressourcen – erzeugt von `scripts/kit-sync.sh` aus `amaker/` |
| `data/layouts/editing.json` | Einfach-Layout (Medien · Vorschau · Film); `advanced.json` = Original-Kdenlive |

Kleine Eingriffe in Kdenlive-Dateien, alle mit „VideoMaker“ kommentiert:
`src/main.cpp`, `src/mainwindow.{h,cpp}`, `src/utils/thememanager.cpp`, `src/timeline2/view/timelinewidget.cpp`,
`src/timeline2/view/qml/Timeline.qml`, `src/kdenlivesettings.kcfg`, `src/CMakeLists.txt`, `data/layouts/CMakeLists.txt`.

Einstellungen in `~/.config/videomakerrc`, Gruppe `[VideoMaker]`: `Sprache=de|en`, `Thema=auto|hell|dunkel`, `Erweitert=true|false`, `Bewegung=an|aus`.

Bauen und starten:

```bash
~/videomaker/scripts/bauen.sh
~/videomaker/scripts/bauen.sh start
```
