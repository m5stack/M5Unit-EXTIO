# ESP-IDF component manifest for M5Unit-EXTIO.
# Upstream: https://github.com/m5stack/M5Unit-EXTIO
#
# The public header (unit/unit_ExtIO2.hpp) pulls in both M5UnitUnified and M5Utility
# (m5_utility/container/circular_buffer.hpp).
file(GLOB_RECURSE SRCS "src/*.cpp")
idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "src"
    REQUIRES M5UnitUnified M5Utility
)
