import argparse
import os
import subprocess
import shutil


if __name__ == '__main__':
	buildFolder = 'build'
	repoDir = os.path.abspath(os.path.dirname(os.path.realpath(__file__)))
	buildDir = os.path.join(repoDir, buildFolder)

	parser = argparse.ArgumentParser(prog='Plague: Survivors', formatter_class=argparse.RawDescriptionHelpFormatter)
	parser.add_argument('-vim', action='store_true', help='create compile commands', default=False)
	parser.add_argument('-rel', action='store_true', help='release configuration', default=False)
	parser.add_argument('-clean', action='store_true', help='remove the build folder', default=False)
	parser.add_argument('platform', type=str, metavar='platform', help='platform name', default='')
	args = parser.parse_args()

	if args.clean == True:
		if os.path.isdir(buildDir):
			shutil.rmtree(buildDir)

	if args.platform == 'win32':

		cmake = ['cmake']

		cmake.append('-S .')
		cmake.append('-B ' + buildFolder)
		cmake.append('-G Visual Studio 17 2022')
		cmake.append(('-DCMAKE_BUILD_TYPE=Debug', '-DCMAKE_BUILD_TYPE=Release')[args.rel])

		print(cmake)
		subprocess.run(cmake)
		subprocess.run(['cmake', '--build', buildFolder])

	if args.platform == 'mac':

		cmake = ['cmake']

		cmake.append('-S .')
		cmake.append('-B ' + buildFolder)
		cmake.append('-GNinja')
		cmake.append(('-DCMAKE_BUILD_TYPE=Debug', '-DCMAKE_BUILD_TYPE=Release')[args.rel])
		cmake.append(('-DCMAKE_EXPORT_COMPILE_COMMANDS=False', '-DCMAKE_EXPORT_COMPILE_COMMANDS=True')[args.vim])

		print(cmake)
		subprocess.run(cmake)

		if args.vim:
			subprocess.run(['mv', './build/compile_commands.json', './'])

		subprocess.run(['cmake', '--build', buildFolder])

