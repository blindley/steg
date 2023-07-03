# Project Scope
The program will be used to hide data in 24-bit rgb or 32-bit rgba images. The technique I will be using is BPCS, or Bit-Plane Complexity Segmentation.

## Input
The input cover file can be any image which can be converted to a 32-bit rgba image. This includes most digital images that exist, as far as I'm aware. For example, 8-bit paletted images can easily be converted to 24-bit images by simply replacing the pixel values with their palette entries. However, I suspect such images will have very low capacity for data hiding.

## Output
The output will be either a 24-bit rgb image, if the input image had no alpha channel, or a 32-bit rgba image, for input images which did have an alpha channel. In case the input image did have an alpha channel, I will probably leave it untouched. Once my algorithm is completed, I will test the idea of hiding data in the alpha channel, and see what the results look like.

## The Algorithm
- Parse command line arguments
- Determine which function the user wants to perform: Hide, extract or measure
- If Hide:
  - Load image
  - Convert image to 32 bits per pixel
  - Convert each byte of image to canonical gray code
  - Extract chunks to chunk array (explained below)
  - Load message file
  - Format message for hiding (explained below)
  - For each chunk:
    - If no message bytes left to hide: break from loop
    - Measure chunk complexity (explained below)
    - If chunk complexity >= threshold (probably 0.3):
      - Replace chunk with next 8 bytes from formatted message
  - If the entire message hasn't been hidden: throw error
  - Replace chunks back into image
  - Convert image bytes back to pure binary code
  - Save image in lossless format
- If Extract:
  - Load image
  - Convert image to 32 bits per pixel
  - Convert image bytes to canonical gray code
  - Extract chunks
  - Create expandable byte array for storing message
  - For each chunk:
    - Measure chunk complexity
    - If chunk complexity >= threshold:
      - Append chunk to message
  - Deconjugate conjugated chunks
  - If signature not present: throw error
  - Unformat message
  - Save message to file
- If Measure:
  - Load image
  - Convert image to 32 bits per pixel
  - Convert each byte of image to canonical gray code
  - Extract chunks
  - Count the number of complex chunks
  - Calculate byte capacity from the number of complex chunks, accounting for metadata
  - Report byte capacity to user

### Extract Chunks
First, some definitions.
 - Bitplane: A 2-dimensional array of bits, 1 bit from each pixel of the image (after gray code conversion). There are 32 bitplanes in a 32 bit image, indexes ranging from 0 to 31, with 0 being the MSB of the red channel, and 31 being the LSB of the alpha channel.
 - Chunk: An 8x8 subsection of a bitplane. There are 64 bits, or 8 bytes in a chunk. The number of chunks per bitplane is (image.width / 8) * (image.height / 8).

So what the extract chunks step does is create a byte array from the image, rearranging the bits such that all the bits of any particular chunk are adjacent to each other in memory, packed together in a sequence of 8 bytes. We start with the chunks from the least significant bitplanes (7, 15, 23, 31, ...) and move up to more significant bitplanes.

### Format Message
 - Conjugate: modify a chunk by xoring it with a checkerboard pattern of alternating bits. This has the effect of flipping the complexity of the chunk (new_complexity = 1 - old_complexity). Conjugating a previously conjugated chunk gives back the original chunk.

The message needs to be formatted in a way that the extraction algorithm can find it. If we just store it the straightforward way, 8 bytes at a time, some of the chunks might not exceed the complexity threshold, in which case the extraction algorithm will have to assume those chunks are just part of the unmodified image. If we just conjugate every chunk that is below the threshold, we have another problem, which is that we don't know which chunks need to be conjugated during extraction. So what we do is insert a signal bit at the beginning of each chunk, set to 0, followed by 63 bits from the message. Then, when we conjugate a chunk, this bit gets flipped to 1, so we can use the first bit to detect whether a chunk needs to be conjugated on extraction. This is kind of a messy process, and Professor Ortiz suggested an alternative method which we may switch to.

Some more considerations for the formatted message. First, there should be a way to detect if the image even holds a hidden message at all, so we will insert a 3 specific bytes at the beginning of the first chunk. These will just be some random numbers that I will pick. Next, we need to know when the message ends, so we will insert a 32 bit integer after the signature bytes, indicating the size of the stored message. Finally, after inserting signal bits, signature, size and the message content, we will pad the formatted message to a multiple of 64 bits. The content of these bits will be ignored. After all that is done, we have a byte array which is some multiple of 8 bytes, and we go through it 8 bytes at a time, conjugating all chunks which are below the complexity threshold.

### Measure Complexity
I won't go into detail here about how complexity is measured, but it basically corellates to the number of bit transitions (0 to 1 or 1 to 0) between adjacent bits in a chunk. The calculation gives a value between 0 and 1. The basic theory behind BPCS is that if you replace one complex portion of an image with another, humans have difficulty percieving the difference. So we hide our message in chunks which pass a certain complexity threshold.

# References
- http://datahide.org/BPCSe/
- https://ttu-ir.tdl.org/handle/2346/11993
