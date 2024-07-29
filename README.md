# grady-lib
Basic open address hash sets/maps. Integer to std:string and std::string to integer maps can be written to a file in an easily memory mapped format, 
- OpenHashMap
- MMapI2SOpenHashMap
- MMapS2IOpenHashMap

Open address hash sets and maps with fast copy/destruction for trivially copyable types.  Memory mappable from file.
- OpenHashMapTC
- OpenHashSetTC

Efficient integer to std::string mapping where the strings are highly redundant.  Memory mappable from file.
- MMapI2HRSOpenHashMap
