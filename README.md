Write user defined functions for DuckDB in CPython.

# WIP Disclaimer
You shouldn't use this. I mean, unless you want to. I'm not your dad. But be aware, this project is 100% a proof of concept at the moment.

## Current Limitations
Note these are not inherent limitations that can not be overcome, but are presently as yet overcome. Feel free to help with that!
* Only presently compiles with Python3.9. Will not work with Python3.8, or Python3.10. Do not ask me about Python3.11.
* Not optimized for high throughput queries.
* Only Scalar functions are supported (table functions would be great!)
* Only string arugements and return values are supported.

# Installation and Usage


First, [install DuckDB](https://duckdb.org/docs/installation/).

Next, start the DuckDB shell using the 'unsigned' option. Note that depending on your choosen environment (commandline, Python, etc) the manner in which you specify this will vary. A few examples are provided in the next section below.

Run the following commands to install the extension and activate it:

```sql
SET custom_extension_repository='net.ednit.duckdb-extensions.s3.us-west-2.amazonaws.com/python_udf/latest';
INSTALL python_udf;
LOAD python_udf;
```

You can now use any Python function in your path as part of a query. Example:
```sql
> select python_udf('string', 'capwords', 'foo bar baz') as result;
┌─────────────┐
│   result    │
│   varchar   │
├─────────────┤
│ Foo Bar Baz │
└─────────────┘
>
```

## Setting the Unsigned Option
CLI:
```shell
duckdb -unsigned
```

Python:
```python
con = duckdb.connect(':memory:', config={'allow_unsigned_extensions' : 'true'})
```

NodeJS:
```js
db = new duckdb.Database(':memory:', {"allow_unsigned_extensions": "true"});
```
    

# Development
Clone the repo being sure to use the `recurse-submodules` option:

```sh
git clone --recurse-submodules git@github.com:MarkRoddy/duckdb-python-udf.git
```
Note that `--recurse-submodules` will ensure the correct version of duckdb is pulled allowing you to get started right away.

## Dependencies
Python3.9 development version. On a Ubuntu system, you can install these via:
```sh
sudo apt-get -y install python3.9-dev
```

## Building
To build the extension:
```sh
make
```
The main binaries that will be built are:
```sh
./build/release/duckdb
./build/release/test/unittest
./build/release/extension/python_udf/python_udf.duckdb_extension
```
- `duckdb` is the binary for the duckdb shell with the extension code automatically loaded. 
- `unittest` is the test runner of duckdb. Again, the extension is already linked into the binary.
- `python_udf/python_udf.duckdb_extension` is the loadable binary as it would be distributed.

## Running the extension
To run the extension code, simply start the shell with `./build/release/duckdb`.

Now we can use the features from the extension directly in DuckDB. Included in this extension is the ability to execute python functions. Bundled with this repository is a python file named 'udfs.py' that contains some example functions. You can invoke a function in this module by specifying the module name, the function name, and a single string argument to be passed to the function:
```
D select python_udf('udfs', 'reverse', 'Jane') as result;
┌───────────────┐
│    result     │
│    varchar    │
├───────────────┤
│     enaJ      │
└───────────────┘
```

## Running the tests
Different tests can be created for DuckDB extensions. The primary way of testing DuckDB extensions should be the SQL tests in `./test/sql`. These SQL tests can be run using:
```sh
make test
```

