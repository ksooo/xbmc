set(SOURCES PVRIntSettingDefinition.cpp
            PVRIntSettingValues.cpp
            PVRSettings.cpp
            PVRStringSettingDefinition.cpp
            PVRStringSettingValues.cpp
            PVRTimerSettingDefinition.cpp)

set(HEADERS PVRIntSettingDefinition.h
            PVRIntSettingValues.h
            PVRSettings.h
            PVRStringSettingDefinition.h
            PVRStringSettingValues.h
            PVRTimerSettingDefinition.h)

core_add_library(pvr_settings)
