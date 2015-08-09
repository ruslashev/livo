attribute vec4 coord;
varying vec2 uv;

void main() {
	gl_Position = vec4(coord.xy, 0, 1);
	uv = coord.zw;
}

