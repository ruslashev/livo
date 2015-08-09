varying vec2 uv;

uniform sampler2D tex;
uniform vec4 color;

void main() {
	gl_FragColor = vec4(1, 1, 1, texture2D(tex, uv).a) * color;
}

