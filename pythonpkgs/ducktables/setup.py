from setuptools import setup, find_packages

setup(
    name='ducktables',
    version='0.1.1',
    description='Python Table Functions for DuckDB',
    packages=find_packages(),
    install_requires=open('requirements.txt').read().splitlines(),
    )
