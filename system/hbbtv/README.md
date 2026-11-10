# HbbTV polyfill

Browser side of Kodi's HbbTV support, built from the ORB polyfill
(https://github.com/OpenRedButtonProject/Orb, Apache License 2.0, see LICENSE)
with a Kodi native layer that talks to the application engine through
`window.prompt('__orb_bridge__', json)`.

The files are generated at build time from the ORB version pinned in
`tools/depends/target/orb-polyfill/ORB-POLYFILL-VERSION`, with the patches next to it
applied (see `cmake/scripts/common/HbbTVPolyfill.cmake`).

* `hbbtv.js`: OIPF/HbbTV object polyfill, injected into every application document.
* `playerpage.html`, `iframe.js`: the broadband player page the polyfill loads from
  `orb://player`.
* `dash.all.min.js`: dash.js (BSD-3-Clause) as modified by ORB for the player page.
