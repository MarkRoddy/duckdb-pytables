"""Installs the Extension"""

import os, sys
import shutil
import subprocess
import ctypes.util
import subprocess

def get_python_version():
    return f"{sys.version_info.major}.{sys.version_info.minor}"

def find_libpython():
    library_name = f"python{get_python_version()}"
    library_path = ctypes.util.find_library(library_name)
    return library_path

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


def find_duckdb():
    ddb = shutil.which('duckdb')
    if not ddb:
        # If it isn't in our path, make sure it wasn't curl'd into our CWD.
        possible_path = os.path.join(os.getcwd(), 'duckdb')
        if os.path.isfile(possible_path) and os.access(possible_path, os.X_OK):
            ddb = possible_path
    return ddb

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
    libpython = find_libpython()
    if not libpython:
        install_cmd = get_libpython_install_cmd()

        if install_cmd:
            print(f"Unable to find the needed python{get_python_version()}.so shared library. If you installed Python via your package manager, you can install the shared library with the following command:")
            print(install_cmd)
        else:
            print(f"Unable to find the needed python{get_python_version()}.so shared library. If you installed Python via a package, check if there is an additional libpython package.")
            print("Alternatively, if you've compiled your Python interpretter from source, make sure you specify the shared library is installed by running `./configure --enable-shared` before compiling.")
        return 1
    ddb_path = find_duckdb()
    if not ddb_path:
        print("Unable to find a DuckDB binary. If you've already downloaded it, did you place it in your $PATH?")
        print("If you havent' downloaded it, grab a copy here: https://duckdb.org/docs/installation/")
        return 1
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
