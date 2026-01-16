# Sistema di Controllo Temperatura - Documentazione

## Caratteristiche Implementate

### Navigazione Multi-Pagina
- **Long Press (800ms)** sull'encoder per cambiare pagina tra TIMER e HEATING
- Header sempre visibile mostra la pagina corrente (evidenziata in giallo)

### Pagina TIMER
Funzionalità originale del timer:
- Impostazione ore, minuti, secondi
- START/PAUSE
- RESET
- Output su D3

### Pagina HEATING (Controllo Temperatura)
Funzionalità complete per controllo temperatura acquario:

#### Campi Navigabili
1. **Setpoint** (10-50°C)
   - Editabile con encoder (incrementi di 0.5°C)
   - Click per entrare/uscire da modalità editing (rosso)
   
2. **Actual** (temperatura attuale)
   - Lettura da sensore NTC su A0
   - Filtro media mobile su 10 campioni
   - Aggiornamento ogni 500ms

3. **Output** (percentuale PID)
   - Mostra l'output del controllore PID (0-100%)
   - Rosso quando attivo, bianco quando spento

4. **ENABLE/DISABLE**
   - Attiva/disattiva controllo temperatura
   - Click per toggle

## Hardware Setup

### Sensore NTC (A0)
```
VCC (3.3V) ---[R_series 10kΩ]--- A0 ---[NTC 10kΩ]--- GND
```

Parametri NTC (modificabili nel setup):
- R0 = 10kΩ a 25°C
- Beta = 3950
- R_series = 10kΩ

### Relè Resistenza (D4)
- Output: GPIO2 (D4)
- Controllo PWM time-based:
  - Periodo: 30 secondi (100% duty cycle)
  - Durata minima: 1 secondo (protezione relè)
  - Pulsi < 1s vengono soppressi

## Controllore PID

### Parametri Default
```cpp
Kp = 10.0   // Proportional gain
Ki = 0.5    // Integral gain
Kd = 5.0    // Derivative gain
```

### Anti-Windup
Implementato con doppia strategia:
1. **Conditional Integration**: integrale aggiornato solo se:
   - Output non saturo, OPPURE
   - Error riduce la saturazione
   
2. **Integral Clamping**: integrale limitato a ±(100/Ki)

### Tuning Procedure

#### 1. Test Risposta al Gradino
```cpp
// In setup(), cambia parametri PID:
tempController.setPIDParameters(10.0, 0.0, 0.0);  // Solo P
```
- Imposta setpoint +5°C rispetto a temperatura ambiente
- Osserva se:
  - Troppo lento: aumenta Kp
  - Oscilla troppo: riduci Kp

#### 2. Aggiungere Integrale
```cpp
tempController.setPIDParameters(Kp_ottimale, 0.5, 0.0);
```
- Partire con Ki piccolo (0.1-0.5)
- Aumentare gradualmente fino a eliminare offset steady-state
- Se oscilla: ridurre Ki

#### 3. Aggiungere Derivativo (opzionale)
```cpp
tempController.setPIDParameters(Kp, Ki, 5.0);
```
- Kd aiuta a ridurre overshoot
- Può amplificare rumore di misura
- Partire con Kd = Kp/2

#### 4. Fine-Tuning per Acquario
Tipicamente per un acquario 60W:
- Sistema molto lento (inerzia termica alta)
- Suggerimenti:
  - Kp basso (5-15)
  - Ki molto basso (0.1-0.5)
  - Kd moderato (2-10)

### Parametri Ottimali Stimati
```cpp
// Per acquario ~50-100L con resistenza 60W:
tempController.setPIDParameters(8.0, 0.3, 4.0);
```

## Limiti di Sicurezza

### Temperatura
- Setpoint limitato: 10-50°C (codice)
- Per acquari tropicali: tipicamente 24-28°C

### Relè
- Tempo minimo ON: 1 secondo
- Periodo ciclo: 30 secondi
- Questo protegge il relè da switching eccessivo

### Lettura Sensore
- Filtro media mobile: 10 campioni
- Frequenza lettura: 2 Hz (500ms)
- Riduce rumore ADC

## Troubleshooting

### Temperatura Non Letta Correttamente
1. Verifica cablaggio NTC e resistore serie
2. Controlla valore resistore serie (deve essere ~10kΩ)
3. Verifica parametri Beta del tuo NTC specifico
4. Misura tensione su A0 con multimetro (deve essere 0.3-0.7V a 25°C)

### Relè Non Commuta
1. Verifica pin D4 con LED di test
2. Controlla che output% > 3.3% (1s su 30s)
3. Verifica alimentazione relè

### Oscillazioni Temperatura
1. Riduci Kp
2. Riduci Ki
3. Aumenta leggermente Kd
4. Verifica isolamento termico acquario

### Risposta Troppo Lenta
1. Aumenta Kp (ma occhio oscillazioni)
2. Verifica potenza resistenza adeguata
3. Controlla che relè commuti effettivamente

## Modifiche Avanzate

### Cambio Parametri NTC
```cpp
// In setup():
ntcReader.setNTCParameters(
    10000.0,  // R0 (Ohm)
    25.0,     // T0 (°C)
    3950.0,   // Beta
    10000.0   // R_series (Ohm)
);
```

### Cambio Periodo PWM
```cpp
// In TemperatureController.h:
static const unsigned long CYCLE_PERIOD_MS = 60000;  // 60s invece di 30s
```

### Disabilitare Protezione Relè Minima
```cpp
// In TemperatureController.h:
static const unsigned long MIN_PULSE_MS = 0;  // Rimuove limite
// ATTENZIONE: può ridurre vita relè!
```

## Log e Debug

Per abilitare debug seriale (richiede rimuovere display da TX/RX):
```cpp
// In main.cpp setup():
Serial.begin(115200);
Serial.println("Debug enabled");

// In loop(), aggiungi:
if (tempEnabled) {
    Serial.print("Temp: ");
    Serial.print(tempActual);
    Serial.print(" / ");
    Serial.print(tempSetpoint);
    Serial.print(" | Output: ");
    Serial.print(tempController.getOutput());
    Serial.println("%");
}
```

## Note Finali

- Sistema testato in simulazione, richiede tuning con hardware reale
- Periodo 30s è un buon compromesso per acquari
- Anti-windup previene integral windup durante riscaldamento iniziale
- Lettura NTC assume voltage divider con NTC verso GND
