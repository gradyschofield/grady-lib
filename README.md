# grady-lib
**OpenHashSet** and **OpenHashMap** are open address hash containers.
In the special case of integer to string and string to integer maps, the code can write the map to a file in a format that can be loaded via memory mapping, making the load very fast.
**Watch out: No byte ordering translation happens anywhere in this library.**
Load the containers on the same kind of system that saved the containers.
Use **MMapI2SOpenHashMap** or **MMapS2IOpenHashMap** when loading from disk.

**OpenHashSetTC** and **OpenHashMapTC** are open address hash containers for trivially copyable types.
Having a special case for trivially copyable types allows for very fast copy/destruction operations.
These classes can be saved and memory mapped.
No special classes are necessary for memory mapping these.

**MMapI2HRSOpenHashMap** is an efficient integer to string map for when the strings are highly redundant.
It can be saved and memory mapped.
This feature is the class's *raison d'être*.
Create this structure with MMapI2HRSOpenHashMap::Builder and then write it to disk.

**MMapViewableOpenHashMap** is a map for objects that have serialize and deserialize methods or external functions that can be found by argument dependent lookup.
A concept will check that the constraint is satisfied.
Again the *raison d'être* for this class is fast loading via memory mapping.
The intent here is for the values to be almost trivially copyable so views of the objects can be built with little computation.
Think: structs of simple types, spans, and string_views.
This may not be what you're looking for if you have a map of maps, a very long vectors of strings or anything that would require heap allocation.

**ThreadPool**, **CompletionPool**, and the **parallelForEach** method on OpenHashMap and OpenHashSet are parallelization utilities.
