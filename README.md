# PNG 1x1 pixel

Make sure you have `zlib.h` installed, then compile and run:

```
$ cc -std=c99 png_encoder.c -o png_encoder -lz
$ ./png_encoder RRGGBB output.png
```

Public domain.
