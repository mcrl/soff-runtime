#ifndef __CL_EXT_SNUCL_H
#define __CL_EXT_SNUCL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cl_file *        cl_file;
typedef cl_uint                          cl_file_info;
typedef cl_bitfield                      cl_file_open_flags;

/* cl_file_open_flags - bitfield */
#define CL_FILE_OPEN_WRITE_ONLY                     (1 << 0)
#define CL_FILE_OPEN_READ_ONLY                      (1 << 1)
#define CL_FILE_OPEN_READ_WRITE                     (1 << 2)
#define CL_FILE_OPEN_CREATE                         (1 << 4)

/* cl_channel_type */
/* CL_UNORM_INT24 is not used for collective communication extensions */
#define CL_DOUBLE                              0x10DF

/* cl_command_type */
/* collective communication extensions */
#define CL_COMMAND_ALLTOALL_BUFFER             0x1300
#define CL_COMMAND_BROADCAST_BUFFER            0x1301
#define CL_COMMAND_SCATTER_BUFFER              0x1302
#define CL_COMMAND_GATHER_BUFFER               0x1303
#define CL_COMMAND_ALLGATHER_BUFFER            0x1304
#define CL_COMMAND_REDUCE_BUFFER               0x1305
#define CL_COMMAND_ALLREDUCE_BUFFER            0x1306
#define CL_COMMAND_REDUCESCATTER_BUFFER        0x1307
#define CL_COMMAND_SCAN_BUFFER                 0x1308

/* file I/O extensions */
#define CL_COMMAND_LOCAL_FILE_OPEN             0x1310
#define CL_COMMAND_COPY_BUFFER_TO_FILE         0x1311
#define CL_COMMAND_COPY_FILE_TO_BUFFER         0x1312

/* Collective Communication APIs */
extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueAlltoAllBuffer(cl_command_queue * cmd_queue_list,
                        cl_uint num_buffers,
                        cl_mem * src_buffer_list,
                        cl_mem * dst_buffer_list,
                        size_t * src_offset,
                        size_t * dst_offset,
                        size_t cb,
                        cl_uint num_events_in_wait_list,
                        const cl_event * event_wait_list,
                        cl_event * event_list);

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueBroadcastBuffer(cl_command_queue * cmd_queue_list,
                         cl_mem src_buffer,
                         cl_uint num_dst_buffers,
                         cl_mem * dst_buffer_list,
                         size_t src_offset,
                         size_t * dst_offset,
                         size_t cb,
                         cl_uint num_events_in_wait_list,
                         const cl_event * event_wait_list,
                         cl_event * event_list);

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueScatterBuffer(cl_command_queue * cmd_queue_list,
                       cl_mem src_buffer,
                       cl_uint num_dst_buffers,
                       cl_mem * dst_buffer_list,
                       size_t src_offset,
                       size_t * dst_offset,
                       size_t cb, // size sent to each dst buffer
                       cl_uint num_events_in_wait_list,
                       const cl_event * event_wait_list,
                       cl_event * event_list);

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueGatherBuffer(cl_command_queue cmd_queue,
                      cl_uint num_src_buffers,
                      cl_mem * src_buffer_list,
                      cl_mem dst_buffer,
                      size_t * src_offset,
                      size_t dst_offset,
                      size_t cb, // size for any single receive
                      cl_uint num_events_in_wait_list,
                      const cl_event * event_wait_list,
                      cl_event * event);

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueAllGatherBuffer(cl_command_queue * cmd_queue_list,
                         cl_uint num_buffers,
                         cl_mem * src_buffer_list,
                         cl_mem * dst_buffer_list,
                         size_t * src_offset,
                         size_t * dst_offset,
                         size_t cb,
                         cl_uint num_events_in_wait_list,
                         const cl_event * event_wait_list,
                         cl_event * event_list);

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReduceBuffer(cl_command_queue cmd_queue,
                      cl_uint num_src_buffers,
                      cl_mem * src_buffer_list,
                      cl_mem dst_buffer,
                      size_t * src_offset,
                      size_t dst_offset,
                      size_t cb,
                      cl_channel_type datatype,
                      cl_uint num_events_in_wait_list,
                      const cl_event * event_wait_list,
                      cl_event * event);

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueAllReduceBuffer(cl_command_queue * cmd_queue_list,
                         cl_uint num_buffers,
                         cl_mem * src_buffer_list,
                         cl_mem * dst_buffer_list,
                         size_t * src_offset,
                         size_t * dst_offset,
                         size_t cb,
                         cl_channel_type datatype,
                         cl_uint num_events_in_wait_list,
                         const cl_event * event_wait_list,
                         cl_event * event_list);

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReduceScatterBuffer(cl_command_queue * cmd_queue_list,
                             cl_uint num_buffers,
                             cl_mem * src_buffer_list,
                             cl_mem * dst_buffer_list,
                             size_t * src_offset,
                             size_t * dst_offset,
                             size_t cb,
                             cl_channel_type datatype,
                             cl_uint num_events_in_wait_list,
                             const cl_event * event_wait_list,
                             cl_event * event_list);

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueScanBuffer(cl_command_queue * cmd_queue_list,
                    cl_uint num_buffers,
                    cl_mem * src_buffer_list,
                    cl_mem * dst_buffer_list,
                    size_t * src_offset,
                    size_t * dst_offset,
                    size_t cb,
                    cl_channel_type datatype,
                    cl_uint num_events_in_wait_list,
                    const cl_event * event_wait_list,
                    cl_event * event_list);

/* File I/O APIs */
extern CL_API_ENTRY cl_file CL_API_CALL
clEnqueueLocalFileOpen(cl_command_queue command_queue,
                       cl_bool blocking_open,
                       const char * filename,
                       cl_file_open_flags flags,
                       cl_uint num_events_in_wait_list,
                       const cl_event * event_wait_list,
                       cl_event * event,
                       cl_int * errcode_ret);

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBufferToFile(cl_command_queue command_queue,
                          cl_mem src_buffer,
                          cl_file dst_file,
                          size_t src_offset,
                          size_t dst_offset,
                          size_t cb,
                          cl_uint num_events_in_wait_list,
                          const cl_event * event_wait_list,
                          cl_event * event);

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyFileToBuffer(cl_command_queue command_queue,
                          cl_file src_file,
                          cl_mem dst_buffer,
                          size_t src_offset,
                          size_t dst_offset,
                          size_t cb,
                          cl_uint num_events_in_wait_list,
                          const cl_event * event_wait_list,
                          cl_event * event);

extern CL_API_ENTRY cl_int CL_API_CALL
clRetainFile(cl_file file);
    
extern CL_API_ENTRY cl_int CL_API_CALL
clReleaseFile(cl_file file);

extern CL_API_ENTRY cl_int CL_API_CALL
clGetFileHandlerInfo(cl_file file,
                     cl_file_info param_name,
                     size_t param_value_size,
                     void * param_value,
                     size_t * param_value_size_ret);

#ifdef __cplusplus
}
#endif

#endif /* __CL_EXT_SNUCL_H */
