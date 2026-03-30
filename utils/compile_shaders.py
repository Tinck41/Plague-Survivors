import os
import subprocess
import argparse

SRC_DIR = 'assets/shaders/src'
OUT_DIR = 'assets/shaders/out'

EXTENSIONS = {
	'msl': 'msl',
	'spirv': 'spv',
	'dxil': 'dxil',
}

def get_suppported_extensioins():
	return EXTENSIONS.keys()

def compile(ext: str, generator: str):
	SHADERCROSS = os.path.expanduser(f'build/{generator}/vendor/SDL_shadercross/shadercross')

	out_ext = EXTENSIONS[ext]
	os.makedirs(OUT_DIR, exist_ok=True)

	for root, _, files in os.walk(SRC_DIR):
		for file in files:
			if not file.endswith('.hlsl'):
				continue

			src_path = os.path.join(root, file)
			out_file = file.replace('.hlsl', f'.{out_ext}')
			out_path = os.path.join(OUT_DIR, out_file)

			print(f'Compiling {src_path} -> {out_path}')
			result = subprocess.run([SHADERCROSS, src_path, '-o', out_path])

			if result.returncode != 0:
				print(f'Failed to compile {src_path}')

if __name__ == '__main__':
	parser = argparse.ArgumentParser(description='Compile HLSL shaders')
	parser.add_argument('--ext', choices=EXTENSIONS.keys(), help='Output shader format')
	parser.add_argument('--g', help='project generator name')
	args = parser.parse_args()

	compile(args.ext, args.g)
