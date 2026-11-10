# Assembles the polyfill files like ORB's BuildPolyfill.mk does, with Kodi's native layer.
# Arguments: ORB_SOURCE_DIR, OUTPUT_DIR

file(READ ${ORB_SOURCE_DIR}/BuildPolyfill.mk _recipe)
# $(3) selects the native layer. Replacing it first also keeps its ")" out of the block ends.
string(REPLACE "$(3)" "kodi" _recipe "${_recipe}")

# Source list of one "$(eval <name> = \ ... )" block.
function(polyfill_sources name out)
  string(REGEX MATCH "\\$\\(eval ${name} = \\\\[^)]*\\)" _block "${_recipe}")
  string(REGEX MATCHALL "(src|external)/[^ \t\r\n\\\\)]+" _sources "${_block}")
  set(${out} ${_sources} PARENT_SCOPE)
endfunction()

function(concat output)
  file(WRITE ${output} "")
  foreach(_source IN LISTS ARGN)
    file(READ ${ORB_SOURCE_DIR}/${_source} _content)
    file(APPEND ${output} "${_content}")
  endforeach()
endfunction()

polyfill_sources(polyfill_hbbtv_srcs _hbbtv)
polyfill_sources(polyfill_iframe_srcs _iframe)
polyfill_sources(polyfill_dash_srcs _dash)

file(MAKE_DIRECTORY ${OUTPUT_DIR})
concat(${OUTPUT_DIR}/hbbtv.js ${_hbbtv} src/run.js src/housekeeping/endiffe.js)
concat(${OUTPUT_DIR}/iframe.js ${_iframe})
concat(${OUTPUT_DIR}/dash.all.min.js ${_dash})
file(COPY ${ORB_SOURCE_DIR}/external/playerpage.html DESTINATION ${OUTPUT_DIR})
