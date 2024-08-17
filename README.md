# grady-lib
Below are basic open address hash sets/maps. For integer to std:string and std::string to integer maps, the code can write them to a file in an easily memory mapped format, 
- OpenHashMap
- MMapI2SOpenHashMap
- MMapS2IOpenHashMap

Below are open address hash sets and maps for trivially copyable types.  They feature fast copy/destruction operations.  The code can also write them to disk in a memory mappable format.
- OpenHashMapTC
- OpenHashSetTC

Below is an efficient integer to std::string mapping where the strings are highly redundant.  It is also writable to disk in a memory mappable format.
- MMapI2HRSOpenHashMap

Below are utilities for multi-threading.
- ThreadPool
- CompletionPool
- parallelForEach method on OpenHashMap/OpenHashSet
