# Kamedati

**Kamedati** is a Pronounceable Password Generator. It creates memorable, highly structured passwords using a mixture of syllables, numbers, and special characters. By default, transliterated syllables from the ancient Linear B script are used, but alternative builtin syllable sets and custom external files are also supported.

## Features

* **Template-Based Generation**: Provides complete control over the structure of your passwords using a simple token-based templating system.
* **Flexible Syllable Sources**: Supports default Linear B syllables, an alternative open-syllable pool, or external whitespace-separated custom syllable files.
* **Secure by Default**: Reads fresh entropy directly from `/dev/urandom` by default, ensuring cryptographically secure password generation.
* **Deterministic Mode**: Offers an opt-in deterministic mode driven by the Splitmix64 pseudorandom number generator, which is highly useful for repeatable outputs or testing. However, generating actual production passwords in this mode is _not_ recommended.
* **Entropy Calculation**: Calculates and displays the total number of unique password combinations possible for any given template when running in verbose mode.
* **Graceful Error Handling**: Features robust runtime error management that will exit safely rather than crashing if the system's entropy pool becomes unavailable.

## Compilation

The project source files are located in the `src` subdirectory and are built using the provided Makefile.

To compile the project, simply run:

```bash
make
```

The Makefile will compile the source files into object files within `target/obj/` and create the final executable in the `target/` directory.

To remove the compiled binaries and clean the build directory, run:

```bash
make clean
```

## Installation

The preferred installation method is to create a symbolic link to a directory in your `PATH`.

Assuming you are in the project's directory and `~/bin` is in your `PATH`, you can run the following command:

```bash
ln -s $PWD/target/kamedati ~/bin
```

To uninstall, simply delete the symbolic link created above.

## Usage

Once compiled, the executable is located in the `target` directory. Note that the final binary name is dynamically based on the directory name, but assuming the directory is named `kamedati`:

```bash
./target/kamedati [options]
```

### Options

* `-n, --number <count>`: Number of passwords to generate (default: 5).
* `-t, --template <string>`: Structure pattern for the password (default: "Sssnnx").
* `-b, --syllables <set>`: Select syllable set: 'linearB' (default), 'alt', or 'file:\<path\>'.
* `-s, --seed <num>`: Switch to deterministic mode using the specified Splitmix64 seed.
* `-v, --verbose`: Print the template combination space to standard error.
* `-h, --help`: Show the help information text and exit.
* `-V, --version`: Display application version details and exit.

### Template Tokens

The password structure is defined by passing tokens to the `-t` or `--template` flag.

* `s` : Generates a lowercase syllable (e.g., "da", "ko").
* `S` : Generates a capitalized syllable (e.g., "Da", "Ko").
* `U` : Generates a fully capitalized syllable (e.g., "DA", "KO").
* `n` : Appends a numeric digit (0-9).
* `x` : Appends a special symbol character (e.g., !, @, #, $, %).
* `*` : Any other character in the template is treated as a literal character and inserted directly into the password (e.g., hyphens or spaces).
* `\` : Escape character, treats next character literally.

## Examples

**Basic Generation**

Generate 10 passwords using the default pattern (one capital syllable, two lowercase syllables, two numbers, one special character):

```bash
./target/kamedati -n 10 -t Sssnnx
```

**Custom Patterns**

Generate a password that alternates syllables, numbers and special characters:

```bash
./target/kamedati --template SnxsnxUnn
```

**Deterministic Output**

Generate a repeatable batch of passwords using literal hyphen separators and a specific integer seed:

```bash
./target/kamedati -s 1234567890 -t Sss-Sss-nnnn
```

**Entropy Checking**

View the combination statistics for your chosen pattern without generating multiple passwords:

```bash
./target/kamedati -n 1 -v -t SssUUnnx
```

**Escape Character**

Treat the next character after `\` literally:

```bash
./target/kamedati -t 'Pa\s\sword: Ssssnnnnx' -n 10
```

**Alternative Builtin Syllables**

Use the alternative set of builtin syllables:

```bash
./target/kamedati -t SssnnxSssnnx -b alt
```

**External Syllable File**

Load custom syllables from a whitespace-delimited file:

```bash
./target/kamedati -t Sssnnx -b file:syllables.txt
```
