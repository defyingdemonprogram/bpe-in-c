# Byte Pair Encoding

Byte Pair Encoding (BPE) is a compression algorithm originally introduced in 1994 by Philip Gage. It encodes text into a more compact form by iteratively replacing the most frequent pairs of bytes with new symbols, which are stored in a translation table.

A slightly modified version of this algorithm is widely used in tokenizers for large language models. While the original BPE algorithm focuses on compression, its adaptation for tokenization replaces the most frequent byte pairs with new tokens that do not appear in the initial dataset.

## Quick Start

1. Build the project using `nob`:

    ```bash
    cc -o nob nob.c  # run this only the first time
    ./nob
    ```

2. Run the executable:

    ```bash
    ./build/text2bpe <input.txt> <output.bpe>
    ```

    `<input.txt>` is the file whose content is used to generate the BPE data. This command generates the `<output.bpe>` file.

    You may convert `output.bpe` to a `.dot` file, which can be used to visualize how tokens are related using Graphviz.

    Convert the pairs into a `.dot` file:

    ```bash
    ./build/bpe2dot <input.bpe> <output.dot>
    ```

    Generate a PNG file from the generated `.dot` file using:

    ```bash
    dot -Tpng input.dot -o output.png
    ```

## References

* **[stb_ds.h – nothings.org](https://nothings.org/stb_ds/)**: A single-header library providing type-safe dynamic arrays and hash tables.
* **[Byte Pair Encoding – Wikipedia](https://en.wikipedia.org/wiki/Byte-pair_encoding)**
* **[nob.h – GitHub](https://github.com/tsoding/nob.h)**