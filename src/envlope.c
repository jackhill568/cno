#include "envlope.h"


void env_init(Env *env, float sample_rate, float attck, float decay, float sus, float rel) {

	env->state = OFF;
	env->value = 0.0f;

	env->attackRate = 1.0f / (attck * sample_rate);
	env->decayRate = (1.0f - sus) / (decay * sample_rate);
	env->sustainLevel = sus;
	env->releaseRate = sus / (rel * sample_rate);

}


float env_process(Env *env) {
	switch (env->state) {
		case ATTACK:
			env->value+=env->attackRate;
			if (env->value >=1.0f ) {
				env->value = 1.0f;
				env->state = DECAY;
			}
			break;
		case DECAY:
			env->value -= env->decayRate;
			if (env->value <= env->sustainLevel) {
				env->value = env->sustainLevel;
				env->state = SUSTAIN;
			}
			break;
		case SUSTAIN:
			break;
		case RELEASE:
			env->value -= env->releaseRate;
			if (env->value <= 0.0f) {
				env->value = 0.0f;
				env->state = OFF;
			}
			break;
		case OFF:
			env->value = 0.0f;
			break;
		default:
			env->value = 0.0f;
			break;	
	}
	return env->value;
}










