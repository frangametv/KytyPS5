# UI languages

Each UTF-8 JSON file declares one language. English (`en`) is the default.
The selected language is stored separately from the emulated console language
in `Kyty.ini`, under `MainDialog/ui_language`, and changes apply immediately.

To add a language:

1. Copy `en.json` to a new file, for example `fr.json`.
2. Set `code` to a unique language code, for example `fr` or `pt-BR`.
3. Set `label` to the language's native name, for example `Français`.
4. Translate the values inside `translations`. Keep the English keys unchanged.
5. Build `kyty_library`. CMake automatically detects and embeds the new file;
   no C++ or QML changes and no separate translation compiler are needed.

Example (a partial catalog is valid):

```json
{
  "code": "fr",
  "label": "Français",
  "translations": {
    "Library": "Bibliothèque",
    "Options": "Paramètres",
    "Interface language": "Langue de l'interface"
  }
}
```

Preserve placeholders such as `%1`, line breaks (`\n`), and significant spaces
(for example the leading space in ` game`). Missing keys fall back to English.
The language label is shown as written, so users can recognize their language.
Invalid JSON, missing metadata and duplicate codes are not added to the menu.

Catalogs are packaged inside the application on Windows, Linux and macOS.
Adding a language to the source folder requires rebuilding the application.
Game titles, paths, identifiers, configuration values and the emulator's raw
console output are not translated. Translation never changes emulator options.
