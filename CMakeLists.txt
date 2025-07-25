cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(TON VERSION 0.5 LANGUAGES C CXX)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
#set(OPENSSL_USE_STATIC_LIBS TRUE)

# Define the two required variables before including the source code for watching a git repository.
set(PRE_CONFIGURE_FILE "git.cc.in")
set(POST_CONFIGURE_FILE "${CMAKE_CURRENT_BINARY_DIR}/git.cc")
include(git_watcher.cmake)

# Create a library out of the compiled post-configure file.
add_library(git STATIC ${POST_CONFIGURE_FILE})
target_include_directories(git PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(git check_git)

# Prevent in-source build
get_filename_component(TON_REAL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" REALPATH)
get_filename_component(TON_REAL_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}" REALPATH)

if (TON_REAL_BINARY_DIR STREQUAL TON_REAL_SOURCE_DIR)
  message("  Out-of-source build should be used to build TON.")
  message("  You need to remove the files already created by CMake and")
  message("  rerun CMake from a new directory:")
  message("  rm -rf CMakeFiles CMakeCache.txt")
  message("  mkdir build")
  message("  cd build")
  message("  cmake ..")
  message(FATAL_ERROR "In-source build failed.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

set(CMAKE_COLOR_DIAGNOSTICS TRUE)

#BEGIN internal
option(BUILD_SHARED_LIBS "Use \"ON\" to build shared libraries instead of static where it's not specified (not recommended)" OFF)
option(USE_EMSCRIPTEN "Use \"ON\" for config building wasm." OFF)
option(TON_ONLY_TONLIB "Use \"ON\" to build only tonlib." OFF)
if (USE_EMSCRIPTEN)
  set(TON_ONLY_TONLIB true)
endif()
if (TON_ONLY_TONLIB)
  set(NOT_TON_ONLY_TONLIB false)
else()
  set(NOT_TON_ONLY_TONLIB true)
endif()
option(TON_USE_ROCKSDB "Use \"ON\" to enable RocksDb." ${NOT_TON_ONLY_TONLIB})
option(TON_USE_ABSEIL "Use \"ON\" to enable Abseil." ${NOT_TON_ONLY_TONLIB})
option(TON_USE_JEMALLOC "Use \"ON\" to enable JeMalloc." OFF)
#END internal

option(TONLIB_ENABLE_JNI "Use \"ON\" to enable JNI-compatible TonLib API.")
option(TON_USE_ASAN "Use \"ON\" to enable AddressSanitizer." OFF)
option(TON_USE_TSAN "Use \"ON\" to enable ThreadSanitizer." OFF)
option(TON_USE_UBSAN "Use \"ON\" to enable UndefinedBehaviorSanitizer." OFF)
set(TON_ARCH "native" CACHE STRING "Architecture, will be passed to -march=")

option(TON_PRINT_BACKTRACE_ON_CRASH "Attempt to print a backtrace when a fatal signal is caught" ON)

option(TON_USE_LLD "Use LLD for linking" OFF)

if (TON_USE_LLD)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=lld")
endif()

#BEGIN M1 support
EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )

if ((ARCHITECTURE MATCHES "arm64") AND (CMAKE_SYSTEM_NAME STREQUAL "Darwin") AND
    (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)) # only clang 13+ supports cpu=apple-m1
    set(TON_ARCH "apple-m1")
endif()
#END M1 support

if (TON_USE_ABSEIL)
  message("Add abseil-cpp")
  set(ABSL_PROPAGATE_CXX_STD TRUE)
  add_subdirectory(third-party/abseil-cpp EXCLUDE_FROM_ALL)
  set(ABSL_FOUND 1)
endif()

#add_subdirectory(third-party/libcuckoo EXCLUDE_FROM_ALL)
#add_subdirectory(third-party/junction EXCLUDE_FROM_ALL)

if (WIN32)
  message("Add wingetopt")
  function(wingetopt_scope)
    set(CMAKE_POLICY_VERSION_MINIMUM "3.10")
    add_subdirectory(third-party/wingetopt EXCLUDE_FROM_ALL)
  endfunction()
  wingetopt_scope()
  set(WINGETOPT_FOUND 1)
endif()

set(CRC32C_BUILD_TESTS OFF CACHE BOOL "Build CRC32C's unit tests")
set(CRC32C_BUILD_BENCHMARKS OFF CACHE BOOL "Build CRC32C's benchmarks")
set(CRC32C_USE_GLOG OFF CACHE BOOL "Build CRC32C's tests with Google Logging")
set(CRC32C_INSTALL OFF CACHE BOOL "Install CRC32C's header and library")
message("Add crc32c")
function(crc32_scope)
  set(CMAKE_POLICY_VERSION_MINIMUM "3.10")
  if (NOT MSVC)
    # fix aarch64 build @ crc32c/src/crc32c_arm64_linux_check.h
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=address")
  endif()
  add_subdirectory(third-party/crc32c EXCLUDE_FROM_ALL)
endfunction()
crc32_scope()
set(CRC32C_FOUND 1)

if (TON_USE_ROCKSDB)
  if (ANDROID)
    set(PORTABLE ON CACHE BOOL "portable")
  endif()
  set(WITH_GFLAGS OFF CACHE BOOL "build with GFlags")
  set(WITH_TESTS OFF CACHE BOOL "build with tests")
  set(WITH_TOOLS OFF CACHE BOOL "build with tools")
  set(USE_RTTI ON CACHE BOOL "use rtti")
  set(FAIL_ON_WARNINGS OFF CACHE BOOL "fail on warnings")
  message("Add rocksdb")
  add_subdirectory(third-party/rocksdb EXCLUDE_FROM_ALL)
  # Broken CMake in rocksdb alters properties it has no business changing.
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK)
endif()

option(USE_COROUTINES "experimental support of coroutines" OFF)
if (USE_COROUTINES)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(TD_HAVE_COROUTINES 1)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines-ts")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  endif()
endif()

option(USE_LIBRAPTORQ "use libraptorq for tests" OFF)
if (USE_LIBRAPTORQ)
  set(USE_LZ4 OFF CACHE BOOL "use lz4")
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CLANG_STDLIB "ON") # for libraptorq
  endif()
  message("Add libraptorq")
  add_subdirectory(third-party/libraptorq EXCLUDE_FROM_ALL)
endif()

message("Add ton")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

include(BuildBLST)
include(BuildSECP256K1)

# Configure CCache if available
find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
  if (NOT DEFINED CMAKE_C_COMPILER_LAUNCHER AND NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
    message(STATUS "Using ccache")
    set(CMAKE_C_COMPILER_LAUNCHER ccache)
    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
  endif()
else()
  message(STATUS "Could NOT find ccache")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(GCC 1)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CLANG 1)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  set(INTEL 1)
elseif (NOT MSVC)
  message(FATAL_ERROR "Compiler isn't supported")
endif()

include(CheckCXXCompilerFlag)

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

if (NOT ZLIB_FOUND)
  find_package(ZLIB REQUIRED)
else()
  message(STATUS "Using zlib ${ZLIB_LIBRARIES}")
endif()


if (TON_ARCH AND NOT MSVC)
  CHECK_CXX_COMPILER_FLAG( "-march=${TON_ARCH}" COMPILER_OPT_ARCH_SUPPORTED )
  if (TON_ARCH STREQUAL "apple-m1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=${TON_ARCH}")
  elseif(COMPILER_OPT_ARCH_SUPPORTED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${TON_ARCH}")
  elseif(NOT TON_ARCH STREQUAL "native")
    message(FATAL_ERROR "Compiler doesn't support arch ${TON_ARCH}")
  endif()
endif()
if (THREADS_HAVE_PTHREAD_ARG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

if (TON_USE_JEMALLOC)
  find_package(jemalloc REQUIRED)
endif()

if (NIX)
  find_package(Secp256k1 REQUIRED)
endif()


set(MEMPROF "" CACHE STRING "Use one of \"ON\", \"FAST\" or \"SAFE\" to enable memory profiling. \
Works under macOS and Linux when compiled using glibc. \
In FAST mode stack is unwinded only using frame pointers, which may fail. \
In SAFE mode stack is unwinded using backtrace function from execinfo.h, which may be very slow. \
By default both methods are used to achieve maximum speed and accuracy")

if (CLANG OR GCC)
  if (MEMPROF)
    check_cxx_compiler_flag(-no-pie CXX_NO_PIE_FLAG)
    if (CXX_NO_PIE_FLAG)
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie")
    elseif (APPLE)
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-no_pie")
    endif()
  endif()
endif()

if (MSVC)
  if (CMAKE_CXX_FLAGS_DEBUG MATCHES "/RTC1")
    string(REPLACE "/RTC1" " " CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  endif()
  add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /W4 /wd4100 /wd4127 /wd4324 /wd4456 /wd4457 /wd4458 /wd4505 /wd4702")
elseif (CLANG OR GCC)
  if (GCC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstrong-eval-order=some")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
  if (APPLE)
    #use "-Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/export_list" for exported symbols
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fvisibility=hidden -Wl,-dead_strip,-x,-S")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -fvisibility=hidden -Wl,-dead_strip,-x,-S")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
    if (NOT USE_EMSCRIPTEN)
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,--exclude-libs,ALL")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
    if (NOT TON_USE_ASAN AND NOT TON_USE_TSAN AND NOT MEMPROF)
      if (NOT USE_EMSCRIPTEN)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--exclude-libs,ALL")
      endif()
    endif()
  endif()
endif()

if (WIN32)
  add_definitions(-DNTDDI_VERSION=0x06020000 -DWINVER=0x0602 -D_WIN32_WINNT=0x0602 -DNOMINMAX -DUNICODE -D_UNICODE)
endif()
if (CYGWIN)
  add_definitions(-D_DEFAULT_SOURCE=1 -DFD_SETSIZE=4096)
endif()

if (NOT ANDROID) # _FILE_OFFSET_BITS is broken in ndk r15 and r15b and doesn't work prior to Android 7.0
  add_definitions(-D_FILE_OFFSET_BITS=64)
endif()

set(INTERNAL_COMPILE "0")
#BEGIN internal
  add_definitions(-D_INTERNAL_COMPILE=1)
  set(INTERNAL_COMPILE "1")
#END internal

set(TONLIB_COMPILE "0")
#BEGIN tonlib
  add_definitions(-D_TONLIB_COMPILE=1)
  set(TONLIB_COMPILE "1")
#END tonlib

include(AddCXXCompilerFlag)
if (MSVC)
  add_cxx_compiler_flag("/experimental:external /external:anglebrackets /external:W0")
endif()
if (NOT MSVC)
  add_cxx_compiler_flag("-Wall")
  add_cxx_compiler_flag("-Wextra")
endif()

add_cxx_compiler_flag("-Wimplicit-fallthrough=2")
add_cxx_compiler_flag("-Wpointer-arith")
add_cxx_compiler_flag("-Wcast-qual")
add_cxx_compiler_flag("-Wsign-compare")
add_cxx_compiler_flag("-Wduplicated-branches")
add_cxx_compiler_flag("-Wduplicated-cond")
add_cxx_compiler_flag("-Walloc-zero")
add_cxx_compiler_flag("-Wlogical-op")
add_cxx_compiler_flag("-Wno-tautological-compare")
add_cxx_compiler_flag("-Wpointer-arith")
add_cxx_compiler_flag("-Wvla")
add_cxx_compiler_flag("-Wnon-virtual-dtor")
add_cxx_compiler_flag("-Wno-unused-parameter")
add_cxx_compiler_flag("-Wconversion")
add_cxx_compiler_flag("-Wno-sign-conversion")
add_cxx_compiler_flag("-Qunused-arguments")
add_cxx_compiler_flag("-Wno-unused-private-field")
add_cxx_compiler_flag("-Wno-redundant-move")

if (GCC OR CLANG)
  if (CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    # For historical reasons, CMake falls back to -O2 optimization level when CMAKE_BUILD_TYPE is
    # set to RelWithDebInfo.
    add_compile_options(-O3)
  endif()
endif()

#add_cxx_compiler_flag("-Wno-unused-function")
#add_cxx_compiler_flag("-Wno-unused-variable")
#add_cxx_compiler_flag("-Wno-shorten-64-to-32")
#add_cxx_compiler_flag("-Werror")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem /usr/include/c++/v1")
if (CLANG)
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()
if (TON_USE_ASAN)
  add_cxx_compiler_flag("-fsanitize=address")
  add_definitions(-DTD_USE_ASAN=1)
endif()
if (TON_USE_TSAN)
  add_cxx_compiler_flag("-fsanitize=thread")
endif()
if (TON_USE_UBSAN)
  add_cxx_compiler_flag("-fsanitize=undefined")
endif()
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finstrument-functions")

#Compilation database
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

#BEGIN internal
find_package(LATEX)
if (LATEX_FOUND)
  include(UseLATEX)
  add_latex_document(doc/ton.tex TARGET_NAME ton_white_paper)
  add_latex_document(doc/tvm.tex TARGET_NAME ton_vm_description)
  add_latex_document(doc/tblkch.tex TARGET_NAME ton_blockchain_description)
  add_latex_document(doc/fiftbase.tex TARGET_NAME fift_basic_description)
  add_latex_document(doc/catchain.tex TARGET_NAME catchain_consensus_description)
endif()
if (NOT LATEX_FOUND)
  message(STATUS "Could NOT find LATEX (this is NOT an error)")
endif()
#END internal

function(target_link_libraries_system target)
  set(libs ${ARGN})
  foreach(lib ${libs})
    get_target_property(lib_include_dirs ${lib} INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(${target} SYSTEM PUBLIC ${lib_include_dirs})
    target_link_libraries(${target} PUBLIC ${lib})
  endforeach(lib)
endfunction(target_link_libraries_system)

set(TDUTILS_MIME_TYPE OFF CACHE BOOL "Generate mime type conversion")
add_subdirectory(tdutils)
add_subdirectory(memprof)
add_subdirectory(tdactor)
add_subdirectory(tdnet)
if (TON_USE_ROCKSDB)
  option(TDDB_USE_ROCKSDB "Use rockdb" ON)
endif()
add_subdirectory(tddb)
add_subdirectory(tdtl)
add_subdirectory(tl)
add_subdirectory(terminal)
add_subdirectory(keys)
add_subdirectory(tl-utils)
add_subdirectory(adnl)
add_subdirectory(crypto)
add_subdirectory(lite-client)
add_subdirectory(emulator)
add_subdirectory(tolk)

#BEGIN tonlib
add_subdirectory(tonlib)
#END tonlib

#BEGIN internal
if (NOT TON_ONLY_TONLIB)
add_subdirectory(common)
add_subdirectory(tdfec)
add_subdirectory(keyring)
add_subdirectory(fec)
add_subdirectory(rldp)
add_subdirectory(rldp2)
add_subdirectory(dht)
add_subdirectory(overlay)
add_subdirectory(catchain)
add_subdirectory(validator-session)
add_subdirectory(validator)
add_subdirectory(blockchain-explorer)
add_subdirectory(storage)
add_subdirectory(validator-engine)
add_subdirectory(validator-engine-console)
add_subdirectory(create-hardfork)
add_subdirectory(dht-server)
add_subdirectory(utils)
add_subdirectory(http)
add_subdirectory(rldp-http-proxy)
endif()
#END internal

if (NOT CMAKE_CROSSCOMPILING)
  if (TDUTILS_MIME_TYPE)
    set(TDMIME_AUTO tdmime_auto)
  endif()
  add_custom_target(prepare_cross_compiling DEPENDS tl_generate_common tlb_generate_block gen_fif ${TDMIME_AUTO})
endif()

#TESTS
add_executable(test-ed25519 test/test-td-main.cpp ${ED25519_TEST_SOURCE})
target_link_libraries(test-ed25519 PRIVATE ton_crypto)

add_executable(test-vm test/test-td-main.cpp ${TONVM_TEST_SOURCE})
target_link_libraries(test-vm PRIVATE ton_crypto fift-lib)

add_executable(test-smartcont test/test-td-main.cpp ${SMARTCONT_TEST_SOURCE})
target_link_libraries(test-smartcont PRIVATE smc-envelope fift-lib ton_db)

add_executable(test-bigint ${BIGINT_TEST_SOURCE})
target_link_libraries(test-bigint PRIVATE ton_crypto)

if (WINGETOPT_FOUND)
  target_link_libraries_system(test-bigint wingetopt)
endif()

add_executable(test-cells test/test-td-main.cpp ${CELLS_TEST_SOURCE})
target_link_libraries(test-cells PRIVATE ton_crypto)

add_executable(test-fift test/test-td-main.cpp ${FIFT_TEST_SOURCE})
target_link_libraries(test-fift PRIVATE fift-lib)

add_executable(test-tdutils test/test-td-main.cpp ${TDUTILS_TEST_SOURCE})
target_link_libraries(test-tdutils PRIVATE tdutils ${CMAKE_THREAD_LIBS_INIT} memprof ${JEMALLOC_LIBRARIES})
#target_link_libraries_system(test-tdutils absl::base absl::container absl::hash )
#target_link_libraries_system(test-tdutils libcuckoo)
#target_include_directories(test-tdutils PRIVATE SYSTEM ${JUNCTION_ALL_INCLUDE_DIRS})
#target_link_libraries(test-tdutils PRIVATE ${JUNCTION_ALL_LIBRARIES})

add_executable(test-tdactor test/test-td-main.cpp ${TDACTOR_TEST_SOURCE})
target_link_libraries(test-tdactor PRIVATE tdactor ${CMAKE_THREAD_LIBS_INIT})

add_executable(test-net test/test-td-main.cpp ${NET_TEST_SOURCE})
target_link_libraries(test-net PRIVATE tdnet tdutils ${CMAKE_THREAD_LIBS_INIT})

#BEGIN tonlib
add_executable(test-tonlib ${TONLIB_ONLINE_TEST_SOURCE})
target_link_libraries(test-tonlib tdactor adnllite tl_api ton_crypto tl_tonlib_api tonlib)

add_executable(test-tonlib-offline test/test-td-main.cpp ${TONLIB_OFFLINE_TEST_SOURCE})
target_link_libraries(test-tonlib-offline tdactor adnllite tl_api ton_crypto fift-lib tl_tonlib_api tonlib)

if (NOT CMAKE_CROSSCOMPILING)
  add_dependencies(test-tonlib-offline gen_fif)
endif()
#END tonlib

#BEGIN internal
if (NOT TON_ONLY_TONLIB)
add_executable(test-db test/test-td-main.cpp ${TONDB_TEST_SOURCE})
target_link_libraries(test-db PRIVATE ton_db memprof tdfec)

add_executable(test-storage test/test-td-main.cpp ${STORAGE_TEST_SOURCE})
target_link_libraries(test-storage PRIVATE storage ton_db memprof tl_api tl-utils fec rldp2)

add_executable(test-rocksdb test/test-rocksdb.cpp)
target_link_libraries(test-rocksdb PRIVATE memprof tddb tdutils)

add_executable(test-tddb test/test-td-main.cpp ${TDDB_TEST_SOURCE})
target_link_libraries(test-tddb PRIVATE tdutils tddb ${CMAKE_THREAD_LIBS_INIT} memprof)

add_executable(test-fec test/test-td-main.cpp ${FEC_TEST_SOURCE})
target_link_libraries(test-fec PRIVATE tdfec tdutils ${CMAKE_THREAD_LIBS_INIT})
if (USE_LIBRAPTORQ)
  target_link_libraries(test-fec PRIVATE third_party_fec)
  target_compile_definitions(test-fec PRIVATE "USE_LIBRAPTORQ=1")
endif()

add_executable(test-hello-world test/test-hello-world.cpp )
target_link_libraries(test-hello-world tl_api ton_crypto)

add_executable(test-adnl test/test-adnl.cpp)
target_link_libraries(test-adnl adnl adnltest dht tl_api)
add_executable(test-dht test/test-dht.cpp)
target_link_libraries(test-dht adnl adnltest dht tl_api)
add_executable(test-rldp test/test-rldp.cpp)
target_link_libraries(test-rldp adnl adnltest dht rldp tl_api)
add_executable(test-rldp2 test/test-rldp2.cpp)
target_link_libraries(test-rldp2 adnl adnltest dht rldp2 tl_api)
add_executable(test-validator-session-state test/test-validator-session-state.cpp)
target_link_libraries(test-validator-session-state adnl dht rldp validatorsession tl_api)

add_executable(test-overlay test/test-overlay.cpp)
target_link_libraries(test-overlay overlay tdutils tdactor adnl adnltest tl_api dht )
add_executable(test-catchain test/test-catchain.cpp)
target_link_libraries(test-catchain overlay tdutils tdactor adnl adnltest rldp tl_api dht
  catchain )
add_executable(test-ton-collator test/test-ton-collator.cpp)
target_link_libraries(test-ton-collator overlay tdutils tdactor adnl tl_api dht
  catchain validatorsession validator-disk ton_validator validator-disk )

add_executable(test-http test/test-http.cpp)
target_link_libraries(test-http PRIVATE tonhttp)

add_executable(test-emulator test/test-td-main.cpp emulator/test/emulator-tests.cpp)
target_link_libraries(test-emulator PRIVATE emulator)

get_directory_property(HAS_PARENT PARENT_DIRECTORY)
if (HAS_PARENT)
  set(ALL_TEST_SOURCE
          ${TDUTILS_TEST_SOURCE}
          ${TDACTOR_TEST_SOURCE}
          ${NET_TEST_SOURCE}
          ${TDDB_TEST_SOURCE}
          ${FEC_TEST_SOURCE}
          ${ED25519_TEST_SOURCE}
          ${TONDB_TEST_SOURCE}
          ${BIGNUM_TEST_SOURCE}
          ${CELLS_TEST_SOURCE} # ${TONVM_TEST_SOURCE} ${FIFT_TEST_SOURCE} ${TONLIB_ONLINE_TEST_SOURCE}
          PARENT_SCOPE)
endif()
add_library(all_tests INTERFACE)
target_link_libraries(all_tests INTERFACE tdutils tdactor tdnet tdfec ton_db ton_crypto fift-lib)
endif()
#END internal

enable_testing()
set(TEST_OPTIONS "--regression ${CMAKE_CURRENT_SOURCE_DIR}/test/regression-tests.ans --filter -Bench")
separate_arguments(TEST_OPTIONS)
add_test(test-ed25519 test-ed25519)
add_test(test-bigint test-bigint)
add_test(test-vm test-vm ${TEST_OPTIONS})
add_test(test-fift test-fift ${TEST_OPTIONS})
add_test(test-cells test-cells ${TEST_OPTIONS})
add_test(test-smartcont test-smartcont)
add_test(test-net test-net)
add_test(test-actors test-tdactor)
add_test(test-emulator test-emulator)

#BEGIN tonlib
add_test(test-tdutils test-tdutils)
add_test(test-tonlib-offline test-tonlib-offline)
#END tonlib

# FunC tests
if (NOT NIX)
    if (MSVC)
      set(PYTHON_VER "python")
    else()
      set(PYTHON_VER "python3")
    endif()
    add_test(
      NAME test-func
      COMMAND ${PYTHON_VER} run_tests.py tests/
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/crypto/func/auto-tests)
    if (WIN32)
        set_property(TEST test-func PROPERTY ENVIRONMENT
          "FUNC_EXECUTABLE=${CMAKE_CURRENT_BINARY_DIR}/crypto/func.exe"
          "FIFT_EXECUTABLE=${CMAKE_CURRENT_BINARY_DIR}/crypto/fift.exe"
          "FIFTPATH=${CMAKE_CURRENT_SOURCE_DIR}/crypto/fift/lib/")
    else()
        set_property(TEST test-func PROPERTY ENVIRONMENT
          "FUNC_EXECUTABLE=${CMAKE_CURRENT_BINARY_DIR}/crypto/func"
          "FIFT_EXECUTABLE=${CMAKE_CURRENT_BINARY_DIR}/crypto/fift"
          "FIFTPATH=${CMAKE_CURRENT_SOURCE_DIR}/crypto/fift/lib/")
    endif()

    add_test(
      NAME test-func-legacy
      COMMAND ${PYTHON_VER} legacy_tester.py
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/crypto/func/auto-tests)
    if (WIN32)
        set_property(TEST test-func-legacy PROPERTY ENVIRONMENT
          "FUNC_EXECUTABLE=${CMAKE_CURRENT_BINARY_DIR}/crypto/func.exe"
          "FIFT_EXECUTABLE=${CMAKE_CURRENT_BINARY_DIR}/crypto/fift.exe"
          "FIFTPATH=${CMAKE_CURRENT_SOURCE_DIR}/crypto/fift/lib/")
    else()
        set_property(TEST test-func-legacy PROPERTY ENVIRONMENT
          "FUNC_EXECUTABLE=${CMAKE_CURRENT_BINARY_DIR}/crypto/func"
          "FIFT_EXECUTABLE=${CMAKE_CURRENT_BINARY_DIR}/crypto/fift"
          "FIFTPATH=${CMAKE_CURRENT_SOURCE_DIR}/crypto/fift/lib/")
    endif()
endif()

# Tolk tests
if (NOT NIX)
    if (MSVC)
      set(PYTHON_VER "python")
    else()
      set(PYTHON_VER "python3")
    endif()
    add_test(
      NAME test-tolk
      COMMAND ${PYTHON_VER} tolk-tester.py tests/
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tolk-tester)
    if (WIN32)
        set_property(TEST test-tolk PROPERTY ENVIRONMENT
          "TOLK_EXECUTABLE=${CMAKE_CURRENT_BINARY_DIR}/tolk/tolk.exe"
          "FIFT_EXECUTABLE=${CMAKE_CURRENT_BINARY_DIR}/crypto/fift.exe"
          "FIFTPATH=${CMAKE_CURRENT_SOURCE_DIR}/crypto/fift/lib/")
    else()
        set_property(TEST test-tolk PROPERTY ENVIRONMENT
          "TOLK_EXECUTABLE=${CMAKE_CURRENT_BINARY_DIR}/tolk/tolk"
          "FIFT_EXECUTABLE=${CMAKE_CURRENT_BINARY_DIR}/crypto/fift"
          "FIFTPATH=${CMAKE_CURRENT_SOURCE_DIR}/crypto/fift/lib/")
    endif()
endif()

#BEGIN internal
if (NOT TON_ONLY_TONLIB)
add_test(test-adnl test-adnl)
add_test(test-dht test-dht)
add_test(test-rldp test-rldp)
add_test(test-rldp2 test-rldp2)
add_test(test-validator-session-state test-validator-session-state)
add_test(test-catchain test-catchain)

add_test(test-fec test-fec)
add_test(test-tddb test-tddb ${TEST_OPTIONS})
add_test(test-db test-db ${TEST_OPTIONS})
endif()
#END internal
