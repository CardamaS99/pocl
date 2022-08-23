

#include "TTASimRegion.h"

#include <Memory.hh>

#include "pocl_util.h"

#include <assert.h>

TTASimRegion::TTASimRegion(size_t Address, size_t RegionSize,
                           MemorySystem::MemoryPtr mem) {

  POCL_MSG_PRINT_ACCEL_MMAP(
      "TTASim: Initializing TTASimRegion with Address %zu "
      "and Size %zu and memptr %p\n",
      Address, RegionSize, mem.get());
  PhysAddress = Address;
  Size = RegionSize;
  mem_ = mem;
  assert(mem != nullptr && "memory handle NULL, is the sim opened properly?");
}

uint32_t TTASimRegion::Read32(size_t offset) {

  POCL_MSG_PRINT_ACCEL_MMAP("TTASim: Reading from physical address 0x%zx with "
                            "offset 0x%zx\n",
                            PhysAddress, offset);
  assert(mem_ != nullptr && "No memory handle; read before mapping?");
  assert(offset < Size && "Attempt to access data outside MMAP'd buffer");

  uint64_t result = 0;
  mem_->read(PhysAddress + offset, 4, result);
  return result;
}

void TTASimRegion::Write32(size_t offset, uint32_t value) {

  POCL_MSG_PRINT_ACCEL_MMAP("TTASim: Writing to physical address 0x%zx with "
                            "offset 0x%zx\n",
                            PhysAddress, offset);
  assert(mem_ != nullptr && "No memory handle; write before mapping?");
  assert(offset < Size && "Attempt to access data outside MMAP'd buffer");
  mem_->writeDirectlyLE(PhysAddress + offset, 4, value);
}

void TTASimRegion::Write16(size_t offset, uint16_t value) {
  POCL_MSG_PRINT_ACCEL_MMAP("TTASim: Writing to physical address 0x%zx with "
                            "offset 0x%zx\n",
                            PhysAddress, offset);
  assert(mem_ != nullptr && "No memory handle; write before mapping?");
  assert(offset < Size && "Attempt to access data outside MMAP'd buffer");

  mem_->writeDirectlyLE(PhysAddress + offset, 2, value);
}

uint64_t TTASimRegion::Read64(size_t offset) {
  POCL_MSG_PRINT_ACCEL_MMAP("TTASim: Reading from physical address 0x%zx with "
                            "offset 0x%zx\n",
                            PhysAddress, offset);

  assert(mem_ != nullptr && "No memory handle; write before mapping?");
  assert(offset < Size && "Attempt to access data outside MMAP'd buffer");

  uint64_t result = 0;
  mem_->read(PhysAddress + offset, 8, result);
  return result;
}

void TTASimRegion::CopyToMMAP(size_t destination, const void *source,
                              size_t bytes) {
  POCL_MSG_PRINT_ACCEL_MMAP(
      "TTASim: Writing 0x%zx bytes to buffer at 0x%zx with "
      "address 0x%zx\n",
      bytes, PhysAddress, destination);
  auto src = (uint8_t *)source;
  size_t offset = destination - PhysAddress;
  assert(offset < Size && "Attempt to access data outside TTASim Region");

  for (size_t i = 0; i < bytes; ++i) {
    mem_->writeDirectlyLE(destination + i, 1, (Memory::MAU)src[i]);
  }
}

void TTASimRegion::CopyFromMMAP(void *destination, size_t source,
                                size_t bytes) {
  POCL_MSG_PRINT_ACCEL_MMAP("TTASim: Reading 0x%zx bytes from buffer at 0x%zx "
                            "with address 0x%zx\n",
                            bytes, PhysAddress, source);
  auto dst = (uint8_t *)destination;
  size_t offset = source - PhysAddress;
  assert(offset < Size && "Attempt to access data outside TTASim Region");

  for (size_t i = 0; i < bytes; ++i) {
    dst[i] = mem_->read(source + i);
  }
}

void TTASimRegion::CopyInMem(size_t source, size_t destination, size_t bytes) {
  POCL_MSG_PRINT_ACCEL_MMAP("TTASim: Copying 0x%zx bytes from 0x%zx "
                            "to 0x%zx\n",
                            bytes, source, destination);
  size_t src_offset = source - PhysAddress;
  size_t dst_offset = destination - PhysAddress;
  assert(src_offset < Size && (src_offset + bytes) <= Size &&
         "Attempt to access data outside TTASim Region");
  assert(dst_offset < Size && (dst_offset + bytes) <= Size &&
         "Attempt to access data outside TTASim Region");
  for (size_t i = 0; i < bytes; ++i) {
    Memory::MAU m = mem_->read(source + i);
    mem_->writeDirectlyLE(destination + i, 1, m);
  }
}
