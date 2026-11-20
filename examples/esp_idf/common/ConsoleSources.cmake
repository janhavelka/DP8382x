# Both applications own these sources. Registering them as repository component
# sources would also compile the example CLI for ordinary library consumers.
target_sources(${COMPONENT_LIB} PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/../../common/Dp8382xCli.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/Dp8382xUartConsole.cpp")
