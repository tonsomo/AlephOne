## Process this file with automake to produce Makefile.in 

noinst_LIBRARIES = libcseries.a
libcseries_a_SOURCES = byte_swapping.h csalerts.h csdialogs.h cscluts.h \
  cseries.h csfiles.h csfonts.h csmacros.h csmisc.h cspixels.h csstrings.h \
  cstypes.h mytm.h sdl_cseries.h snprintf.h \
  \
  byte_swapping.cpp csalerts_sdl.cpp cscluts_sdl.cpp csdialogs_sdl.cpp \
  csmisc_sdl.cpp csstrings.cpp mytm_sdl.cpp snprintf.cpp

EXTRA_libcseries_a_SOURCES = csfiles_beos.cpp

INCLUDES = -I$(top_srcdir)/Source_Files/Files -I$(top_srcdir)/Source_Files/GameWorld \
  -I$(top_srcdir)/Source_Files/Input -I$(top_srcdir)/Source_Files/Misc \
  -I$(top_srcdir)/Source_Files/ModelView -I$(top_srcdir)/Source_Files/Network \
  -I$(top_srcdir)/Source_Files/Pfhortran -I$(top_srcdir)/Source_Files/RenderMain \
  -I$(top_srcdir)/Source_Files/RenderOther -I$(top_srcdir)/Source_Files/Sound \
  -I$(top_srcdir)/Source_Files/XML -I$(top_srcdir)/Source_Files/Network/Metaserver \
  -I$(top_srcdir)/Source_Files/TCPMess
