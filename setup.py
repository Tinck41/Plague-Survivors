import sys
import argparse
import textwrap
import os
import subprocess
import platform


if __name__ == '__main__':
    cmake = ['cmake']

    cmake.append('-S .')
    cmake.append('-B ' + './build')
    cmake.append('-GNinja')
    cmake.append('-DCMAKE_BUILD_TYPE=Debug')
    cmake.append('-DCMAKE_EXPORT_COMPILE_COMMANDS=True')
#cmake.append(('-DCMAKE_BUILD_TYPE=Debug',	'-DCMAKE_BUILD_TYPE=Release')[args.rel])

    print(cmake)
    subprocess.run(cmake)
    subprocess.run(['mv', './build/compile_commands.json', './'])
    subprocess.run(['cmake', '--build', './build'])

