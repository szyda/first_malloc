# Custom memory manager for dynamic memory allocation
The goal of the project was to implement a memory manager responsible for managing the heap of a program. 

The manager includes custom versions of the `malloc`, `calloc`, `free`, and `realloc` methods, along with additional utility functions for monitoring the state, coherence, and defragmentation of the heap.

### Memory block structure
````
        +-------------------------------------------------------+
        |  METADATA  |  HEAD FENCE  |  USER DATA  |  TAIL FENCE |
        +-------------------------------------------------------+
````
- **Metadata**: Information about the memory block, including pointers to the previous and next blocks, the size of the block, a flag indicating whether the block is free or allocated, and additional metadata for error detection.
- **Head Fence**: Marker at the beginning to detect potential overflows.
- **User Data**: The actual data allocated to the user resides in this portion of the block.
- **Tail Fence**: Marker at the end to detect potential underflows.

### Key Features:
- **Standard Allocation/Deallocation Tasks**: Implementation of memory allocation and deallocation functions following the API specifications of the malloc family. The behavior of these custom implementations mirrors that of standard counterparts from the GNU C Library.
- **Heap Initialization and Cleanup**: The manager provides functions for initializing the heap and cleaning up the entire allocated memory, allowing for resetting the heap to its initial state.
- **Dynamic Heap Expansion**: The memory manager can autonomously increase the heap size by generating requests to the operating system when needed.
- **Fences for Error Detection**: The memory blocks allocated to the user are surrounded by fences (head and tail markers) to facilitate the detection of one-off errors. Violation of fences indicates incorrect usage of allocated memory.

### Methods
- `heap_setup`: Initializes the heap in the designated memory area.
- `heap_clean`: Returns all allocated memory to the operating system.
- `heap_malloc`, `heap_calloc`, `heap_realloc`, `heap_free`: Custom implementations of memory allocation, deallocation  and reallocation functions.
- `heap_get_largest_used_block_size`: Returns the size of the largest memory block allocated to the user.
- `get_pointer_type`: Classifies a pointer based on its relation to different areas of the heap.
- `heap_validate`: Validates the consistency of the heap, checking for potential errors.


**Note**: The project utilizes fences, metadata, and control structures to enhance error detection and offers a comprehensive memory management solution.