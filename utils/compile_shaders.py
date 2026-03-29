import os
import subprocess

def compile():
	subprocess.run([os.path.expanduser('~/Downloads/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross'), 'assets/shaders/src/camera_composition.vert.hlsl', '-o', 'assets/shaders/out/camera_composition.vert.msl'])
	subprocess.run([os.path.expanduser('~/Downloads/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross'), 'assets/shaders/src/camera_composition.frag.hlsl', '-o', 'assets/shaders/out/camera_composition.frag.msl'])

	subprocess.run([os.path.expanduser('~/Downloads/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross'), 'assets/shaders/src/ui.vert.hlsl', '-o', 'assets/shaders/out/ui.vert.msl'])
	subprocess.run([os.path.expanduser('~/Downloads/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross'), 'assets/shaders/src/ui.frag.hlsl', '-o', 'assets/shaders/out/ui.frag.msl'])

	subprocess.run([os.path.expanduser('~/Downloads/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross'), 'assets/shaders/src/sprite.vert.hlsl', '-o', 'assets/shaders/out/sprite.vert.msl'])
	subprocess.run([os.path.expanduser('~/Downloads/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross'), 'assets/shaders/src/sprite.frag.hlsl', '-o', 'assets/shaders/out/sprite.frag.msl'])

	subprocess.run([os.path.expanduser('~/Downloads/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross'), 'assets/shaders/src/shader_batch.vert.hlsl', '-o', 'assets/shaders/out/shader_batch.vert.msl'])
	subprocess.run([os.path.expanduser('~/Downloads/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross'), 'assets/shaders/src/shader_batch.frag.hlsl', '-o', 'assets/shaders/out/shader_batch.frag.msl'])

	subprocess.run([os.path.expanduser('~/Downloads/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross'), 'assets/shaders/src/sprite_batch.vert.hlsl', '-o', 'assets/shaders/out/sprite_batch.vert.msl'])
	subprocess.run([os.path.expanduser('~/Downloads/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross'), 'assets/shaders/src/sprite_batch.frag.hlsl', '-o', 'assets/shaders/out/sprite_batch.frag.msl'])

	subprocess.run([os.path.expanduser('~/Downloads/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross'), 'assets/shaders/src/text.vert.hlsl', '-o', 'assets/shaders/out/text.vert.msl'])
	subprocess.run([os.path.expanduser('~/Downloads/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross'), 'assets/shaders/src/text.frag.hlsl', '-o', 'assets/shaders/out/text.frag.msl'])
	subprocess.run([os.path.expanduser('~/Downloads/SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross'), 'assets/shaders/src/text_sdf.frag.hlsl', '-o', 'assets/shaders/out/text_sdf.frag.msl'])

if __name__ == '__main__':
	compile();
