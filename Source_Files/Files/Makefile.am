## Process this file with automake to produce Makefile.in 

noinst_LIBRARIES = libfiles.a

libfiles_a_SOURCES = AStream.h crc.h extensions.h FileHandler.h find_files.h \
  game_wad.h Packing.h resource_manager.h tags.h wad.h wad_prefs.h \
  \
  AStream.cpp crc.cpp FileHandler_SDL.cpp find_files_sdl.cpp game_wad.cpp \
  import_definitions.cpp Packing.cpp preprocess_map_sdl.cpp \
  preprocess_map_shared.cpp resource_manager.cpp wad.cpp wad.cpp \
  wad_prefs.cpp wad_sdl.cpp

INCLUDES = -I$(top_srcdir)/Source_Files/CSeries -I$(top_srcdir)/Source_Files/GameWorld \
  -I$(top_srcdir)/Source_Files/Input -I$(top_srcdir)/Source_Files/Misc \
  -I$(top_srcdir)/Source_Files/ModelView -I$(top_srcdir)/Source_Files/Network \
  -I$(top_srcdir)/Source_Files/Pfhortran -I$(top_srcdir)/Source_Files/RenderMain \
  -I$(top_srcdir)/Source_Files/RenderOther -I$(top_srcdir)/Source_Files/Sound \
  -I$(top_srcdir)/Source_Files/XML -I$(top_srcdir)/Source_Files
