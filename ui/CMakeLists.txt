PROJECT(toplevel_ui_sources) # Don't rename - it's referenced elsewhere.

FILE(GLOB ui_files "*.ui")
SOURCE_GROUP("UI Files" FILES ${ui_files})

QT5_WRAP_UI(ui_sources ${ui_files})
ADD_CUSTOM_TARGET(toplevel_ui_sources SOURCES ${ui_sources})
QT5_USE_MODULES(toplevel_ui_sources Gui) # Just to process those .ui files

TRANSLATION_SOURCES(scantailor ${ui_files})
