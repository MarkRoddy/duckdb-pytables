"""Installs the Extension"""

import os, sys
import time
import shutil
import subprocess
import ctypes.util
import subprocess
import logging as log

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

def generate_install_sql():
    python_version = get_python_version()
    sql_script = f"""
SET custom_extension_repository='net.ednit.duckdb-extensions.s3.us-west-2.amazonaws.com/pytables/latest/python{python_version}';
INSTALL pytables;
LOAD pytables;
"""
    return sql_script

def run_query(ddb, query):
    process = subprocess.Popen(ddb, stdin=subprocess.PIPE)
    process.communicate(query.encode())
    return process.returncode
                
def main(args):
    log.basicConfig(filename='/tmp/' + args[0] + '.log', filemode='w',
                    level=log.DEBUG,
                    format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p')
    log.Formatter.converter = time.gmtime
    ddb_path = find_duckdb()
    if not ddb_path:
        print("Unable to find a DuckDB binary. If you've already downloaded it, did you place it in your $PATH?")
        print("If you havent' downloaded it, grab a copy here: https://duckdb.org/docs/installation/")
        return 1

    libpython = find_libpython()
    if not libpython:
        install_cmd = get_libpython_install_cmd()

        if install_cmd:
            print(f"Unable to find the needed python{get_python_version()}.so shared library, though this man not necessarily mean there is a problem. If you see a simliar error when you load the pytables extension, you'll need to install this library. If you installed Python via your package manager, you can install the shared library with the following command:")
            print(install_cmd)
        else:
            print(f"Unable to find the needed python{get_python_version()}.so shared library, thought his may not necessarily mean there is a problem. If you see a simliar error when you load the pytables extension, you'll need to install this library. If you installed Python via a package, check if there is an additional libpython package.")
            print("Alternatively, if you've compiled your Python interpretter from source, make sure you specify the shared library is installed by running `./configure --enable-shared` before compiling.")

    ddb = [ddb_path, '-unsigned']
    query = generate_install_sql()
    result = run_query(ddb, query)
    if result:
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
