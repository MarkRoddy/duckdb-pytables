"""Installs the Extension"""

import os, sys
import time
import shutil
import argparse
import subprocess
import ctypes.util
import subprocess
import logging as log


def check_os():
    log.debug(f"Checking if '{sys.platform}' is a valid platform for installation")
    if sys.platform in ("linux", "linux2"):
        log.debug("we good")
        return True
    elif platform == "darwin":
        # OS X
        log.debug("Not yet supported")
        print("Builds for OSX are not yet available. If you'd like to see OSX support, please share your voice here:")
        print("https://github.com/MarkRoddy/duckdb-pytables/issues/37")
        return False
    elif platform == "win32":
        log.debug("Not yet supported")
        print("Builds for Windows are not yet available. If you'd like to see Windows support, please share your voice here:")
        print("https://github.com/MarkRoddy/duckdb-pytables/issues/38")
        return False
    else:
        log.debug("This platform isn't recognized so we're just gonna hope for the best")
        return True
        
def get_python_version():
    return f"{sys.version_info.major}.{sys.version_info.minor}"

def find_libpython():
    log.debug("Looking for libpython...")
    lib = find_libpython_ctypes()
    if lib:
        log.debug("Found via ctypes: " + lib)
        return lib
    else:
        log.debug("Nothing found via ctypes, falling back to ldconfig")
        try:
            return find_libpython_ldconfig()
        except Exception as e:
            log.debug("Error using ldconfig: " + str(e))
            return None

def find_libpython_ctypes():
    python_version = f'{sys.version_info.major}.{sys.version_info.minor}'

    # Try different names for the shared library
    library_names = [
        f'libpython{python_version}.so.1.0',
        f'libpython{python_version}m.so.1.0',
        f'libpython{python_version}.so',
        f'libpython{python_version}m.so',
        ]
    
    for library_name in library_names:
        library_path = ctypes.util.find_library(library_name)
        if library_path is not None:
            return library_path    
    return None

def find_libpython_ldconfig():
    python_version = f'{sys.version_info.major}.{sys.version_info.minor}'
    library_names = [
        f'libpython{python_version}.so',
        f'libpython{python_version}m.so',
        ]

    # Get the output of `ldconfig -p`
    output = subprocess.check_output(['ldconfig', '-p']).decode()

    # Go through each line in the output
    lines = output.split('\n')
    log.debug("Checking %s libraries from ldconfig" % len(lines))
    for line in lines:
        line = line.strip()
        for target in library_names:
            if target in line:
                log.debug("Found %s in %s" %(target, line))
                return line.split()[-1]
    log.debug("Did not find a libpython in ldconfig output")
    return None
                                                                                        

def check_pip():
    try:
        import pip
        return True
    except ImportError:
        return False

def install_package(package):
    subprocess.check_call([sys.executable, "-m", "pip", "install", package])
        
def is_apt_based():
    try:
        contents = None
        with open('/etc/os-release') as f:
            contents = f.read().lower()
        if 'ubuntu' in contents:
            return True
        elif 'debian' in contents:
            return True
        else:
            return False
    except FileNotFoundError:
        return False

def get_libpython_apt_install_cmd():
    return f"apt-get install -y -qq libpython{get_python_version()}"

def get_libpython_install_cmd():
    if is_apt_based():
        return get_libpython_apt_install_cmd()
    else:
        log.debug("OS is not apt based, don't know how to suggest installing")

def find_duckdb():
    ddb = shutil.which('duckdb')
    if ddb:
        log.debug("Found duckdb via shutil.which()")
        return ddb
    else:
        # If it isn't in our path, make sure it wasn't curl'd into our CWD.
        possible_path = os.path.join(os.getcwd(), 'duckdb')
        if os.path.isfile(possible_path) and os.access(possible_path, os.X_OK):
            log.debug("Found in cwd")
            return possible_path
    log.debug("No DuckDB binary found")
    return None

def generate_install_sql(extension_version):
    python_version = get_python_version()
    sql_script = f"""
SET custom_extension_repository='net.ednit.duckdb-extensions.s3.us-west-2.amazonaws.com/pytables/{extension_version}/python{python_version}';
INSTALL pytables;
LOAD pytables;
"""
    return sql_script

def run_query(ddb, query):
    log.debug("Running DuckDB Query")
    process = subprocess.Popen(ddb, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    b_stdout, b_stderr = process.communicate(query.encode())
    stdout = b_stdout.decode()
    stderr = b_stderr.decode()
    print(stdout)
    print(stderr)
    for line in (l.strip() for l in stdout.split("\n") if l.strip()):
        log.debug("stdout: " + line)
    for line in (l.strip() for l in stderr.split("\n") if l.strip()):
        log.debug("stderr: " + line)
    return process.returncode
                
def main(argv):
    program = os.path.basename(argv[0])
    parser = argparse.ArgumentParser(prog = program, description='Installs pytables, a DuckDB extension')
    parser.add_argument('--ext-version', default = 'latest', dest = 'extension_version')
    args = parser.parse_args()
    

    log.basicConfig(filename='/tmp/' + program + '.log', filemode='a',
                    level=log.DEBUG,
                    format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p')
    log.Formatter.converter = time.gmtime
    log.debug("Starting installation")

    we_are_good = check_os()
    if not we_are_good:
        log.debug("Failed OS system check")
        return 1
    
    ddb_path = find_duckdb()
    if not ddb_path:
        log.debug("No DuckDB binary found, exitting")
        print("Unable to find a DuckDB binary. If you've already downloaded it, did you place it in your $PATH?")
        print("If you havent' downloaded it, grab a copy here: https://duckdb.org/docs/installation/")
        return 1

    libpython = find_libpython()
    if not libpython:
        log.debug("No libpython gound")
        install_cmd = get_libpython_install_cmd()

        if install_cmd:
            print(f"Unable to find the needed python{get_python_version()}.so shared library, though this man not necessarily mean there is a problem. If you see a simliar error when you load the pytables extension, you'll need to install this library. If you installed Python via your package manager, you can install the shared library with the following command:")
            print(install_cmd)
        else:
            print(f"Unable to find the needed python{get_python_version()}.so shared library, thought his may not necessarily mean there is a problem. If you see a simliar error when you load the pytables extension, you'll need to install this library. If you installed Python via a package, check if there is an additional libpython package.")
            print("Alternatively, if you've compiled your Python interpretter from source, make sure you specify the shared library is installed by running `./configure --enable-shared` before compiling.")

    ddb = [ddb_path, '-unsigned']
    query = generate_install_sql(args.extension_version)
    result = run_query(ddb, query)
    if result:
        log.debug("Encountered an error installing the extension")
        print("Error installing extension")
        return 1

    # Install the Python Package
    if not check_pip():
        print("No 'pip' module found. You may need to install an additional package. Once you have gotten Pip configured, run:")
        print("pip install ducktables")
    else:
        install_package('ducktables')
    
if __name__ == '__main__':
    sys.exit(main(sys.argv))
