#include "td/utils/base64.h"
#include "vm/cells.h"
#include "vm/cellslice.h"
#include "td/utils/tests.h"
#include "vm/boc.h"
#include "vm/dict.h"
#include "vm/fmt.hpp"

TEST(dict, main) {
  LOG(ERROR) << "test main";
  auto cell =
      vm::std_boc_deserialize(
          td::base64_decode("te6ccgEBFAEAeAABAcABAgPOQAUCAgHUBAMACQAAAI3gAAkAAACjoAIBIA0GAgEgCgcCASAJCAAJAAAAciAACQAAAI"
                            "fgAgEgDAsACQAAAFZgAAkAAABsIAIBIBEOAgEgEA8ACQAAADqgAAkAAABQYAIBIBMSAAkAAAAe4AAJAAAAv2A=")
              .move_as_ok())
          .move_as_ok();

  LOG(ERROR) << "hash " << cell->get_hash().to_hex();
  vm::Dictionary dict{cell, 32};

  LOG(ERROR) << dict.is_valid();
  auto pair = dict.begin();
  LOG(ERROR) << "it";

  // for (vm::DictIterator i = dict.begin(); i != dict.end(); ++i) {
  //     LOG(ERROR) << "key ";
  //     // LOG(ERROR) << "key: " << i.cur_pos().to_hex(32);
  // }
}