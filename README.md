# Byte Pair Encoding

Byte Pair Encoding (BPE) is a compression algorithm originally introduced in 1994 by Philip Gage. It encodes text into a more compact form by iteratively replacing the most frequent pairs of bytes with new symbols, which are stored in a translation table.

A slightly modified version of this algorithm is widely used in tokenizers for large language models. While the original BPE algorithm focuses on compression, its adaptation for tokenization replaces the most frequent byte pairs with new tokens that do not appear in the initial dataset.

## Quick Start
Build the project
```bash
cc -o nob nob.c # only one time
./nob
```

### text2bpe

Encodes a text file into BPE merges by iteratively replacing the most frequent byte pairs with new tokens.

```bash
./build/text2bpe -input-file samples/file-1.txt -output-dir output
```

**Options:**
- `-report-freq <int>`: Frequency of progress reporting (default: 10)
- `-dump-freq <int>`: Frequency of state dumping (default: 10)
- `-term-freq <int>`: Termination frequency; stops when no pair frequency exceeds this (default: 1)
- `-threads-count <int>`: Number of threads to use (default: 16)
- `-max-iterations <int>`: Maximum number of iterations (0 = no limit, default: 0)

### bpe_inspect

Displays the mapping of token IDs to their string representations from a `.bpe` file.

```bash
./build/bpe_inspect output/92.bpe
```

**Options:**
- `--no-ids`: Print only the rendered tokens without their numeric IDs.

### tkn_inspect

Inspects the contents of a `.tkn` (tokenized sequence) file, optionally rendering it back to text.

```bash
./build/tkn_inspect -bpe output/92.bpe -tkn output/92.tkn -render -ids -split
```

**Options:**
- `-ids`: Print the numeric IDs of the tokens.
- `-render`: Render the tokens back into their string representation.
- `-split`: When rendering, separate tokens with a pipe `|` character.

### bpe_gen

Generates a random sequence of tokens based on a `.bpe` merge table.

```bash
./build/bpe_gen -bpe output/92.bpe -limit 50 -delim " "
```

**Options:**
- `-limit <int>`: Maximum number of tokens to generate (default: 100).
- `-delim <str>`: Delimiter to place between generated tokens.
- `-seed <int>`: Random seed (default: 100, use 0 for time-based seed).

### bpe2dot

Visualizes the BPE merge hierarchy as a Graphviz directed graph.

```bash
./build/bpe2dot output/92.bpe output.dot
dot -Tpng output.dot -o output.png
```

## References

* **[stb_ds.h – nothings.org](https://nothings.org/stb_ds/)**: A single-header library providing type-safe dynamic arrays and hash tables.
* **[Byte Pair Encoding – Wikipedia](https://en.wikipedia.org/wiki/Byte-pair_encoding)**
* **[nob.h – GitHub](https://github.com/tsoding/nob.h)**
