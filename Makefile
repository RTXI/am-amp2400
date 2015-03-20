PLUGIN_NAME = am_amp2400

HEADERS = am_2400_commander.h \
          am_2400_commanderUI.h

SOURCES = am_2400_commander.cpp \
          am_2400_commanderUI.cpp \
          moc_am_2400_commander.cpp \
          moc_am_2400_commanderUI.cpp 

LIBS = 

### Do not edit below this line ###

include $(shell rtxi_plugin_config --pkgdata-dir)/Makefile.plugin_compile
