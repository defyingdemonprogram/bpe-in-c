# Byte Pair Encoding

Byte Pair Encoding (BPE) is a compression algorithm originally introduced in 1994 by Philip Gage. It encodes text into a more compact form by iteratively replacing the most frequent pairs of bytes with new symbols, which are stored in a translation table.

A slightly modified version of this algorithm is widely used in tokenizers for large language models. While the original BPE algorithm focuses on compression, its adaptation for tokenization replaces the most frequent byte pairs with new tokens that do not appear in the initial dataset.

## Getting Started

1. Build the project using `nob`:

```bash
cc -o nob nob.c  # run this only the first time
./nob
```

2. Run the executable:

```bash
./build/bpe
```

## References

* **stb_ds.h – nothings.org**: A single-header library providing type-safe dynamic arrays and hash tables.
* **Byte Pair Encoding – Wikipedia**: [https://en.wikipedia.org/wiki/Byte-pair_encoding](https://en.wikipedia.org/wiki/Byte-pair_encoding)
