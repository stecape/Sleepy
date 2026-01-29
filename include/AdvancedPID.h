#ifndef ADVANCEDPID_H
#define ADVANCEDPID_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Struttura parametri PID
typedef struct {
    float Kp;               // Guadagno principale [% output / °C]
    float Gp;               // Guadagno proporzionale (default 1, adimensionale)
    float Ti;               // Tempo di integrazione [s]
    float Td;               // Tempo di derivazione [s]
    float Taw;              // Tempo anti-windup [s]
    float dt;               // Sample time [s]
    float out_min;          // Limite correzione PID
    float out_max;
    float ref_min;          // Limite uscita totale (reference+correzione)
    float ref_max;
    float gradiente;        // Variazione massima setpoint al secondo (0 = disabilitato)
    float output_gradient;  // Variazione massima uscita al secondo (rampa output, 0 = disabilitato)
} PID_Params;

// Stato interno PID
typedef struct {
    float error;                    // Errore attuale (setpoint - measure)
    float errore_prec;              // Errore precedente - Errore scalato da Kp - (per derivata)
    float proportionalCorrection;   // Contributo proporzionale
    float integralCorrection;       // Contributo integrale
    float antiWindupContribute;     // Contributo anti-windup
    float derivativeCorrection;     // Contributo derivativo
    float totalCorrection;          // Somma P+I+D (pre-saturazione)
    float out_pid;                  // correzione PID (post saturazione)
    float rawOut;                   // Correzione PID saturata + reference (pre-saturazione totale)
    float out_tot;                  // uscita totale (reference+correzione, post-saturazione)
    float out;                      // Uscita dopo la rampa
    float integrale;                // Stato interno integrale
    float setpoint_ramp;            // setpoint rampato (per istanza PID)
    bool ramp_initialized;          // flag inizializzazione ramp
    float output_ramp;              // uscita rampata (per stop)
    bool output_ramp_initialized;
    // --- campi operativi per modalità/debounce ---
    bool manual_mode;
    float manual_output;
    int debounce_counter;
    int debounce_threshold;
    bool pending_mode;
} PID_State;

// Handle PID
// Contiene solo parametri e stato interno
typedef struct {
    PID_Params params;      // Parametri del PID (Kp, Ti, Td, limiti, ecc.)
    PID_State state;        // Stato interno (integrale, errore, ramp, ecc.)
} PID_Handle;

#define PID_DEBOUNCE_DEFAULT 4

/**
 * Gestisce un burst SSR basato sull'uscita PID (0-100%)
 * Divide il periodo da 500ms (50Hz) in 25 semicicli da 10ms
 * 
 * @param pid Handle PID
 * @param tick_10ms Contatore 0-24 (reset ogni 250ms per 50Hz)
 * @return true se SSR deve essere ON in questo tick
 */
bool PID_SSR_Burst(PID_Handle *pid, uint32_t tick_10ms);

/**
 * Funzione monolitica PID: tutti i comandi operativi/modalità sono passati come argomenti.
 * Le variabili di servizio (debounce, pending_mode, ecc.) sono gestite internamente.
 *
 * @param pid Handle PID (contiene solo parametri e stato)
 * @param setpoint Setpoint richiesto
 * @param measure Misura attuale
 * @param reference Feedforward/reference
 * @param stop Se true, esegue la rampa di stop
 * @param manual_mode Se true, forza la modalità manuale
 * @param derivativeEnable Se true, abilita il termine derivativo
 * @param awEnable Se true, abilita l'anti-windup
 * @param manual_output Uscita manuale (se in manuale)
 * @return Uscita PID (o manuale)
 */
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
);

// Struttura dati per HeatingPWM multistanza
// Permette la gestione di più uscite PWM SSR indipendenti con rolling average e isteresi
typedef struct {
    float duty_sum;          // Somma dei duty per la media
    int duty_samples;        // Numero di campioni accumulati
    float duty_avg;          // Media duty ultimo ciclo
    bool hysteresis_forcing; // true: uscita forzata ON per isteresi, false: forzata OFF
    bool forced_state;       // true: uscita forzata da isteresi, false: normale
    bool last_enable;        // Stato enable precedente
    float elapsed;           // Tempo trascorso nel ciclo attuale
} HeatingPWM_Instance;

// Struttura di ritorno per HeatingPWM: uscite positive/negative
// positive_out: true se attiva uscita positiva (riscaldamento)
// negative_out: true se attiva uscita negativa (raffreddamento)
typedef struct {
    bool positive_out;
    bool negative_out;
} HeatingPWM_Return;

/**
 * Gestisce un'uscita PWM per carichi termici (SSR) con rolling average e isteresi temporale.
 * Permette di evitare commutazioni troppo frequenti, forzando ON/OFF per un tempo minimo se il duty medio
 * supera soglie definite, e calcola la media del duty su un periodo configurabile.
 *
 * @param pwm           Puntatore all'istanza HeatingPWM_Instance (stato interno della PWM)
 * @param current_duty  Duty cycle corrente (percentuale, positivo=riscalda, negativo=raffredda)
 * @param duration      Durata del ciclo di regolazione (secondi, es. 60.0)
 * @param cycle_time    Tempo tra due chiamate successive (secondi, es. 1.0)
 * @param hysteresis    Durata isteresi ON/OFF forzato (secondi)
 * @param enable        Abilita/disabilita la PWM (true=attiva, false=resetta tutto)
 * @return              HeatingPWM_Return: struct con positive_out e negative_out (true se ON, false se OFF)
 *
 * Logica:
 *   - Se duty medio vicino a zero: forzatura OFF (nessuna uscita attiva per isteresi)
 *   - Se duty medio vicino a +100% o -100%: forzatura ON (uscita positiva o negativa attiva per isteresi)
 *   - Fuori isteresi: uscita attiva solo per una frazione del ciclo proporzionale al valore assoluto della media
 *   - Se duty medio positivo: attiva solo positive_out; se negativo: solo negative_out
 */
HeatingPWM_Return HeatingPWM(
    HeatingPWM_Instance *pwm,
    float current_duty,
    float duration,
    float cycle_time,
    float hysteresis,
    bool enable
);

#ifdef __cplusplus
}
#endif

#endif // ADVANCEDPID_H
