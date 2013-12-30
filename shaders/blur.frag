#version 140

precision highp float; // needed only for version 1.30

uniform float texel_size; // the width or height of a single texel.
uniform sampler2D screenBuffer; // image to sample from to create blur
uniform sampler1D gaussian; // precomputed gaussian to sample from
// 2D blur is best achieved as 2 passes of 1D blurs: one horizontal and one vertical.
uniform int orientation; // 0 is horizontal; 1 is vertical; all others do nothing
uniform int samples; // number of samples to take (should vary ~linearly with blur_width)
uniform float blur_width; // width (in pixels) of the gaussian

in vec2 ex_UV;
out vec4 out_Color;

void main(void) {
    // easier to work with +/- half-widths
    float p, x, multiplier = 1.0;
    vec4 src;
    // horizontal pass (gaussian is a function of x)
    if(orientation == 0){
        for(int i = 0; i < samples; i++){
            p = float(i) / float(samples-1);
            multiplier = texture(gaussian, p);
            x = (p - 0.5) * blur_width * texel_size;
            src = texture(screenBuffer, ex_UV + vec2(x, 0.0));
            out_Color += src * multiplier;
        }
    }
    // horizontal pass (gaussian is a function of y)
    else if(orientation == 1){
        for(int i = 0; i < samples; i++){
            p = float(i) / float(samples-1);
            multiplier = texture(gaussian, p);
            x = (p - 0.5) * blur_width * texel_size;
            src = texture(screenBuffer, ex_UV + vec2(0.0, x));
            out_Color += src * multiplier;
        }
    }
}
