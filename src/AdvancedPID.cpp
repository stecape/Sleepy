#include "AdvancedPID.h"
#include <string.h>
#include <math.h>

static float saturate(float val, float min, float max) {
    if (val > max) return max;
    if (val < min) return min;
    return val;
}

float PID_Mngt(
    PID_Handle *pid,
    float setpoint,
    float measure,
    float reference,
    bool stop,
    bool manual_mode,
    bool derivativeEnable,
    bool awEnable,
    float manual_output
) {
    if (!pid) return 0.0f;
    PID_Params *p = &pid->params;
    PID_State *s = &pid->state;

    // Gestione modalità manuale/automatica con debounce
    if (manual_mode != s->manual_mode) {
        s->pending_mode = manual_mode;
        s->debounce_counter = 0;
    }
    s->manual_output = manual_output;
    if (s->pending_mode != s->manual_mode) {
        s->debounce_counter++;
        if (s->debounce_counter >= s->debounce_threshold) {
            s->manual_mode = s->pending_mode;
            s->debounce_counter = 0;
        }
    } else {
        s->debounce_counter = 0;
    }

    // --- Ramping setpoint per istanza PID ---
    if (!s->ramp_initialized) {
        s->setpoint_ramp = setpoint;
        s->ramp_initialized = true;
    }
    if (p->gradiente > 0.0f && p->dt > 0.0f) {
        float delta = setpoint - s->setpoint_ramp;
        float max_step = p->gradiente * p->dt;
        if (fabsf(delta) <= max_step) {
            s->setpoint_ramp = setpoint;
        } else {
            s->setpoint_ramp += (delta > 0 ? max_step : -max_step);
        }
    } else {
        s->setpoint_ramp = setpoint;
    }

    // --- Calcolo PID solo se non in manuale e non in stop ---
    float errore = s->setpoint_ramp - measure;
    float errore_scalato = errore * p->Kp;
    float out_p = 0.0f, out_i = 0.0f, out_d = 0.0f, out_pid = 0.0f, out_pid_sat = 0.0f;
    float out_tot = 0.0f, out_tot_sat = 0.0f, aw_term = 0.0f;
    
    if (stop) {
        // In stop: output rampato verso zero, priorità su manuale
        if (!s->output_ramp_initialized) {
            s->output_ramp = s->out_tot;
            s->output_ramp_initialized = true;
        }
        float delta = 0.0f - s->output_ramp;
        float max_step = (p->output_gradient > 0.0f && p->dt > 0.0f) ? (p->output_gradient * p->dt) : fabsf(delta);
        if (fabsf(delta) <= max_step) {
            s->output_ramp = 0.0f;
        } else {
            s->output_ramp += (delta > 0 ? max_step : -max_step);
        }
        s->integrale = 0.0f;
        s->errore_prec = 0.0f;
        s->setpoint_ramp = 0.0f;
        s->ramp_initialized = false;
        s->out_pid = 0.0f;
        out_tot_sat = s->output_ramp;
    } else if (s->manual_mode) {
        // In manuale: PID congelato, output = manual_output
        out_tot_sat = saturate(s->manual_output, p->ref_min, p->ref_max);
    } else {
        // Automatica
        out_p = p->Gp * errore_scalato;
        if (p->Ti > 0.0f) {
            s->integrale += (errore_scalato / p->Ti) * p->dt;
        }
        out_i = s->integrale;
        if (p->Td > 0.0f && p->dt > 0.0f && derivativeEnable) {
            // Derivata solo se abilitata e Td > 0
            out_d = p->Td * (errore_scalato - s->errore_prec) / p->dt;
        }
        out_pid = out_p + out_i + out_d;
        out_pid_sat = saturate(out_pid, p->out_min, p->out_max);
        out_tot = reference + out_pid_sat;
        out_tot_sat = saturate(out_tot, p->ref_min, p->ref_max);
        
        // Anti-windup: prende la parte eccedente (saturato - desiderato)
        // e la mette con segno invertito nell'integrale per sgonfiarlo
        if (out_pid != out_pid_sat) {
            aw_term = (out_pid_sat - out_pid);  // saturato - desiderato
        } else if (out_tot != out_tot_sat) {
            aw_term = (out_tot_sat - out_tot);  // saturato - desiderato
        }
        
        if (p->Taw > 0.0f && awEnable && aw_term != 0.0f) {
            // Somma il contributo anti-windup all'integrale
            float aw_correction = (aw_term / p->Taw) * p->dt;
            s->integrale += aw_correction;
        }
    }

    // --- Rampa sull'uscita (output_gradient) sempre attiva ---
    float out_ramp = s->out;
    float out_target = out_tot_sat;
    if (p->output_gradient > 0.0f && p->dt > 0.0f) {
        float delta = out_target - out_ramp;
        float max_step = p->output_gradient * p->dt;
        if (fabsf(delta) <= max_step) {
            out_ramp = out_target;
        } else {
            out_ramp += (delta > 0 ? max_step : -max_step);
        }
    } else {
        out_ramp = out_target;
    }

    // --- Aggiorna stato informativo ---
    s->error = errore;
    s->errore_prec = errore_scalato;
    s->proportionalCorrection = out_p;
    s->integralCorrection = out_i;
    s->antiWindupContribute = aw_term;
    s->derivativeCorrection = out_d;
    s->totalCorrection = out_pid;
    s->out_pid = out_pid_sat;
    s->rawOut = out_tot;
    s->out_tot = out_tot_sat;
    s->out = out_ramp;
    s->output_ramp_initialized = false;
    
    return out_ramp;
}

bool PID_SSR_Burst(PID_Handle *pid, uint32_t tick_10ms) {
    if (!pid) return false;
    // Uscita PID normalizzata 0-100%
    float output = pid->state.out;
    // Ogni step è 4% (1 semionda su 25)
    uint32_t n_on = (uint32_t)((output * 25.0f + 50.0f) / 100.0f); // arrotonda correttamente
    if (n_on > 25) n_on = 25;
    return (tick_10ms < n_on);
}

HeatingPWM_Return HeatingPWM(
    HeatingPWM_Instance *pwm,
    float current_duty,
    float duration,
    float cycle_time,
    float hysteresis,
    bool enable
) {
    HeatingPWM_Return ret = {false, false};
    bool positive_out = false;
    bool negative_out = false;
    
    if (!pwm) return ret; // Se non inizializzato, ritorna false
    
    // Se disabilitato, resetta tutto
    if (!enable) {
        pwm->duty_sum = 0;
        pwm->duty_samples = 0;
        pwm->duty_avg = 0;
        pwm->hysteresis_forcing = false;
        pwm->forced_state = false;
        pwm->last_enable = false;
        pwm->elapsed = 0;
        ret.positive_out = false;
        ret.negative_out = false;
        return ret;
    }
    
    // Se appena abilitato, resetta accumulo
    if (!pwm->last_enable && enable) {
        pwm->duty_sum = 0;
        pwm->duty_samples = 0;
        pwm->elapsed = 0;
        pwm->hysteresis_forcing = false;
        pwm->forced_state = false;
    }
    pwm->last_enable = enable;
    
    // Accumula duty per la media
    pwm->duty_sum += current_duty;
    pwm->duty_samples++;
    pwm->elapsed += cycle_time;
    
    // Quando termina il ciclo di regolazione
    if (pwm->elapsed >= duration) {
        pwm->duty_avg = pwm->duty_sum / (pwm->duty_samples > 0 ? pwm->duty_samples : 1);
        pwm->duty_sum = 0;
        pwm->duty_samples = 0;
        pwm->elapsed = 0;
        
        // Logica isteresi
        if (fabsf(pwm->duty_avg) <= hysteresis/duration*100.0f) {
            pwm->forced_state = true; 
            pwm->hysteresis_forcing = false; // Forza OFF
        } else if (fabsf(pwm->duty_avg) >= 100.0f - hysteresis/duration*100.0f) {
            pwm->forced_state = true; 
            pwm->hysteresis_forcing = true; // Forza ON
        } else {
            pwm->forced_state = false; // Non forzato
        }
    }
    
    // Uscite
    if (pwm->forced_state) {
        // Durante isteresi, mantieni stato forzato
        positive_out = pwm->duty_avg > 0.0f && pwm->hysteresis_forcing;
        negative_out = pwm->duty_avg < 0.0f && pwm->hysteresis_forcing;
    } else {
        // Normale: ON se duty_avg > 0, OFF se < 0
        if (pwm->elapsed/duration*100.0f <= fabsf(pwm->duty_avg)) {
            positive_out = (pwm->duty_avg > 0.0f);
            negative_out = (pwm->duty_avg < 0.0f);
        } else {
            positive_out = false;
            negative_out = false;
        }
    }
    
    ret.positive_out = positive_out;
    ret.negative_out = negative_out;
    return ret;
}
