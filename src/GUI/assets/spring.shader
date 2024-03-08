#define PI 3.14159

uniform vec2 u_resolution;
uniform float u_length[MAXSPRINGS];
uniform float u_rms[RMS_BUFFER_SIZE*MAXSPRINGS];
uniform int u_rmspos;

void main() {
    vec2 st = (2.*gl_FragCoord.xy-u_resolution)/u_resolution.xy;

    float w = 0.358;
    float f = 3.4/100.0;
    float h = 0.86;

    st.y = (1-st.y)/2.f * float(MAXSPRINGS);
    int spring = int(st.y);
    st.y = fract(st.y)*2.0-1.0;
    
    float xpos = u_length[spring] * float(RMS_BUFFER_SIZE)*(st.x+1.0)/2.0;
    int ixpos = int(xpos);
    float fxpos = xpos - float(ixpos);
    int ixpos0 = (u_rmspos-ixpos)&(RMS_BUFFER_SIZE-1);
    int ixpos1 = (u_rmspos-(ixpos+1))&(RMS_BUFFER_SIZE-1);
    float rms0 = u_rms[ixpos0*MAXSPRINGS+spring];
    float rms1 = u_rms[ixpos1*MAXSPRINGS+spring];
    float rms = rms0 + fxpos*(rms1 - rms0);

    rms *= .3;
    
    float win = pow(cos(st.x*PI/2.), 0.8);
    
    //float xshift = sin(u_time*8.)*pow((sin(u_time*1.)*.5+.5),.4)*0.1;
    float xshift = -rms * win;
    st.x += xshift;
    
    float th = 2.*PI*st.x*u_resolution.x*f;
    float cth = cos(th);
    float sth = sin(th);
    st.y += sth*h;
    
    float d = abs(st.y);
    //d -= mix(0.*w, w, pow(abs(cth),2.2));
    
    w = mix(.0*w,w, pow(abs(cth),2.2));
    w /= (1+abs(dFdx(xshift))*50.0);
    float dAA = fwidth(d);
    
    float c = smoothstep(w+dAA, w-dAA,d);
    float alpha = c;
    //c *= smoothstep(-0.1,0.1,cth)*.5+.5;
    c *= mix(0.0,1., cth*.5+.5);
    //c *= mix(1.,0.28,pow(d,1.8));
    //c *= mix(0.1,1., pow(.5*cos(th)+.5,1.8));
    vec3 color = vec3(c);

    color = mix(vec3(0.1,0.1,0.12), color, alpha);
    gl_FragColor = vec4(color,1.0);
}
