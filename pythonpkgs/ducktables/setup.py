from setuptools import setup, find_packages


with open('version.txt') as f:
    version_str = f.read().strip()

from pathlib import Path
this_directory = Path(__file__).parent
long_description = (this_directory / "README.md").read_text()
   
setup(
    name='ducktables',
    version='0.1.2',
    description='Python Table Functions for DuckDB',
    packages=find_packages(),
    install_requires=open('requirements.txt').read().splitlines(),
    long_description=long_description,
    long_description_content_type='text/markdown'
    )
