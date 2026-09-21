file(GLOB_RECURSE CPP_SOURCE_FILES "src/*.cpp")
file(GLOB_RECURSE C_SOURCE_FILES "src/*.c")

idf_component_register(
  SRCS
  ${CPP_SOURCE_FILES}
  ${C_SOURCE_FILES}
  INCLUDE_DIRS
  "inc"
)
