#.rst:
# FindExiv2
# -------
# Finds the exiv2 library
#
# This will define the following imported targets::
#
#   ${APP_NAME_LC}::Exiv2   - The EXIV2 library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  macro(buildexiv2)
    find_package(Brotli REQUIRED ${SEARCH_QUIET})
    find_package(Iconv REQUIRED ${SEARCH_QUIET})
    find_package(Zlib REQUIRED ${SEARCH_QUIET})

    # Patch pending review upstream (https://github.com/Exiv2/exiv2/pull/3004)
    set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/0001-WIN-lib-postfix.patch")

    generate_patchcommand("${patches}")

    if(WIN32 OR WINDOWS_STORE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX d)

      # Exiv2 cant be built using /RTC1, so we alter and disable the auto addition of flags
      # using WIN_DISABLE_PROJECT_FLAGS
      string(REPLACE "/RTC1" "" ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} )

      set(EXTRA_ARGS "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}$<$<CONFIG:Debug>: ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CXX_FLAGS_DEBUG}>$<$<CONFIG:Release>: ${CMAKE_CXX_FLAGS_RELEASE}>"
                     "-DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}$<$<CONFIG:Debug>: ${CMAKE_EXE_LINKER_FLAGS_DEBUG}>$<$<CONFIG:Release>: ${CMAKE_EXE_LINKER_FLAGS_RELEASE}>")

      set(WIN_DISABLE_PROJECT_FLAGS ON)
    endif()

    set(CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                   -DEXIV2_ENABLE_WEBREADY=OFF
                   -DEXIV2_ENABLE_XMP=OFF
                   -DEXIV2_ENABLE_CURL=OFF
                   -DEXIV2_ENABLE_NLS=OFF
                   -DEXIV2_BUILD_SAMPLES=OFF
                   -DEXIV2_BUILD_UNIT_TESTS=OFF
                   -DEXIV2_ENABLE_VIDEO=OFF
                   -DEXIV2_ENABLE_BMFF=ON
                   -DEXIV2_ENABLE_BROTLI=ON
                   -DEXIV2_ENABLE_INIH=OFF
                   -DEXIV2_ENABLE_FILESYSTEM_ACCESS=OFF
                   -DEXIV2_BUILD_EXIV2_COMMAND=OFF
                   ${EXTRA_ARGS})

    if(NOT CMAKE_CXX_COMPILER_LAUNCHER STREQUAL "")
      list(APPEND CMAKE_ARGS -DBUILD_WITH_CCACHE=ON)
    endif()

    BUILD_DEP_TARGET()

    # Link libraries for target interface
    set(EXIV2_LINK_LIBRARIES LIBRARY::Brotli LIBRARY::Iconv LIBRARY::Zlib)

    # Add dependencies to build target
    add_dependencies(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC} LIBRARY::Brotli)
    add_dependencies(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC} LIBRARY::Iconv)
    add_dependencies(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC} LIBRARY::Zlib)
  endmacro()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  # If there is a potential this library can be built internally
  # Check its dependencies to allow forcing this lib to be built if one of its
  # dependencies requires being rebuilt
  if(ENABLE_INTERNAL_EXIV2)
    # Dependency list of this find module for an INTERNAL build
    set(${CMAKE_FIND_PACKAGE_NAME}_DEPLIST Brotli
                                           Iconv
                                           Zlib)

    check_dependency_build(${CMAKE_FIND_PACKAGE_NAME} "${${CMAKE_FIND_PACKAGE_NAME}_DEPLIST}")
  endif()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC exiv2)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  find_package(exiv2 ${CONFIG_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} CONFIG ${SEARCH_QUIET}
                     HINTS ${DEPENDS_PATH}/lib/cmake
                     ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  # cmake config may not be available
  # fallback to pkgconfig for non windows platforms
  if(NOT exiv2_FOUND)
    find_package(PkgConfig ${SEARCH_QUIET})
    if(PKG_CONFIG_FOUND AND NOT (WIN32 OR WINDOWSSTORE))
      pkg_check_modules(exiv2 exiv2${PC_${CMAKE_FIND_PACKAGE_NAME}_FIND_SPEC} ${SEARCH_QUIET} IMPORTED_TARGET)
    endif()
  endif()

  # Check for existing EXIV2. If version >= EXIV2-VERSION file version, dont build
  if((exiv2_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_EXIV2) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_EXIV2) OR
     (DEFINED ${CMAKE_FIND_PACKAGE_NAME}_FORCE_BUILD))

    buildexiv2()
  else()
    if(TARGET Exiv2::exiv2lib OR TARGET exiv2lib)
      # We create variables based off TARGET data for use with FPHSA
      # exiv2 < 0.28 uses a non namespaced target, but also has an alias. Prioritise
      # namespaced target, and fallback to old target for < 0.28
      if(TARGET Exiv2::exiv2lib)
        set(_exiv_target_name Exiv2::exiv2lib)
      else()
        set(_exiv_target_name exiv2lib)
      endif()

      # This is for the case where a distro provides a non standard (Debug/Release) config type
      # eg Debian's config file is exiv2Config-none.cmake
      # convert this back to either DEBUG/RELEASE or just RELEASE
      # we only do this because we use find_package_handle_standard_args for config time output
      # and it isnt capable of handling TARGETS, so we have to extract the info
      get_target_property(_EXIV2_CONFIGURATIONS ${_exiv_target_name} IMPORTED_CONFIGURATIONS)
      foreach(_exiv2_config IN LISTS _EXIV2_CONFIGURATIONS)
        # Some non standard config (eg None on Debian)
        # Just set to RELEASE var so select_library_configurations can continue to work its magic
        string(TOUPPER ${_exiv2_config} _exiv2_config_UPPER)
        if((NOT ${_exiv2_config_UPPER} STREQUAL "RELEASE") AND
          (NOT ${_exiv2_config_UPPER} STREQUAL "DEBUG"))
          get_target_property(EXIV2_LIBRARY_RELEASE ${_exiv_target_name} IMPORTED_LOCATION_${_exiv2_config_UPPER})
        else()
          get_target_property(EXIV2_LIBRARY_${_exiv2_config_UPPER} ${_exiv_target_name} IMPORTED_LOCATION_${_exiv2_config_UPPER})
        endif()
      endforeach()

      get_target_property(EXIV2_INCLUDE_DIR ${_exiv_target_name} INTERFACE_INCLUDE_DIRECTORIES)
    elseif(TARGET PkgConfig::exiv2)
      # First item is the full path of the library file found
      # pkg_check_modules does not populate a variable of the found library explicitly
      list(GET exiv2_LINK_LIBRARIES 0 EXIV2_LIBRARY_RELEASE)

      get_target_property(EXIV2_INCLUDE_DIR PkgConfig::exiv2 INTERFACE_INCLUDE_DIRECTORIES)
    endif()
  endif()

  include(SelectLibraryConfigurations)
  select_library_configurations(EXIV2)
  unset(EXIV2_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Exiv2
                                    REQUIRED_VARS EXIV2_LIBRARY EXIV2_INCLUDE_DIR
                                    VERSION_VAR EXIV2_VER)

  if(EXIV2_FOUND)
    if((TARGET Exiv2::exiv2lib OR TARGET exiv2lib) AND NOT TARGET exiv2)
      # Exiv alias exiv2lib in their latest cmake config. We test for the alias
      # to workout what we need to point OUR alias at.
      get_target_property(_EXIV2_ALIASTARGET exiv2lib ALIASED_TARGET)
      if(_EXIV2_ALIASTARGET)
        set(_exiv_target_name ${_EXIV2_ALIASTARGET})
      endif()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${_exiv_target_name})
    elseif(TARGET PkgConfig::exiv2 AND NOT TARGET exiv2)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::exiv2)
    else()
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} UNKNOWN IMPORTED)
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                       INTERFACE_INCLUDE_DIRECTORIES "${EXIV2_INCLUDE_DIR}"
                                                                       INTERFACE_LINK_LIBRARIES "${EXIV2_LINK_LIBRARIES}")

      if(EXIV2_LIBRARY_RELEASE)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_CONFIGURATIONS RELEASE
                                                                         IMPORTED_LOCATION_RELEASE "${EXIV2_LIBRARY_RELEASE}")
      endif()
      if(EXIV2_LIBRARY_DEBUG)
        set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                         IMPORTED_LOCATION_DEBUG "${EXIV2_LIBRARY_DEBUG}")
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                              IMPORTED_CONFIGURATIONS DEBUG)
      endif()

      if(CORE_SYSTEM_NAME STREQUAL "freebsd")
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                              INTERFACE_LINK_LIBRARIES procstat)
      elseif(CORE_SYSTEM_NAME MATCHES "windows")
        set_property(TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} APPEND PROPERTY
                                                                              INTERFACE_LINK_LIBRARIES psapi)
      endif()
    endif()

    if(TARGET exiv2)
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} exiv2)
    endif()

    # Add internal build target when a Multi Config Generator is used
    # We cant add a dependency based off a generator expression for targeted build types,
    # https://gitlab.kitware.com/cmake/cmake/-/issues/19467
    # therefore if the find heuristics only find the library, we add the internal build
    # target to the project to allow user to manually trigger for any build type they need
    # in case only a specific build type is actually available (eg Release found, Debug Required)
    # This is mainly targeted for windows who required different runtime libs for different
    # types, and they arent compatible
    if(_multiconfig_generator)
      if(NOT TARGET exiv2)
        buildexiv2()
        set_target_properties(exiv2 PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends exiv2)
    endif()
  else()
    if(Exiv2_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find or build Exiv2 library. You may want to try -DENABLE_INTERNAL_EXIV2=ON")
    endif()
  endif()
endif()
