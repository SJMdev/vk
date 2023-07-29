#version 430 core 

in float particle_lifetime; 
out vec4 colour_out;

void main(){
	
	if (particle_lifetime < 0.1) {
		// Particles that are almost dead, we blend towards black.
		colour_out = mix(
				vec4(vec3(0.0),1.0),
				vec4(0.0,0.5,1.0,1.0),
				particle_lifetime*10.0);  
	} else if (particle_lifetime > 0.9) {
		// Newborn particles come from black.
		colour_out = mix(vec4(0.6,0.05,0.0,1.0), vec4(vec3(0.0),1.0), (particle_lifetime-0.9)*10.0);  
	} else {
		// regular lifetime is modeled from red to blue.
		colour_out = mix(vec4(0.0,0.5,1.0,1.0), vec4(0.6,0.05,0.0,1.0), particle_lifetime);
	}
	
}