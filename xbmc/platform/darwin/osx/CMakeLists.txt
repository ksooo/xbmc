set(SOURCES CocoaInterface.mm
            CPUInfoOsx.cpp
            GPUInfoMacOS.cpp
            HbbTVWebView.mm
            HotKeyController.mm
            MediaKeys.mm
            PlatformDarwinOSX.mm)

set(HEADERS CocoaInterface.h
            CPUInfoOsx.h
            GPUInfoMacOS.h
            HbbTVWebView.h
            HotKeyController.h
            MediaKeys.h
            PlatformDarwinOSX.h)

core_add_library(platform_osx)
