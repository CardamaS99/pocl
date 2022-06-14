
#include "TTASimControlRegion.h"
#include "TTASimDevice.h"

#include "accel-shared.h"

#include <SimpleSimulatorFrontend.hh>
//#include <SimulatorFrontend.hh>
#include <AddressSpace.hh>
#include <Machine.hh>

#include <math.h>

//#define ACCEL_TTASIM_DEBUG

TTASimControlRegion::TTASimControlRegion(const TTAMachine::Machine& mach, TTASimDevice* parent) {

#ifdef ACCEL_TTASIM_DEBUG
  POCL_MSG_PRINT_INFO("TTASim: Initializing TTASimControlRegion\n");
#endif
  PhysAddress = 0;
  Size = ACCEL_DEFAULT_CTRL_SIZE;
  parent_ = parent;
  assert(parent_ != nullptr &&
         "simulator parent handle NULL, is the sim opened properly?");

  setupControlRegisters(mach);
}

uint32_t TTASimControlRegion::Read32(size_t offset) {

#ifdef ACCEL_TTASIM_DEBUG
  POCL_MSG_PRINT_INFO("MMAP: Reading from physical address 0x%zx with "
                      "offset 0x%zx\n",
                      PhysAddress, offset);
#endif
  assert(offset < Size && "Attempt to access data outside MMAP'd buffer");
  auto value = ControlRegisters_[offset / sizeof(uint32_t)];
  return value;
}

void TTASimControlRegion::Write32(size_t offset, uint32_t value) {
#ifdef ACCEL_TTASIM_DEBUG
  POCL_MSG_PRINT_INFO("MMAP: Writing to physical address 0x%zx with "
                      "offset 0x%zx\n",
                      PhysAddress, offset);
#endif

  if (offset == ACCEL_CONTROL_REG_COMMAND) {
    switch(value) {
    case ACCEL_RESET_CMD:
      POCL_MSG_PRINT_INFO("parent %p",parent_);
      parent_->simulator_->stop();
      break;
    case ACCEL_CONTINUE_CMD:
      parent_->restartProgram();
      break;
    }
  }
}

void TTASimControlRegion::Write16(size_t offset, uint16_t value) {
  POCL_ABORT("Unimplemented 16bit writes to ttasimcontrolregion\n");
}

uint64_t TTASimControlRegion::Read64(size_t offset) {

#ifdef ACCEL_TTASIM_DEBUG
  POCL_MSG_PRINT_INFO("MMAP: Reading from physical address 0x%zx with "
                      "offset 0x%zx\n",
                      PhysAddress, offset);
#endif
  assert(offset < Size && "Attempt to access data outside MMAP'd buffer");
  auto value =
      reinterpret_cast<uint64_t *>(ControlRegisters_)[offset / sizeof(uint64_t)];
  return value;
}


void TTASimControlRegion::CopyToMMAP(size_t destination, const void *source,
                            size_t bytes) {
  POCL_ABORT("Unimplemented copytommap for ttasimcontrolregion\n");
}

void TTASimControlRegion::CopyFromMMAP(void *destination, size_t source, size_t bytes) {


  POCL_ABORT("Unimplemented copyfrommmap for ttasimcontrolregion\n");
}

void TTASimControlRegion::CopyInMem (size_t source, size_t destination, size_t bytes) {

  POCL_ABORT("Unimplemented copyinmem for ttasimcontrolregion\n");
}

void TTASimControlRegion::setupControlRegisters(const TTAMachine::Machine& mach)
{
  bool hasPrivateMem = false;
  bool sharedDataAndCq = false;
  bool relativeAddressing = true;
  int dmem_size = 0;
  int cq_size = 0;
  int imem_size = 0;
  const TTAMachine::Machine::AddressSpaceNavigator& nav =
    mach.addressSpaceNavigator();
  for (int i = 0; i < nav.count(); i++){
    TTAMachine::AddressSpace *as = nav.item(i);
    if (as->hasNumericalId(TTA_ASID_GLOBAL)) {
      if (as->end() == UINT32_MAX) {
        dmem_size = pow(2, 12); // TODO magic number from almaifintegrator.cc
                                // (this one is from aamudsp)
        relativeAddressing = false;
      } else {
        dmem_size = as->end() + 1;
      }
      if (as->hasNumericalId(TTA_ASID_LOCAL)) {
        sharedDataAndCq = true;
      }
    }
    else if (as->hasNumericalId(TTA_ASID_LOCAL)) {
      cq_size = as->end() + 1;
    }
    else if (as->hasNumericalId(TTA_ASID_PRIVATE)) {
      hasPrivateMem = true;
    } else if (as->name() == "instructions") {

      imem_size = (as->end() + 1) * as->width();
    }
  }

  int segment_size = dmem_size > imem_size ? dmem_size : imem_size;

  int dmem_start, cq_start;
  if (relativeAddressing) {
    dmem_start = 0;
    cq_start = 0;
  } else {
    cq_start = 2*segment_size;
    dmem_start = 3*segment_size;
          POCL_MSG_PRINT_INFO("segsize=%d, dmem_start=%d\n",segment_size,dmem_start);
  }

  if (!hasPrivateMem) {
    //No private mem, so the latter half of the dmem is reserved for it
    int fallback_mem_size = pocl_get_int_option("POCL_ACCEL_PRIVATE_MEM_SIZE",1024);
    dmem_size -= fallback_mem_size;
    POCL_MSG_PRINT_INFO("Accel: No separate private mem found. Setting it to %d\n",fallback_mem_size);
  }
  if(sharedDataAndCq) {
    //No separate Cq so reserve small slice of dmem for it
    cq_size = 4 * AQL_PACKET_LENGTH;
    dmem_size -= cq_size;
    cq_start = dmem_start + dmem_size;
  }

  int imem_start = 0; 

  if (!relativeAddressing) {
    unsigned default_baseaddress = 0x43C00000; //TODO get from env variable
    cq_start += default_baseaddress;
    dmem_start += default_baseaddress;
  }

  memset(ControlRegisters_, 0, ACCEL_DEFAULT_CTRL_SIZE);

  ControlRegisters_[ACCEL_INFO_DEV_CLASS / 4] = 0;
  ControlRegisters_[ACCEL_INFO_DEV_ID / 4] = 0;
  ControlRegisters_[ACCEL_INFO_IF_TYPE / 4] = 3;
  ControlRegisters_[ACCEL_INFO_CORE_COUNT / 4] = mach.coreCount();
  ControlRegisters_[ACCEL_INFO_CTRL_SIZE / 4] = 1024;
  ControlRegisters_[ACCEL_INFO_IMEM_SIZE / 4] = imem_size;
  ControlRegisters_[ACCEL_INFO_IMEM_START_LOW / 4] = imem_start;
  ControlRegisters_[ACCEL_INFO_CQMEM_SIZE_LOW / 4] = cq_size;
  ControlRegisters_[ACCEL_INFO_CQMEM_START_LOW / 4] = cq_start;
  ControlRegisters_[ACCEL_INFO_DMEM_SIZE_LOW / 4] = dmem_size;
  ControlRegisters_[ACCEL_INFO_DMEM_START_LOW / 4] = dmem_start;
  ControlRegisters_[ACCEL_INFO_FEATURE_FLAGS_LOW / 4] = (relativeAddressing) ? 0 : 1;
  ControlRegisters_[ACCEL_INFO_PTR_SIZE / 4] = 4;
  
}

