if (NOT OPENSSL_FOUND)
  find_package(OpenSSL REQUIRED)
endif()

#add_subdirectory(dummy0)
add_subdirectory(impl)

set(VALIDATOR_DB_SOURCE
  db/archiver.cpp
  db/archiver.hpp
  db/archive-manager.cpp
  db/archive-manager.hpp
  db/archive-slice.cpp
  db/archive-slice.hpp
  db/celldb.cpp
  db/celldb.hpp
  db/permanent-celldb/permanent-celldb-utils.cpp
  db/permanent-celldb/permanent-celldb-utils.h
  db/files-async.hpp
  db/fileref.hpp
  db/fileref.cpp
  db/rootdb.cpp
  db/rootdb.hpp
  db/statedb.hpp
  db/statedb.cpp
  db/staticfilesdb.cpp
  db/staticfilesdb.hpp
  db/db-utils.cpp
  db/db-utils.h

  db/package.hpp
  db/package.cpp
)

set(VALIDATOR_HEADERS
  block-handle.hpp
  get-next-key-blocks.h

  downloaders/download-state.hpp
  downloaders/wait-block-data-disk.hpp
  downloaders/wait-block-data.hpp
  downloaders/wait-block-state-merge.hpp
  downloaders/wait-block-state.hpp

  fabric.h
  interfaces/db.h
  interfaces/external-message.h
  interfaces/liteserver.h
  interfaces/out-msg-queue-proof.h
  interfaces/proof.h
  interfaces/shard.h
  interfaces/signature-set.h
  interfaces/validator-full-id.h
  interfaces/validator-manager.h
  interfaces/validator-set.h
  invariants.hpp
  
  import-db-slice.hpp
  import-db-slice-local.hpp
  queue-size-counter.hpp
  validator-telemetry.hpp
  storage-stat-cache.hpp
  shard-block-verifier.hpp
  shard-block-retainer.hpp

  collation-manager.hpp
  collator-node.hpp
  manager-disk.h
  manager-disk.hpp
  manager-init.h
  manager-init.hpp
  manager.h
  manager.hpp
  shard-client.hpp
  validate-broadcast.hpp
  validator-group.hpp
  validator-options.hpp
  validator.h
)
set(VALIDATOR_SOURCE
  apply-block.cpp
  block-handle.cpp
  collation-manager.cpp
  collator-node.cpp
  get-next-key-blocks.cpp
  import-db-slice.cpp
  import-db-slice-local.cpp
  shard-client.cpp
  state-serializer.cpp
  token-manager.cpp
  validate-broadcast.cpp
  validator-full-id.cpp
  validator-group.cpp
  validator-options.cpp
  queue-size-counter.cpp
  validator-telemetry.cpp
  storage-stat-cache.cpp
  shard-block-verifier.cpp
  shard-block-retainer.cpp

  downloaders/wait-block-data.cpp
  downloaders/wait-block-state.cpp
  downloaders/wait-block-state-merge.cpp
  downloaders/download-state.cpp

  manager-init.cpp
  manager.cpp

  ${VALIDATOR_DB_SOURCE}

  ${VALIDATOR_HEADERS}
)

set(DISK_VALIDATOR_SOURCE
  apply-block.cpp
  block-handle.cpp
  shard-client.cpp
  validator-full-id.cpp
  validator-group.cpp
  validator-options.cpp
  queue-size-counter.cpp

  downloaders/wait-block-data-disk.cpp
  downloaders/wait-block-state.cpp
  downloaders/wait-block-state-merge.cpp
  downloaders/download-state.cpp

  manager-init.cpp
  manager-disk.cpp

  ${VALIDATOR_DB_SOURCE}
)

set(HARDFORK_VALIDATOR_SOURCE
  apply-block.cpp
  block-handle.cpp
  shard-client.cpp
  validator-full-id.cpp
  validator-group.cpp
  validator-options.cpp
  queue-size-counter.cpp

  downloaders/wait-block-data-disk.cpp
  downloaders/wait-block-state.cpp
  downloaders/wait-block-state-merge.cpp
  downloaders/download-state.cpp

  manager-init.cpp
  manager-hardfork.cpp

  ${VALIDATOR_DB_SOURCE}
)

set(FULL_NODE_SOURCE
  full-node.h
  full-node.hpp
  full-node.cpp
  full-node-shard.h
  full-node-shard.hpp
  full-node-shard.cpp
  full-node-master.h
  full-node-master.hpp
  full-node-master.cpp
  full-node-private-overlay.hpp
  full-node-private-overlay.cpp
  full-node-serializer.hpp
  full-node-serializer.cpp
  full-node-fast-sync-overlays.hpp
  full-node-fast-sync-overlays.cpp

  net/download-block.hpp
  net/download-block.cpp
  net/download-block-new.hpp
  net/download-block-new.cpp
  net/download-archive-slice.hpp
  net/download-archive-slice.cpp
  net/download-next-block.hpp
  net/download-next-block.cpp
  net/download-state.hpp
  net/download-state.cpp
  net/download-proof.hpp
  net/download-proof.cpp
  net/get-next-key-blocks.hpp
  net/get-next-key-blocks.cpp
)

add_library(validator STATIC ${VALIDATOR_SOURCE})
add_library(validator-disk STATIC ${DISK_VALIDATOR_SOURCE})
add_library(validator-hardfork STATIC ${HARDFORK_VALIDATOR_SOURCE})
add_library(full-node STATIC ${FULL_NODE_SOURCE})

target_include_directories(validator PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/.. 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../crypto 
  ${OPENSSL_INCLUDE_DIR}
)

target_include_directories(validator-disk PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/.. 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../crypto 
  ${OPENSSL_INCLUDE_DIR}
  )

target_include_directories(validator-hardfork PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/.. 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../crypto 
  ${OPENSSL_INCLUDE_DIR}
  )

target_include_directories(full-node PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/.. 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../crypto 
  ${OPENSSL_INCLUDE_DIR}
  )

target_link_libraries(validator PRIVATE tdactor adnl rldp tl_api dht tdfec overlay catchain validatorsession ton_db)

target_link_libraries(validator-disk PRIVATE tdactor adnl rldp tl_api dht tdfec overlay catchain validatorsession ton_db)

target_link_libraries(validator-hardfork PRIVATE tdactor adnl rldp tl_api dht tdfec overlay catchain validatorsession ton_db)

target_link_libraries(full-node PRIVATE tdactor adnl rldp rldp2 tl_api dht tdfec overlay catchain validatorsession ton_db)
