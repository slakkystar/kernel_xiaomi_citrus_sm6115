#define CREATE_TRACE_POINTS
#include <trace/hooks/vendor_hooks.h>
#include <trace/hooks/memory.h>
#include <trace/hooks/syscall_check.h>
#include <trace/hooks/vh_vmscan.h>

/*
 * Export tracepoints that act as a bare tracehook (ie: have no trace event
 * associated with them) to allow external modules to probe them.
 */
EXPORT_TRACEPOINT_SYMBOL_GPL(android_vh_set_memory_x);
EXPORT_TRACEPOINT_SYMBOL_GPL(android_vh_set_memory_nx);
EXPORT_TRACEPOINT_SYMBOL_GPL(android_vh_set_memory_ro);
EXPORT_TRACEPOINT_SYMBOL_GPL(android_vh_set_memory_rw);
EXPORT_TRACEPOINT_SYMBOL_GPL(android_vh_check_mmap_file);
EXPORT_TRACEPOINT_SYMBOL_GPL(android_vh_check_file_open);
EXPORT_TRACEPOINT_SYMBOL_GPL(android_vh_check_bpf_syscall);

EXPORT_TRACEPOINT_SYMBOL_GPL(android_vh_mem_cgroup_alloc);
EXPORT_TRACEPOINT_SYMBOL_GPL(android_vh_mem_cgroup_free);
EXPORT_TRACEPOINT_SYMBOL_GPL(android_vh_mem_cgroup_id_remove);
EXPORT_TRACEPOINT_SYMBOL_GPL(android_vh_mem_cgroup_css_online);
EXPORT_TRACEPOINT_SYMBOL_GPL(android_vh_mem_cgroup_css_offline);
EXPORT_TRACEPOINT_SYMBOL_GPL(android_vh_meminfo_proc_show);
EXPORT_TRACEPOINT_SYMBOL_GPL(android_vh_tune_scan_type);
EXPORT_TRACEPOINT_SYMBOL_GPL(android_vh_alloc_pages_slowpath);
EXPORT_TRACEPOINT_SYMBOL_GPL(android_vh_rmqueue);