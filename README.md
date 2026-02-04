# ECE-315

## Setting up the `clangd` LSP

Make sure the submodules are updated, this includes the FreeRTOS Kernel and the embeddedsw repo for Xilinx specific headers

```sh
$ git submodule update --init --recursive
```

Then run from the repo root

```sh
$ cmake -B build
```

Now clangd will work!!

