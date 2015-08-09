varying vec2 uv;

uniform sampler2D texture;
uniform vec4 color;

void main() {
	gl_FragColor = vec4(1, 1, 1, texture2D(texture, uv).a) * color;
}

