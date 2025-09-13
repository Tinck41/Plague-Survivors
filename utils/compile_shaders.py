import os
import subprocess

def compile():
	subprocess.run([os.path.expanduser('~/Downloads/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross'), 'assets/shaders/src/sprite.vert.hlsl', '-o', 'assets/shaders/out/sprite.vert.msl'])
	subprocess.run([os.path.expanduser('~/Downloads/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross'), 'assets/shaders/src/sprite.frag.hlsl', '-o', 'assets/shaders/out/sprite.frag.msl'])

if __name__ == '__main__':
	subprocess.run([os.path.expanduser('~/Downloads/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross'), '../assets/shaders/src/sprite.vert.hlsl', '-o', '../assets/shaders/out/sprite.vert.msl'])
	subprocess.run([os.path.expanduser('~/Downloads/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross'), '../assets/shaders/src/sprite.frag.hlsl', '-o', '../assets/shaders/out/sprite.frag.msl'])
