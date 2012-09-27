// Copyright (c) 2006, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// minidump_generator.h:  Create a minidump of the current MacOS process.

#ifndef CLIENT_MAC_GENERATOR_MINIDUMP_GENERATOR_H__
#define CLIENT_MAC_GENERATOR_MINIDUMP_GENERATOR_H__

#include <mach/mach.h>
#include <TargetConditionals.h>

#include <string>

#include "client/minidump_file_writer.h"
#include "common/memory.h"
#include "common/mac/macho_utilities.h"
#include "google_breakpad/common/minidump_format.h"

#include "dynamic_images.h"
#include "mach_vm_compat.h"

#if !TARGET_OS_IPHONE && (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_7)
  #define HAS_PPC_SUPPORT
#endif
#if defined(__arm__)
  #define HAS_ARM_SUPPORT
#elif defined(__i386__) || defined(__x86_64__)
  #define HAS_X86_SUPPORT
#endif

namespace google_breakpad {

using std::string;

// Use the REGISTER_FROM_THREADSTATE to access a register name from the
// breakpad_thread_state_t structure.
#if __DARWIN_UNIX03 || TARGET_CPU_X86_64 || TARGET_CPU_PPC64 || TARGET_CPU_ARM
// In The 10.5 SDK Headers Apple prepended __ to the variable names in the
// i386_thread_state_t structure.  There's no good way to tell what version of
// the SDK we're compiling against so we just toggle on the same preprocessor
// symbol Apple's headers use.
#define REGISTER_FROM_THREADSTATE(a, b) ((a)->__ ## b)
#else
#define REGISTER_FROM_THREADSTATE(a, b) (a->b)
#endif

// Creates a minidump file of the current process.  If there is exception data,
// use SetExceptionInformation() to add this to the minidump.  The minidump
// file is generated by the Write() function.
// Usage:
// MinidumpGenerator minidump();
// minidump.Write("/tmp/minidump");
//
class MinidumpGenerator {
 public:
  MinidumpGenerator();
  MinidumpGenerator(mach_port_t crashing_task, mach_port_t handler_thread);

  virtual ~MinidumpGenerator();

  // Return <dir>/<unique_name>.dmp
  // Sets |unique_name| (if requested) to the unique name for the minidump
  static string UniqueNameInDirectory(const string &dir, string *unique_name);

  // Write out the minidump into |path|
  // All of the components of |path| must exist and be writable
  // Return true if successful, false otherwise
  bool Write(const char *path);

  // Specify some exception information, if applicable
  void SetExceptionInformation(int type, int code, int subcode,
                               mach_port_t thread_name) {
    exception_type_ = type;
    exception_code_ = code;
    exception_subcode_ = subcode;
    exception_thread_ = thread_name;
  }

  // Specify the task context. If |task_context| is not NULL, it will be used
  // to retrieve the context of the current thread, instead of using
  // |thread_get_state|.
  void SetTaskContext(ucontext_t *task_context);

  // Gather system information.  This should be call at least once before using
  // the MinidumpGenerator class.
  static void GatherSystemInformation();

 protected:
  // Overridable Stream writers
  virtual bool WriteExceptionStream(MDRawDirectory *exception_stream);

  // Overridable Helper
  virtual bool WriteThreadStream(mach_port_t thread_id, MDRawThread *thread);

 private:
  typedef bool (MinidumpGenerator::*WriteStreamFN)(MDRawDirectory *);

  // Stream writers
  bool WriteThreadListStream(MDRawDirectory *thread_list_stream);
  bool WriteMemoryListStream(MDRawDirectory *memory_list_stream);
  bool WriteSystemInfoStream(MDRawDirectory *system_info_stream);
  bool WriteModuleListStream(MDRawDirectory *module_list_stream);
  bool WriteMiscInfoStream(MDRawDirectory *misc_info_stream);
  bool WriteBreakpadInfoStream(MDRawDirectory *breakpad_info_stream);

  // Helpers
  u_int64_t CurrentPCForStack(breakpad_thread_state_data_t state);
  bool GetThreadState(thread_act_t target_thread, thread_state_t state,
                      mach_msg_type_number_t *count);
  bool WriteStackFromStartAddress(mach_vm_address_t start_addr,
                                  MDMemoryDescriptor *stack_location);
  bool WriteStack(breakpad_thread_state_data_t state,
                  MDMemoryDescriptor *stack_location);
  bool WriteContext(breakpad_thread_state_data_t state,
                    MDLocationDescriptor *register_location);
  bool WriteCVRecord(MDRawModule *module, int cpu_type,
                     const char *module_path, bool in_memory);
  bool WriteModuleStream(unsigned int index, MDRawModule *module);
  size_t CalculateStackSize(mach_vm_address_t start_addr);
  int  FindExecutableModule();

  // Per-CPU implementations of these methods
#ifdef HAS_ARM_SUPPORT
  bool WriteStackARM(breakpad_thread_state_data_t state,
                     MDMemoryDescriptor *stack_location);
  bool WriteContextARM(breakpad_thread_state_data_t state,
                       MDLocationDescriptor *register_location);
  u_int64_t CurrentPCForStackARM(breakpad_thread_state_data_t state);
#endif
#ifdef HAS_PPC_SUPPORT
  bool WriteStackPPC(breakpad_thread_state_data_t state,
                     MDMemoryDescriptor *stack_location);
  bool WriteContextPPC(breakpad_thread_state_data_t state,
                       MDLocationDescriptor *register_location);
  u_int64_t CurrentPCForStackPPC(breakpad_thread_state_data_t state);
  bool WriteStackPPC64(breakpad_thread_state_data_t state,
                       MDMemoryDescriptor *stack_location);
  bool WriteContextPPC64(breakpad_thread_state_data_t state,
                       MDLocationDescriptor *register_location);
  u_int64_t CurrentPCForStackPPC64(breakpad_thread_state_data_t state);
#endif
#ifdef HAS_X86_SUPPORT
  bool WriteStackX86(breakpad_thread_state_data_t state,
                       MDMemoryDescriptor *stack_location);
  bool WriteContextX86(breakpad_thread_state_data_t state,
                       MDLocationDescriptor *register_location);
  u_int64_t CurrentPCForStackX86(breakpad_thread_state_data_t state);
  bool WriteStackX86_64(breakpad_thread_state_data_t state,
                        MDMemoryDescriptor *stack_location);
  bool WriteContextX86_64(breakpad_thread_state_data_t state,
                          MDLocationDescriptor *register_location);
  u_int64_t CurrentPCForStackX86_64(breakpad_thread_state_data_t state);
#endif

  // disallow copy ctor and operator=
  explicit MinidumpGenerator(const MinidumpGenerator &);
  void operator=(const MinidumpGenerator &);

 protected:
  // Use this writer to put the data to disk
  MinidumpFileWriter writer_;

 private:
  // Exception information
  int exception_type_;
  int exception_code_;
  int exception_subcode_;
  mach_port_t exception_thread_;
  mach_port_t crashing_task_;
  mach_port_t handler_thread_;

  // CPU type of the task being dumped.
  cpu_type_t cpu_type_;
  
  // System information
  static char build_string_[16];
  static int os_major_version_;
  static int os_minor_version_;
  static int os_build_number_;

  // Context of the task to dump.
  ucontext_t *task_context_;

  // Information about dynamically loaded code
  DynamicImages *dynamic_images_;

  // PageAllocator makes it possible to allocate memory
  // directly from the system, even while handling an exception.
  mutable PageAllocator allocator_;

 protected:
  // Blocks of memory written to the dump. These are all currently
  // written while writing the thread list stream, but saved here
  // so a memory list stream can be written afterwards.
  wasteful_vector<MDMemoryDescriptor> memory_blocks_;
};

}  // namespace google_breakpad

#endif  // CLIENT_MAC_GENERATOR_MINIDUMP_GENERATOR_H__
