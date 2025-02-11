#message("zpc: plantuml: Prefer local/env one over system or detected one (${PlantUML_JARFILE})")
if(NOT DEFINED PlantUML_FOUND)
  unset(PlantUML_JARFILE CACHE)
  if(EXISTS "$ENV{PLANTUML_JAR_PATH}")
    set(PlantUML_JARFILE "$ENV{PLANTUML_JAR_PATH}")
  endif()
  if(NOT EXISTS ${PlantUML_JARFILE})
    find_file(PlantUML_JARFILE
      plantuml.jar
      PATHS
      "/usr/local/share/plantuml/"      
      "/usr/local/opt/plantuml/"
      "/opt/plantuml/"
      "/opt/"
      "/usr/share/plantuml/"
    )
  endif()
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(PlantUML DEFAULT_MSG PlantUML_JARFILE)
endif()

# message("zpc: plantuml: PlantUML_JARFILE - Path to PlantUML JAR file : ${PlantUML_JARFILE}")
# message("zpc: plantuml: PlantUML_FOUND - True if PlantUML found: : ${PlantUML_FOUND}")
