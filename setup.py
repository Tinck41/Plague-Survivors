import argparse
import os
import subprocess
import shutil

from utils import compile_shaders


if __name__ == '__main__':
	buildFolder = 'build'
	repoDir = os.path.abspath(os.path.dirname(os.path.realpath(__file__)))
	buildDir = os.path.join(repoDir, buildFolder)

	GENERATOR_ALIASES = {
		"vs19": "Visual Studio 16 2019",
		"vs22": "Visual Studio 17 2022",
		"ninja": "Ninja",
		"make": "Unix Makefiles",
		"xcode": "Xcode",
	}

	parser = argparse.ArgumentParser(prog='Plague: Survivors', formatter_class=argparse.RawDescriptionHelpFormatter)
	parser.add_argument('-vim', action='store_true', help='create compile commands', default=False)
	parser.add_argument('-rel', action='store_true', help='release configuration', default=False)
	parser.add_argument('-clean', action='store_true', help='remove the build folder', default=False)
	parser.add_argument('-b', action='store_true', help='build project', default=False)
	parser.add_argument('generator', type=str, metavar='generator', help='generator name', choices=GENERATOR_ALIASES.keys(), default='ninja')
	parser.add_argument('platform', type=str, metavar='platform', help='platform name', default='')
	args = parser.parse_args()

	compile_shaders.compile()

	buildDir = os.path.join(buildDir, args.generator)

	print(buildDir)

	if args.clean == True:
		if os.path.isdir(buildDir):
			shutil.rmtree(buildDir)

	if args.platform == 'win32':

		cmake = ['cmake']

		cmake.append('-S .')
		cmake.append('-B ' + buildDir)
		cmake.append('-G ' + GENERATOR_ALIASES[args.generator])
		cmake.append(('-DCMAKE_BUILD_TYPE=Debug', '-DCMAKE_BUILD_TYPE=Release')[args.rel])

		print(cmake)
		subprocess.run(cmake)

		if args.b:
			subprocess.run(['cmake', '--build', buildDir])

	if args.platform == 'mac':

		cmake = ['cmake']

		cmake.append('-S .')
		cmake.append('-B ' + buildDir)
		cmake.append('-G ' + GENERATOR_ALIASES[args.generator])
		cmake.append(('-DCMAKE_BUILD_TYPE=Debug', '-DCMAKE_BUILD_TYPE=Release')[args.rel])
		cmake.append(('-DCMAKE_EXPORT_COMPILE_COMMANDS=False', '-DCMAKE_EXPORT_COMPILE_COMMANDS=True')[args.vim])

		print(cmake)
		subprocess.run(cmake)

		if args.vim:
			subprocess.run(['mv', './build/' + args.generator + '/compile_commands.json', './'])

		if args.b:
			subprocess.run(['cmake', '--build', buildDir])

