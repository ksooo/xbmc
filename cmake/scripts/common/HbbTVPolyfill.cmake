# Builds the HbbTV polyfill in system/hbbtv from a pinned ORB version and Kodi's patches
# in tools/depends/target/orb-polyfill, and adds the result to install_data.

include(ExternalProject)
include(${CMAKE_SOURCE_DIR}/cmake/scripts/common/ModuleHelpers.cmake)

set(_orb_dir ${CMAKE_SOURCE_DIR}/tools/depends/target/orb-polyfill)
file(STRINGS ${_orb_dir}/ORB-POLYFILL-VERSION _orb_version REGEX "^VERSION=")
file(STRINGS ${_orb_dir}/ORB-POLYFILL-VERSION _orb_base_url REGEX "^BASE_URL=")
file(STRINGS ${_orb_dir}/ORB-POLYFILL-VERSION _orb_sha512 REGEX "^SHA512=")
string(REPLACE "VERSION=" "" _orb_version "${_orb_version}")
string(REPLACE "BASE_URL=" "" _orb_base_url "${_orb_base_url}")
string(REPLACE "SHA512=" "" _orb_sha512 "${_orb_sha512}")

file(GLOB _orb_patches ${_orb_dir}/*.patch)
list(SORT _orb_patches)
generate_patchcommand("${_orb_patches}")

set(_hbbtv_files hbbtv.js iframe.js dash.all.min.js playerpage.html)
set(_hbbtv_outputs ${_hbbtv_files})
list(TRANSFORM _hbbtv_outputs PREPEND ${CMAKE_BINARY_DIR}/system/hbbtv/)

ExternalProject_Add(hbbtv-polyfill
                    URL ${_orb_base_url}/archive/${_orb_version}.tar.gz
                    URL_HASH SHA512=${_orb_sha512}
                    DOWNLOAD_NAME orb-${_orb_version}.tar.gz
                    DOWNLOAD_DIR ${TARBALL_DIR}
                    PREFIX ${CORE_BUILD_DIR}/hbbtv-polyfill
                    PATCH_COMMAND ${PATCH_COMMAND}
                    CONFIGURE_COMMAND ""
                    BUILD_COMMAND ${CMAKE_COMMAND} -DORB_SOURCE_DIR=<SOURCE_DIR>
                                                   -DOUTPUT_DIR=${CMAKE_BINARY_DIR}/system/hbbtv
                                                   -P ${CMAKE_SOURCE_DIR}/cmake/scripts/common/BuildHbbTVPolyfill.cmake
                    BUILD_BYPRODUCTS ${_hbbtv_outputs}
                    INSTALL_COMMAND "")
set_target_properties(hbbtv-polyfill PROPERTIES FOLDER "External Projects")
# Unpack and patch again when a patch changes.
ExternalProject_Add_StepDependencies(hbbtv-polyfill download ${_orb_patches})

foreach(_file IN LISTS _hbbtv_files)
  list(APPEND install_data system/hbbtv/${_file})
endforeach()

unset(_orb_dir)
unset(_orb_version)
unset(_orb_base_url)
unset(_orb_sha512)
unset(_orb_patches)
unset(_hbbtv_files)
unset(_hbbtv_outputs)
unset(PATCH_COMMAND)
