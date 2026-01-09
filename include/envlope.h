#ifndef ENV_H
#define ENV_H

typedef enum {
	 OFF=0, ATTACK, DECAY, SUSTAIN, RELEASE,
} Env_state;


typedef struct {
	
	float value;
	float attackRate;
	float decayRate;
	float sustainLevel;
	float releaseRate;
	Env_state state;

} Env;


void env_init(Env *env, float sample_rate, float attck, float decay, float sus, float rel);

float env_process(Env *env);

#endif
