Write user defined functions for DuckDB in CPython.

# WIP Disclaimer
You shouldn't use this. I mean, unless you want to. I'm not your dad. But be aware, this project is 100% a proof of concept at the moment.

## Current Limitations
Note these are not inherent limitations that can not be overcome, but are presently as yet overcome. Feel free to help with that!
* Only presently compiles with Python3.9. Will not work with Python3.8, or Python3.10. Do not ask me about Python3.11.
* Not optimized for high throughput queries.
* Only Scalar functions are supported (table functions would be great!)
* Only string arugements and return values are supported.


# Getting started
Clone the repo being sure to use the `recurse-submodules` option:

```sh
git clone --recurse-submodules git@github.com:MarkRoddy/duckdb-python-udf.git
```
Note that `--recurse-submodules` will ensure the correct version of duckdb is pulled allowing you to get started right away.

## Dependencies
Python3.9 development version. On a Ubuntu system, you can install these via:
```sh
sudo apt-get -y install libpython3.9 libpython3.9-dev
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

Now we can use the features from the extension directly in DuckDB. The template contains a single scalar function `python_udf()` that takes a string arguments and returns a string:
```
D select python_udf('Jane') as result;
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


## Distributing your extension
Easy distribution of extensions built with this template is facilitated using a similar process used by DuckDB itself. 
Binaries are generated for various versions/platforms allowing duckdb to automatically install the correct binary.

This step requires that you pass the following 4 parameters to your GitHub repo as action secrets:

| secret name   | description                         |
| ------------- | ----------------------------------- |
| S3_REGION     | s3 region holding your bucket       |
| S3_BUCKET     | the name of the bucket to deploy to |
| S3_DEPLOY_ID  | the S3 key id                       |
| S3_DEPLOY_KEY | the S3 key secret                   |

After setting these variables, all pushes to master will trigger a new (dev) release. Note that your AWS token should
have full permissions to the bucket, and you will need to have ACLs enabled.

### Installing the deployed binaries
To install your extension binaries from S3, you will need to do two things. Firstly, DuckDB should be launched with the 
`allow_unsigned_extensions` option set to true. How to set this will depend on the client you're using. Some examples:

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

Secondly, you will need to set the repository endpoint in DuckDB to the HTTP url of your bucket + version of the extension 
you want to install. To do this run the following SQL query in DuckDB:
```sql
SET custom_extension_repository='bucket.s3.eu-west-1.amazonaws.com/<your_extension_name>/latest';
```
Note that the `/latest` path will allow you to install the latest extension version available for your current version of 
DuckDB. To specify a specific version, you can pass the version instead.

After running these steps, you can install and load your extension using the regular INSTALL/LOAD commands in DuckDB:
```sql
INSTALL <your_extension_name>
LOAD <your_extension_name>
```

### Versioning of your extension
Extension binaries will only work for the specific DuckDB version they were built for. Since you may want to support multiple 
versions of DuckDB for a release of your extension, you can specify which versions to build for in the CI of this template.
By default, the CI will build your extension against the version of the DuckDB submodule, which should generally be the most
recent version of DuckDB. To build for multiple versions of DuckDB, simply add the version to the matrix variable, e.g.:
```
strategy:
    matrix:
        duckdb_version: [ '<submodule_version>', 'v0.7.0']
```

## Setting up CLion 

### Opening project
Configuring CLion with the extension template requires a little work. Firstly, make sure that the DuckDB submodule is available. 
Then make sure to open `./duckdb/CMakeLists.txt` (so not the top level `CMakeLists.txt` file from this repo) as a project in CLion.
Now to fix your project path go to `tools->CMake->Change Project Root`([docs](https://www.jetbrains.com/help/clion/change-project-root-directory.html)) to set the project root to the root dir of this repo.

### Debugging
To set up debugging in CLion, there are two simple steps required. Firstly, in `CLion -> Settings / Preferences -> Build, Execution, Deploy -> CMake` you will need to add the desired builds (e.g. Debug, Release, RelDebug, etc). There's different ways to configure this, but the easiest is to leave all empty, except the `build path`, which needs to be set to `../build/{build type}`. Now on a clean repository you will first need to run `make {build type}` to initialize the CMake build directory. After running make, you will be able to (re)build from CLion by using the build target we just created.

The second step is to configure the unittest runner as a run/debug configuration. To do this, go to `Run -> Edit Configurations` and click `+ -> Cmake Application`. The target and executable should be `unittest`. This will run all the DuckDB tests. To specify only running the extension specific tests, add `--test-dir ../../.. [sql]` to the `Program Arguments`. Note that it is recommended to use the `unittest` executable for testing/development within CLion. The actual DuckDB CLI currently does not reliably work as a run target in CLion.
