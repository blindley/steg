# Project Scope
The program will be used to hide data in 24-bit rgb or 32-bit rgba images. The technique I will be using is BPCS, or Bit-Plane Complexity Segmentation.

## Input
The input cover file can be any image which can be converted to a 24-bit rgb or 32-bit image. This includes most digital images that exist, as far as I'm aware. For example, 8-bit paletted images can easily be converted to 24-bit images by simply replacing the pixel values with their palette entries. 

## Output
The output will be either a 24-bit rgb image, if the input image had no alpha channel, or a 32-bit rgba image, for input images which did have an alpha channel. In case the input image did have an alpha channel, I will probably leave it untouched. Once my algorithm is completed, I will test the idea of hiding data in the alpha channel, and see what the results look like.

# References
- http://datahide.org/BPCSe/
- https://ttu-ir.tdl.org/handle/2346/11993
