from setuptools import setup, find_packages


with open('version.txt') as f:
    version_str = f.read().strip()
setup(
    name='ducktables',
    version='0.1.2',
    description='Python Table Functions for DuckDB',
    packages=find_packages(),
    install_requires=open('requirements.txt').read().splitlines(),
    )
