/**
 * @file
 * @brief Public API of the ESDM. Inlcudes several other public interfaces.
 */
#ifndef ESDM_H
#define ESDM_H

#include <esdm-datatypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

///////////////////////////////////////////////////////////////////////////////
// ESDM ///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// These functions must be used before calling init:
/**
 * Set the number of processes to use per node.
 * Must not be called after `init()`.
 *
 * @param [in] procs the number of processes to use per node
 *
 * @return status
 */
esdm_status esdm_set_procs_per_node(int procs);

/**
 * Set the total number of processes to use.
 * Must not be called after `init()`.
 *
 * @param [in] procs the number of processes to use
 *
 * @return status
 */
esdm_status esdm_set_total_procs(int procs);

/**
 * Set the configuration to use.
 * Must not be called after `init()`, and must not be called twice.
 *
 * @param [in] str a string containing configuration data in JSON format
 *
 * @return status
 */
esdm_status esdm_load_config_str(const char *str);

/**
 * Initialize ESDM:
 *	- allocate data structures for ESDM
 *	- allocate memory for node local caches
 *	- initialize submodules
 *	- initialize threadpool
 *
 * @return status
 */

esdm_status esdm_init();

/**
 * Display status information for objects stored in ESDM.
 *
 * @return status
 */

esdm_status esdm_finalize();

/**
 * Write data with a given size and offset.
 *
 * @param [in] dataset TODO, currently a stub, we assume it has been identified/created before...., json description?
 * @param [in] buf the pointer to a contiguous memory region that shall be written to permanent storage
 * @param [in] subspace an existing dataspace that describes the shape and location of the hypercube that is to be written
 *
 * @return status
 */

esdm_status esdm_write(esdm_dataset_t *dataset, void *buf, esdm_dataspace_t *subspace);

/**
 * Reads a data fragment described by desc to the dataset dset.
 *
 * @param [in] dataset TODO, currently a stub, we assume it has been identified/created before.... , json description?
 * @param [out] buf a contiguous memory region that shall be filled with the data from permanent storage
 * @param [in] subspace an existing dataspace that describes the shape and location of the hypercube that is to be read
 *
 * @return status
 */

esdm_status esdm_read(esdm_dataset_t *dataset, void *buf, esdm_dataspace_t *subspace);

///////////////////////////////////////////////////////////////////////////////
// Public API: Data Model Manipulators ////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Container //////////////////////////////////////////////////////////////////

/**
 * Create a new container.
 *
 *  - Allocate process local memory structures.
 *  - Register with metadata service.
 *
 * @param [in] name string to identify the container, must not be empty
 * @param [out] out_container returns a pointer to the new container
 *
 * @return status
 *
 */

esdm_status esdm_container_create(const char *name, int allow_overwrite, esdm_container_t **out_container);

/**
 * Open an existing container.
 *
 *  - Allocate process local memory structures.
 *  - Register with metadata service.
 *
 * @param [in] name string to identify the container, must not be empty
 * @param [out] out_container returns a pointer to the container
 *
 * @return status
 */
esdm_status esdm_container_open(const char *name, int esdm_mode_flags, esdm_container_t **out_container);

/**
 * Make container persistent to storage.
 * Enqueue for writing to backends.
 *
 * Calling container commit may trigger subsequent commits for datasets that
 * are part of the container.
 *
 * @param [in] container pointer to an existing container which is to be committed to storage
 *
 * @return status
 *
 */

esdm_status esdm_container_commit(esdm_container_t *container);

esdm_status esdm_container_delete_attribute(esdm_container_t *c, const char *name);

esdm_status esdm_container_link_attribute(esdm_container_t *container, int overwrite, smd_attr_t *attr);

/* This function returns the attributes */
esdm_status esdm_container_get_attributes(esdm_container_t *container, smd_attr_t **out_metadata);

/**
 * Close a container object. If it isn't in use any more free it.
 *
 * @param [in] container an existing container object that is no longer needed
 */

esdm_status esdm_container_close(esdm_container_t *container);

/*
 * Check if the dataset with the given name exists.
 *
 * @param [in] container an existing container to query
 * @param [in] name the identifier to check, must not be NULL
 *
 * @return true if the identifier already exists within the container
 */
bool esdm_container_dataset_exists(esdm_container_t * container, char const * name);

/*
 * Return the number of datasets in the container.
 *
 * @param [in] container an existing container to query
 *
 * @return the number of datasets
 */
int esdm_container_dataset_count(esdm_container_t * container);

esdm_status esdm_dataset_rename(esdm_dataset_t *dataset, const char *name);

/*
 * Return the n-th dataset in the container array.
 *
 * @param [in] container an existing container to query
 * @param [in] dset_number the number (it shall exists according to esdm_container_dataset_count())
 *
 * @return the dataset or NULL, if dset_number >= count
 */
esdm_dataset_t * esdm_container_dataset_from_array(esdm_container_t * container, int dset_number);

// Dataset

/**
 * Create a new dataset.
 *
 *  - Allocate process local memory structures.
 *	- Register with metadata service.
 *
 * @param [in] container pointer to an existing container to which the new dataset will be linked
 * @param [in] name identifier for the new dataset, must not be empty
 * @param [in] dataspace pointer to an existing dataspace which defines the shape of the data that will be stored within the dataset
 * @param [out] out_dataset returns a pointer to the new dataset
 *
 * @return status
 *
 */

esdm_status esdm_dataset_create(esdm_container_t *container, const char *name, esdm_dataspace_t *dataspace, esdm_dataset_t **out_dataset);

esdm_status esdm_dataset_name_dims(esdm_dataset_t *dataset, char **names);

/*
 The value to be used if data hasn't been written to a datapoint, it must be of the same type as the dataset type.
 If the fill value was already set, overwrite it.
 */
esdm_status esdm_dataset_set_fill_value(esdm_dataset_t *dataset, void * value);

/*
 Copy the fill value into value
 */
esdm_status esdm_dataset_get_fill_value(esdm_dataset_t *dataset, void * value);

/*
 Returns the name of the dataset, do not free or temper with it, the name is still owned by the dataset
 */
char const * esdm_dataset_name(esdm_dataset_t *dataset);

esdm_status esdm_dataset_get_name_dims(esdm_dataset_t *dataset, char const *const **out_names);

esdm_status esdm_dataset_get_dataspace(esdm_dataset_t *dset, esdm_dataspace_t **out_dataspace);
smd_dtype_t const * esdm_dataset_get_type(esdm_dataset_t * d);

int64_t esdm_dataspace_get_dims(esdm_dataspace_t * d);
int64_t const * esdm_dataspace_get_size(esdm_dataspace_t * d);
int64_t const * esdm_dataspace_get_offset(esdm_dataspace_t * d);
smd_dtype_t const * esdm_dataspace_get_type(esdm_dataspace_t * d);

esdm_status esdm_dataset_change_name(esdm_dataset_t *dset, char const * new_name);

/*
 This function returns the actual size for ulim
 Return a pointer to the internal size (having the same dimensions)
 */
int64_t const * esdm_dataset_get_actual_size(esdm_dataset_t *dset);

esdm_status esdm_dataset_iterator(esdm_container_t *container, esdm_dataset_iterator_t **out_iter);

/**
 * Open a dataset.
 *
 *  - Allocate process local memory structures
 *  - Retrieve metadata
 *
 * @param [in] container pointer to an open container that contains the dataset that is to be opened
 * @param [in] name identifier of the dataset within the container, must not be empty
 * @param [out] out_dataset returns a pointer to the opened dataset
 *
 * @return status
 */
esdm_status esdm_dataset_open(esdm_container_t *container, const char *name, int esdm_mode_flags, esdm_dataset_t **out_dataset);

/*
 Similar to esdm_dataset_open but returns the dataset without opening it
 */
esdm_status esdm_dataset_by_name(esdm_container_t *container, const char *name, int esdm_mode_flags, esdm_dataset_t **out_dataset);

/*
 * Obtain a reference to the dataset, if it was not yet open, it will be openend and metadata will be fetched.
 * To return the dataset, call dataset_close()
 *
 * This function is *not thread-safe*.
 * Only a single master thread must be used to call into ESDM.
 */
esdm_status esdm_dataset_ref(esdm_dataset_t *dataset);


/**
 * Make dataset persistent to storage.
 * Schedule for writing to backends.
 *
 * @param [in] dataset pointer to an existing dataset which is to be committed to storage
 *
 * @return status
 */

esdm_status esdm_dataset_commit(esdm_dataset_t *dataset);


/**
 * Close a dataset object, if it isn't used anymore, it's metadata will be unloaded
 *
 * This function is *not thread-safe*.
 * Only a single master thread must be used to call into ESDM.
 *
 * @param [in] dataset an existing dataset object that is no longer needed
 *
 * @return status
 */
esdm_status esdm_dataset_close(esdm_dataset_t *dataset);

esdm_status esdm_dataset_delete_attribute(esdm_dataset_t *dataset, const char *name);

/* This function adds the metadata to the ESDM */

esdm_status esdm_dataset_link_attribute(esdm_dataset_t *dset, int overwrite, smd_attr_t *attr);

/* This function returns the attributes */
esdm_status esdm_dataset_get_attributes(esdm_dataset_t *dataset, smd_attr_t **out_metadata);

// Dataspace

/**
 * Create a new dataspace.
 *
 *  - Allocate process local memory structures.
 *
 * @param [in] dims count of dimensions of the new dataspace
 * @param [in] sizes array of the sizes of the different dimensions, the length of this array is dims. Must not be `NULL` unless `dims == 0`
 * @param [in] type the datatype for each data point
 * @param [out] out_dataspace pointer to the new dataspace
 *
 * @return status
 *
 */

esdm_status esdm_dataspace_create(int64_t dims, int64_t *sizes, esdm_type_t type, esdm_dataspace_t **out_dataspace);

/**
 * Reinstantiate dataspace from serialization.
 */

esdm_status esdm_dataspace_deserialize(void *serialized_dataspace, esdm_dataspace_t **out_dataspace);

/**
 * Define a dataspace that is a subset of the given dataspace.
 *
 * - Allocates process local memory structures.
 *
 * @param [in] dataspace an existing dataspace that encloses the subspace
 * @param [in] dims length of the `size` and `offset` arguments, must be equal to the number of dimensions of the given `dataspace`
 * @param [in] size size of the hypercube of data within the subspace
 * @param [in] offset location of the first data point within the subspace
 * @param [out] out_dataspace pointer to the new sub-dataspace
 *
 * @return `ESDM_SUCCESS` on success, `ESDM_INVALID_ARGUMENT_ERROR` if the provided `dims`, `size`, or `offset` arguments do not agree with the provided `dataspace`
 */
esdm_status esdm_dataspace_subspace(esdm_dataspace_t *dataspace, int64_t dims, int64_t *size, int64_t *offset, esdm_dataspace_t **out_dataspace);

/**
 * Specify a non-standard serialization order for a dataspace.
 *
 * This can be used to handle FORTRAN arrays, for example, or do some crazy stuff like inverted dimensions, or to skip over holes.
 * *Use carefully, or don't use at all. You have been warned.*
 *
 * @param [inout] dataspace the dataspace that is to be modified
 * @param [in] dims number of entries in the `stride` argument, must match the dimension count of the dataspace
 * @param [in] stride array with `dims` entries, each entry gives the number of elements to skip over when increasing the respective coordinate by one.
 *
 * @return status
 *
 * Examples:
 *
 * A C array `int array[7][11]` does not need a stride, the stride is implicitly assumed to be `(11, 1)`.
 *
 * To handle a FORTRAN array `INTEGER :: array(7, 11)`, use the following call:
 *     esdm_dataspace_set_stride(dataspace, 2, (int64_t[2]){1, 7});
 *
 * To use only a 3x5 part of an existing C array `int array[7][11]`, starting at (1, 2), use these calls:
 *     esdm_dataspace_t* subspace;
 *     esdm_dataspace_subspace(parent, 2, (int64_t[2]){3, 5}, (int64_t[2]){1, 2}, &subspace);
 *     esdm_dataspace_set_stride(subspace, (int64_t[2]){11, 1});
 * After this, the 2D coordinates will be mapped to the buffer offsets like this:
 *     (1,2)=0,  (1,3)=1,  (1,4)=2,  (1,5)=3,  (1,6)=4,
 *     (2,2)=11, (2,3)=12, (2,4)=13, (2,5)=14, (2,6)=15,
 *     (3,2)=22, (3,3)=23, (3,4)=24, (3,5)=25, (3,6)=26,
 */
esdm_status esdm_dataspace_set_stride(esdm_dataspace_t* dataspace, int64_t* stride);

/**
 * Copy data from one buffer to another, possibly partially, possibly rearranging the data as prescribed by the given dataspaces.
 *
 * This function copies all the data that is contained within the intersection of the two dataspaces from the source buffer to the destination buffer.
 * The order and layout of the data elements in each buffer is described by the associated dataspace, allowing this function to be used to
 *
 *   * pack non-contiguous (= strided) data into a contiguous buffer
 *   * unpack a contiguous buffer into a larger, non-contiguous (= strided) dataspace
 *   * transpose data (for example from FORTRAN order to C order and vice versa)
 *   * reverse the order of the data in one or more dimensions (rather esoteric use!)
 *
 * In all cases, only the intersection of the two hypercubes described by the two dataspaces is copied:
 * If the source space is larger, only the overlapping part will be read,
 * and if the destination space is larger, only the overlapping part will be written to.
 * If the two dataspaces don't intersect, nothing will be done.
 *
 * @param [in] sourceSpace dataspace that describes the layout of the `sourceData` buffer
 * @param [in] sourceData pointer to the first source data element, the logical coordinate of this data element is the offset of the source dataspace
 * @param [in] destSpace dataspace that describes the layout of the `destData` buffer
 * @param [out] destData pointer to the first destination data element, the logical coordinate of this data element is the offset of the destination dataspace
 *
 * @return status
 */
esdm_status esdm_dataspace_copy_data(esdm_dataspace_t* sourceSpace, void *sourceData, esdm_dataspace_t* destSpace, void *destData);

/**
 * Destruct and free a dataspace object.
 *
 * @param [in] dataspace an existing dataspace object that is no longer needed
 *
 * @return status
 *
 * "_destroy" sounds too destructive, this will be renamed to esdm_dataspace_close().
 */
esdm_status esdm_dataspace_destroy(esdm_dataspace_t *dataspace);

/**
 * Serializes dataspace description.
 *
 * e.g., to store along with fragment
 */

esdm_status esdm_dataspace_serialize(esdm_dataspace_t *dataspace, void **out);

uint64_t esdm_dataspace_element_count(esdm_dataspace_t *dataspace);

uint64_t esdm_dataspace_size(esdm_dataspace_t *dataspace);


// Fragment

/**
 * Reinstantiate fragment from serialization.
 */

esdm_status esdm_fragment_deserialize(void *serialized_fragment, esdm_fragment_t **_out_fragment);

esdm_status esdm_fragment_retrieve(esdm_fragment_t *fragment);

/**
 * Make fragment persistent to storage.
 * Schedule for writing to backends.
 *
 * @param [in] fragment pointer to an existing fragment which is to be committed to storage
 *
 * @return status
 */

esdm_status esdm_fragment_commit(esdm_fragment_t *fragment);

/**
 * Destruct and free a fragment object.
 *
 * @param [in] fragment an existing fragment object that is no longer needed
 *
 * @return status
 *
 * "_destroy" sounds too destructive, this will be renamed to esdm_fragment_close().
 */
esdm_status esdm_fragment_destroy(esdm_fragment_t *fragment);

/**
 * Serializes fragment for storage.
 *
 * @startuml{fragment_serialization.png}
 *
 * User -> Fragment: serialize()
 *
 * Fragment -> Dataspace: serialize()
 * Fragment <- Dataspace: (status, string)
 *
 * User <- Fragment: (status, string)
 *
 * @enduml
 *
 */


void esdm_fragment_print(esdm_fragment_t *fragment);

void esdm_dataspace_print(esdm_dataspace_t *dataspace);

//size_t esdm_sizeof(esdm_type_t type);
#define esdm_sizeof(type) (type->size)

/**
  * Initialize backend by invoking mkfs callback for matching target
  *
  * @param [in] enforce_format  force reformatting existing system (may result in data loss)
  * @param [in] target  target descriptor
  *
  * @return status
  */

enum esdm_format_flags{
  ESDM_FORMAT_DELETE = 1,
  ESDM_FORMAT_CREATE = 2,
  ESDM_FORMAT_IGNORE_ERRORS = 4,
  ESDM_FORMAT_PURGE_RECREATE = 7
};

esdm_status esdm_mkfs(int format_flags, data_accessibility_t target);

//// LOGGING

/*
Loglevel for stdout.
*/
void esdm_loglevel(esdm_loglevel_e loglevel);
void esdm_log_on_exit(int on);
/*
 Keeps a log to record last messages for crashes
 Must be called from a single master thread
 NOTE: logging into the shared buffer costs performance.
*/
void esdm_loglevel_buffer(esdm_loglevel_e loglevel);


#endif
