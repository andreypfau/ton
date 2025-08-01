option(TONLIBJSON_STATIC "Build tonlibjson as static library" OFF)

if (NOT OPENSSL_FOUND)
  find_package(OpenSSL REQUIRED)
endif()

set(TONLIB_SOURCE
  tonlib/Client.cpp
  tonlib/Config.cpp
  tonlib/ExtClient.cpp
  tonlib/ExtClientOutbound.cpp
  tonlib/KeyStorage.cpp
  tonlib/KeyValue.cpp
  tonlib/LastBlock.cpp
  tonlib/LastBlockStorage.cpp
  tonlib/LastConfig.cpp
  tonlib/Logging.cpp
  tonlib/TonlibClient.cpp
  tonlib/TonlibClientWrapper.cpp
  tonlib/utils.cpp

  tonlib/Client.h
  tonlib/Config.h
  tonlib/ExtClient.h
  tonlib/ExtClientOutbound.h
  tonlib/KeyStorage.h
  tonlib/KeyValue.h
  tonlib/LastBlock.h
  tonlib/LastBlockStorage.h
  tonlib/LastConfig.h
  tonlib/Logging.h
  tonlib/TonlibCallback.h
  tonlib/TonlibClient.h
  tonlib/TonlibClientWrapper.h
  tonlib/utils.h

  tonlib/keys/bip39.cpp
  tonlib/keys/DecryptedKey.cpp
  tonlib/keys/EncryptedKey.cpp
  tonlib/keys/Mnemonic.cpp
  tonlib/keys/SimpleEncryption.cpp
  tonlib/keys/bip39.h
  tonlib/keys/DecryptedKey.h
  tonlib/keys/EncryptedKey.h
  tonlib/keys/Mnemonic.h
  tonlib/keys/SimpleEncryption.h
)

set(TONLIB_OFFLINE_TEST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/test/offline.cpp PARENT_SCOPE)
set(TONLIB_ONLINE_TEST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/test/online.cpp PARENT_SCOPE)

set(USE_EMSCRIPTEN ${USE_EMSCRIPTEN} PARENT_SCOPE)

add_library(tonlib STATIC ${TONLIB_SOURCE})

target_include_directories(tonlib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/..
  $<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIR}>
)
target_link_libraries(tonlib PRIVATE tdactor adnllite tl_lite_api tl-lite-utils ton_crypto lite-client-common smc-envelope emulator_static)
target_link_libraries(tonlib PUBLIC tdutils tl_tonlib_api)

if (TONLIB_ENABLE_JNI AND NOT ANDROID) # jni is available by default on Android
  if (NOT JNI_FOUND)
    find_package(JNI REQUIRED)
  endif()
  message(STATUS "Found JNI: ${JNI_INCLUDE_DIRS} ${JNI_LIBRARIES}")
  target_include_directories(tonlib PUBLIC ${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})
  target_include_directories(tl_tonlib_api PUBLIC ${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})
  target_link_libraries(tonlib PUBLIC ${JAVA_JVM_LIBRARY})
endif()

add_executable(tonlib-cli tonlib/tonlib-cli.cpp)
target_link_libraries(tonlib-cli tonlib tdactor tdutils terminal ton_crypto git)

if (NOT CMAKE_CROSSCOMPILING)
  if (TONLIB_ENABLE_JNI)
	  #FIXME
	  #add_dependencies(tonlib tonlib_generate_java_api)
  endif()
endif()
add_library(tonlibjson_private STATIC tonlib/ClientJson.cpp tonlib/ClientJson.h)
target_include_directories(tonlibjson_private PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${TL_TD_AUTO_INCLUDES}>)
target_link_libraries(tonlibjson_private PUBLIC tonlib PRIVATE tl_tonlib_api_json)

set(TONLIB_JSON_HEADERS tonlib/tonlib_client_json.h)
set(TONLIB_JSON_SOURCE tonlib/tonlib_client_json.cpp)

include(GenerateExportHeader)
if (TONLIBJSON_STATIC OR USE_EMSCRIPTEN)
  add_library(tonlibjson STATIC ${TONLIB_JSON_SOURCE})
else()
  add_library(tonlibjson SHARED ${TONLIB_JSON_SOURCE})
endif()

if (PORTABLE AND NOT APPLE)
  target_link_libraries(tonlibjson PRIVATE tonlibjson_private -static-libgcc -static-libstdc++)
else()
  target_link_libraries(tonlibjson PRIVATE tonlibjson_private)
endif()

if (APPLE)
  set(CMAKE_MACOSX_RPATH ON)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
endif()

if (APPLE AND PORTABLE)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libstdc++")
endif()

generate_export_header(tonlibjson EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/tonlib/tonlibjson_export.h)
if (TONLIBJSON_STATIC OR USE_EMSCRIPTEN)
  target_compile_definitions(tonlibjson PUBLIC TONLIBJSON_STATIC_DEFINE)
endif()
target_include_directories(tonlibjson PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
if (APPLE)
  set_target_properties(tonlibjson PROPERTIES LINK_FLAGS "-Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/tonlibclientjson_export_list")
endif()

add_library(TonlibJson INTERFACE)
target_link_libraries(TonlibJson INTERFACE tonlibjson)

add_library(Tonlib::TonlibJson ALIAS TonlibJson)

add_library(Tonlib INTERFACE)
target_link_libraries(Tonlib INTERFACE tonlib)

add_library(Tonlib::Tonlib ALIAS Tonlib)

install(TARGETS tonlibjson TonlibJson EXPORT Tonlib
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

if (NOT TON_USE_ABSEIL)
if (WIN32)
  set(WINGETOPT_TARGET wingetopt)
endif()
install(TARGETS tdnet keys crc32c tdactor adnllite tl_api tl-utils tl_lite_api tl-lite-utils ton_crypto ton_crypto_core ton_block smc-envelope ${WINGETOPT_TARGET}
  tdutils tl_tonlib_api tonlib lite-client-common tddb_utils emulator_static Tonlib EXPORT Tonlib
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/tonlib/Client.h DESTINATION include/tonlib/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../tl/generate/auto/tl/tonlib_api.h DESTINATION include/auto/tl/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../tl/tl/TlObject.h DESTINATION include/tl/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../tdutils/td/utils/int_types.h DESTINATION include/td/utils/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../tdutils/td/utils/Slice-decl.h DESTINATION include/td/utils/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../tdutils/td/utils/Slice.h DESTINATION include/td/utils/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../tdutils/td/utils/common.h DESTINATION include/td/utils/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../tdutils/td/utils/unique_ptr.h DESTINATION include/td/utils/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../tdutils/td/utils/check.h DESTINATION include/td/utils/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../tdutils/td/utils/SharedSlice.h DESTINATION include/td/utils/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../tdutils/td/utils/port/platform.h DESTINATION include/td/utils/port)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/../tdutils/td/utils/config.h DESTINATION include/td/utils/)
endif()

install(FILES ${TONLIB_JSON_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/tonlib/tonlibjson_export.h DESTINATION include/tonlib/)

if (NOT USE_EMSCRIPTEN AND NOT TONLIBJSON_STATIC)
  install(EXPORT Tonlib
    FILE TonlibTargets.cmake
    NAMESPACE Tonlib::
    DESTINATION lib/cmake/Tonlib
  )

  # Add SOVERSION to shared libraries
  set_property(TARGET tonlibjson PROPERTY SOVERSION ${TON_VERSION})
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file("TonlibConfigVersion.cmake"
  VERSION ${TON_VERSION}
  COMPATIBILITY ExactVersion
)
install(FILES "TonlibConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/TonlibConfigVersion.cmake"
  DESTINATION lib/cmake/Tonlib
)

install(TARGETS tonlib-cli RUNTIME DESTINATION bin)
