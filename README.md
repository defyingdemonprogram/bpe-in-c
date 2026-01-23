# Byte Pair Encoding

Byte Pair Encoding (BPE) is a compression algorithm originally introduced in 1994 by Philip Gage. It encodes text into a more compact form by iteratively replacing the most frequent pairs of bytes with new symbols, which are stored in a translation table.

A slightly modified version of this algorithm is widely used in tokenizers for large language models. While the original BPE algorithm focuses on compression, its adaptation for tokenization replaces the most frequent byte pairs with new tokens that do not appear in the initial dataset.

## Quick Start

1. Build the project using `nob`:

    ```bash
    cc -o nob nob.c  # run this only the first time
    ./nob
    ```

2. Run the executable

    ```bash
    ./build/text2bpe -input-file <input.txt> -output-dir <output-dir>
    ```

    * `<input.txt>`: input text used to generate BPE merges
    * `<output-dir>`: directory where BPE iteration files will be written

    By default, the tool runs until no pair exceeds the termination frequency and dumps progress every 10 iterations. Each iteration is written to `<output-dir>` as `<iteration>.bpe`.

    Optional flags:

    ```bash
    -report-freq <int>      # progress reporting frequency (default: 10)
    -dump-freq <int>        # dump state frequency (default: 10)
    -term-freq <int>        # termination pair frequency (default: 1)
    -max-iterations <int>   # max iterations, 0 = no limit (default: 0)
    ```

3. Visualize BPE merges (optional)

    Convert a `.bpe` file to Graphviz `.dot` format:

    ```bash
    ./build/bpe2dot <input.bpe> <output.dot>
    ```

    Generate a PNG from the `.dot` file:

    ```bash
    dot -Tpng output.dot -o output.png
    ```

## References

* **[stb_ds.h – nothings.org](https://nothings.org/stb_ds/)**: A single-header library providing type-safe dynamic arrays and hash tables.
* **[Byte Pair Encoding – Wikipedia](https://en.wikipedia.org/wiki/Byte-pair_encoding)**
* **[nob.h – GitHub](https://github.com/tsoding/nob.h)**
