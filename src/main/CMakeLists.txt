#   _____                     _        _     _           
#  | ____|_  _____  ___ _   _| |_ __ _| |__ | | ___  ___ 
#  |  _| \ \/ / _ \/ __| | | | __/ _` | '_ \| |/ _ \/ __|
#  | |___ >  <  __/ (__| |_| | || (_| | |_) | |  __/\__ \
#  |_____/_/\_\___|\___|\__,_|\__\__,_|_.__/|_|\___||___/
#  
if (APPLE)
  set(CMAKE_INSTALL_RPATH "@executable_path/../lib")
  # include_directories(/opt/homebrew/include)
  # link_directories(/opt/homebrew/lib)
else()
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib;/usr/local/lib")
endif()
if(WIN32)
  add_compile_options(/wd4005)
endif()



# Agent
add_executable(CNCagent cncpp-agent.cpp)

target_compile_definitions(CNCagent PRIVATE JSON_USE_GLOBAL_UDLS=0 JSON_DIAGNOSTICS=0 ENABLE_RT_SCHEDULER)
target_include_directories(CNCagent BEFORE PRIVATE ${CMAKE_SOURCE_DIR}/src/compat)
target_include_directories(CNCagent PRIVATE ${cxxopts_SOURCE_DIR}/include)
if(WIN32)
  target_compile_definitions(CNCagent PRIVATE _HAS_STD_BYTE=0)
  link_directories(${MADS_PREFIX}/lib ${MADS_PREFIX}/bin)
  target_link_libraries(CNCagent PRIVATE Ws2_32 bcrypt Secur32 Crypt32 Dnsapi IPHLPAPI MadsCore)
else()
  target_link_libraries(CNCagent PRIVATE 
    MadsCore 
    cncpp_lib         # <--- AGGIUNTO: collega la tua libreria core
    fmt::fmt          # <--- AGGIUNTO: risolve gli errori fmt::v11
    # nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
    keystroker        # <--- AGGIUNTO: per risolvere 'read_key'
  )

  add_executable(simple simple.cpp)
  target_link_libraries(simple PRIVATE MadsCore cncpp_lib fmt::fmt yaml-cpp::yaml-cpp keystroker) # nlohmann_json::nlohmann_json)
endif()

#   ___           _        _ _ 
#  |_ _|_ __  ___| |_ __ _| | |
#   | || '_ \/ __| __/ _` | | |
#   | || | | \__ \ || (_| | | |
#  |___|_| |_|___/\__\__,_|_|_|

if(MADS_INSTALL_AGENT)
  install(TARGETS CNCagent
    RUNTIME DESTINATION bin
  )
endif()