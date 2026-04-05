# Sleepy — CLAUDE.md

Progetto PlatformIO per ESP8266 (D1 Mini). Dispositivo fisico con display TFT, encoder rotativo, due uscite relè/SSR, sensore NTC. Funzioni principali: **timer conto alla rovescia** e **regolatore di temperatura PID**.

---

## Hardware

| Pin | Funzione |
|-----|----------|
| D0 (GPIO16) | Encoder button |
| D1 (GPIO5) | Encoder DT |
| D2 (GPIO4) | Encoder CLK |
| D3 (GPIO2) | Output TEMP CTRL (SSR riscaldamento) |
| D4 (GPIO0) | Output TIMER |
| D5/D6/D7/D8 | Display TFT SPI software (RES/DC/CS/LED) |
| TX/RX | Display TFT SPI software (SCK/MOSI) — Serial DISABILITATO |
| A0 | NTC termometro |

**Display:** ST7735/ST7789 via SPI software — nel codice chiamato "eink" per storia, ma è TFT.  
**NTC:** Aussel 10K Beta=3950, R0=10k@25°C, resistenza serie=10.28k (voltage divider: NTC tra 3.3V e ADC, serie a GND).  
**WiFi:** IP statico 192.168.2.58, SSID "Boring".  
**OTA:** hostname `SleepyESP8266`, upload via espota a 192.168.2.58.

---

## Struttura file

```
src/
  main.cpp              — setup/loop, logica display, encoder, integrazione moduli
  WebServer.cpp         — ESP8266WebServer, pagine HTML, API JSON, EEPROM R/W
  TemperatureController.cpp — wrapper PID: enable/disable, update, output PWM ciclico
  NTCReader.cpp         — lettura ADC, Steinhart-Hart, media mobile
  AdvancedPID.cpp       — algoritmo PID puro (C), anti-windup, debounce, rampe
  EinkDisplay.cpp       — rendering TFT (pagina timer, pagina riscaldamento)
  Menu.cpp              — navigazione encoder, cursore, editing, sleep
  Timer.cpp             — timer conto alla rovescia
  Encoder.cpp           — lettura encoder con debounce
  Output.cpp            — gestione uscita digitale timer

include/
  WebServer.h           — struct PIDParams (tutti i parametri persistiti)
  TemperatureController.h
  AdvancedPID.h         — PID_Handle, PID_Params, PID_State, HeatingPWM
  NTCReader.h, Menu.h, Timer.h, Encoder.h, Output.h, EinkDisplay.h
```

---

## Persistenza EEPROM

`PIDParams` (definita in `include/WebServer.h`) contiene tutti i parametri salvati.  
**`sizeof(PIDParams)` = 68 byte** (14 float + 2 unsigned long + 2 bool + padding).  
`EEPROM_SIZE` è definita in `src/WebServer.cpp` — deve essere **>= 68**. Attualmente = 128.

La libreria ESP8266 EEPROM fa bounds check in `put()`/`get()`: se `addr + sizeof(T) > size`, non legge/scrive nulla. Questo era il bug: EEPROM_SIZE era 64, lo struct 68 → nulla veniva salvato.

Validazione al boot: se uno dei float è NaN o `cycle_period_ms == 0 || > 300000`, viene ripristinato il default e salvato subito.

---

## PID

- `PID_Mngt()` in `AdvancedPID.cpp` — funzione monolitica, sample time fisso dt=0.1s
- Derivata e anti-windup **disabilitati** nel loop principale (`derivativeEnable=false`, `awEnable=false`)
- Output PWM: ciclo configurabile (`cycle_period_ms`), impulso minimo (`min_pulse_ms`)
- Modalità manuale: `pidParams.manual_mode` + `pidParams.manual_output`
- Setpoint temperatura mantenuto a 40°C ± 0.3°C

---

## Web interface

- `/` — dashboard principale (temp, timer, controlli start/stop)
- `/settings` — parametri PID avanzati
- `/diagnostics` — dati PID in tempo reale (polling JSON 1s)
- `/save_settings` POST — salva tutti i parametri in EEPROM
- `/temp_setpoint` POST — cambia solo setpoint
- `/temp_control` POST — start/stop/auto/manual
- `/reset_defaults` POST — ripristina default PID
- `/status` GET JSON — aggiornamento automatico dashboard (polling 2s)
- `/diag_data` GET JSON — dati diagnostica PID

---

## Encoder

- Lettura in **polling** da `encoder_update()` chiamata una volta per loop (~10ms). Il codice ISR (`encoder_isr`) esiste ma non è collegato (`attachInterrupt` non chiamato in `encoder_init`) — è codice morto.
- Polling semplificato: rileva solo il **falling edge di pin A**; il rising edge aggiorna `lastA` ma non genera eventi. Una rotazione rapida può perdere step se il pin A cambia due volte tra una chiamata e l'altra.
- Direzione invertita rispetto al segnale fisico: `dir = -encoder_get_direction()` in main.cpp.
- Long press (>800ms) → cambia pagina (Timer ↔ Heating). Short click → azione menu.
- Durante sleep: qualsiasi evento encoder sveglia il display (`eink_wake_up()`) e consuma l'evento senza elaborarlo (ignora il movimento/click di wake).

---

## Display — logica di refresh

Il refresh è **event-driven**: `needsUpdate` in main.cpp viene settato solo su variazione di stato (encoder, timer tick, cambio pagina). Le funzioni `drawMenu()` e `drawHeatingPage()` hanno una propria cache di stato interno (`lastCursor`, `lastHH`, ecc.) e ridisegnano solo le aree cambiate.

**Regola critica:** non aggiungere debounce temporale dentro le funzioni di draw. Il controllo "disegna solo se necessario" è già a due livelli (main.cpp + stato interno del draw). Un debounce nel draw causa desync: lo stato di main.cpp avanza con ogni evento encoder aggiornando i `last*` vars, ma il draw skippato non aggiorna i propri `last*` interni — e siccome main.cpp ha già allineato i suoi `last*` allo stato corrente, non triggera più un nuovo draw. Il display resta fermo e al prossimo evento mostra un salto multiplo.

Clear periodico anti-ghosting ogni 100 refresh (`CLEAR_EVERY_N_REFRESHES`) — non rimuovere, serve per evitare artefatti TFT.

Sleep display: gestito da `eink_check_sleep()`, timeout 2 minuti di inattività. Al wake: `eink_force_redraw()` setta i flag per il prossimo draw completo.

---

## Note operative

- **Serial disabilitato**: TX/RX usati per display. Non aggiungere `Serial.print` senza prima liberare i pin.
- **Debug**: solo via web (`/diagnostics`) o OTA.
- Sleep display: si risveglia con qualunque input encoder, ma quell'input viene scartato (serve solo per il wake).
