// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/page_table.h"

namespace Common {

PageTable::PageTable() = default;

PageTable::~PageTable() = default;

void PageTable::Resize(std::size_t address_space_width_in_bits, std::size_t page_size_in_bits,
                       bool has_attribute) {
    const std::size_t num_page_table_entries{1ULL
                                             << (address_space_width_in_bits - page_size_in_bits)};
    pointers.resize(num_page_table_entries);
    backing_addr.resize(num_page_table_entries);

    if (has_attribute) {
        attributes.resize(num_page_table_entries);
    }
}

} // namespace Common
