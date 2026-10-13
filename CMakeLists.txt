# ESP-IDF component manifest for M5Unit-EXTIO.
# Upstream: https://github.com/m5stack/M5Unit-EXTIO
#
# The public header (unit/unit_ExtIO2.hpp) includes M5UnitUnified only.
# M5Utility is included from the sources (M5Utility.hpp), so it is a private requirement.
file(GLOB_RECURSE SRCS "src/*.cpp")
idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "src"
    REQUIRES M5UnitUnified
    PRIV_REQUIRES M5Utility
)
