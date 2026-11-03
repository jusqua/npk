# Node Pipelined Kernels (NPK)

Execute SYCL kernels with graph nodes.

<img width="1917" height="1055" alt="image" src="https://github.com/user-attachments/assets/3100e470-deda-423a-a872-491f1ec79232" />

## Features

- Import PNG and TIFF 2D images
- Process every kernel provided by [HOK](https://github.com/jusqua/hok)

## TODOs

- Save processed images
- NPK samples
- NPK project save and load

## Compiling

NPK requires Intel DPCPP C++ or AdaptiveCpp compiler to build SYCL kernels.

```bash
cmake -DCMAKE_CXX_COMPILER=<sycl-compiler> # Force compiler package load by using -DPREFER_BACKEND=AdaptiveCpp or -DPREFER_BACKEND=IntelSYCL
```

## License

[MIT](./LICENSE.md)
