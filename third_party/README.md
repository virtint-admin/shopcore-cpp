# Vendored third-party code

| Component | Version | Source | SHA-256 of source archive |
|---|---|---|---|
| GoogleTest (`googletest/`) | 1.15.2 | https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz | `7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926` |

Only the `googletest/` sub-project, the top-level `CMakeLists.txt` and `LICENSE` are kept
(googlemock, docs, samples and GoogleTest's own tests were removed). The build configures it
with `BUILD_GMOCK=OFF` and `INSTALL_GTEST=OFF`, so no network access is ever needed.

Do not edit files in this directory; upgrade by replacing the whole tree.
