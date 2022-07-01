# jdiff

### Description

#### Purpose
This application prepares and generates diff files (deltas and signatures) to allow user update files
without need to send the whole data. It's especially useful when it comes to update large files. It's much
more efficient to send Kilobytes or even Megabytes instead of Gigabytes, isn't it?


#### Usage
Usage:
./jdiff target <arg> {required args} [options]

target options:

-s, --signature <base_file_path> {-o <out>} [-x | -f]
Create signature file

-d, --delta <signature_path> {-i <new> | -o <out>} [-x | -f]
Create delta file

-p, --patch <base_file_path> {-i <delta> | -o <out>} [-x | -f]
Patch file

-h, --help                    Print help

arg options:

-i, --input <file_path>     Additional input file

-o, --output <output_path>  Output file

more options:

-x, --sha                   Enable sha hashing

-f, --force                 Force output overwrite

-b, --block-size <decimal>  Block size to hash (not recommended!)

#### Block size
Basic block size is set to 4096, but it will be recalulated for files smaller than 8192 bytes.
To provide at least two signature chunks for file.

#### Rolling hash - modulo value - M
Rolling hash checksum is 32 bit variable created by concatenation of two 16 bit sums,
so it's reasonable to keep both values in uint16 range (0 - 65535). But there are lots of suggestions in
the web to use the largest prime number in this range to reduce repetitions or cycles.

#### Return data
* Program creates or overwrites file (signature, delta or recreated one) in the filesystem.
* Program returns overwrite prompt when output file already exists (without force option).
* Program returns "Error: " message if:
  * file provided doesn't exist.
  * file is empty.
  * delta sha and base file sha don't match.

### External sources

#### Requirements
https://github.com/eqlabs/recruitment-exercises/blob/8e49a7b8cf9c415466876e852fbd862f74105ec6/rolling-hash.md

#### Documentation
https://www.andrew.cmu.edu/course/15-749/READINGS/required/cas/tridgell96.pdf

https://iq.opengenus.org/rolling-hash/

https://en.wikipedia.org/wiki/Rolling_hash

http://zlib.net/zlib.html

#### Repositories
https://github.com/WayneD/rsync - read, for research purposes

https://github.com/catchorg/Catch2 - one header lib used for testing

https://github.com/stbrumme/xxhash - fast 64bit hashing algorithm 

https://github.com/jarro2783/cxxopts - input parser and help printer

#### Others
https://www.youtube.com/watch?v=BfUejqd07yo