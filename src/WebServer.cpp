#include "WebServer.h"
#include <EEPROM.h>
#include "TemperatureController.h"
#include "NTCReader.h"
#include "Timer.h"
#include "Menu.h"

#include "TemperatureController.h"

ESP8266WebServer server(80);
PIDParams pidParams = TemperatureController::getDefaultPIDParams();

#define EEPROM_SIZE 128
#define EEPROM_PID_ADDR 0

extern TemperatureController tempController;
extern NTCReader ntcReader;

void loadPIDParamsFromEEPROM() {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(EEPROM_PID_ADDR, pidParams);
    // Verifica validità di tutti i campi critici
    if (isnan(pidParams.Kp) || isnan(pidParams.Gp) || isnan(pidParams.Ti) || isnan(pidParams.Td) ||
        isnan(pidParams.Taw) || isnan(pidParams.setpoint) ||
        pidParams.cycle_period_ms == 0 || pidParams.cycle_period_ms > 300000) {
        // Ripristina valori di default
        pidParams = TemperatureController::getDefaultPIDParams();
        // Salva immediatamente i default in EEPROM
        savePIDParamsToEEPROM();
    }
}

void savePIDParamsToEEPROM() {
    EEPROM.put(EEPROM_PID_ADDR, pidParams);
    EEPROM.commit();
}

void setupWebServer() {
    loadPIDParamsFromEEPROM();
    tempController.setAdvancedPIDParams(pidParams);
    
    // Pagina principale - Controlli
    server.on("/", HTTP_GET, []() {
        float temp = ntcReader.readTemperature() + pidParams.compensazione;
        float pidOut = tempController.getOutput();
        int hh, mm, ss;
        menu_get_timer(hh, mm, ss);
        bool timerRunning = timer_is_running();
        
        String html = "<html><head><meta charset='UTF-8'><title>Sleepy</title>";
        html += "<style>";
        html += "body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
        html += ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}";
        html += "h2{color:#333;border-bottom:2px solid #007bff;padding-bottom:10px}";
        html += ".section{margin:20px 0;padding:15px;background:#f9f9f9;border-radius:5px}";
        html += ".control-row{display:flex;justify-content:space-between;align-items:center;margin:10px 0}";
        html += ".label{font-weight:bold;color:#555}";
        html += ".value{font-size:1.2em;color:#007bff}";
        html += ".status{display:inline-block;padding:5px 10px;border-radius:3px;font-weight:bold}";
        html += ".status.active{background:#28a745;color:white}";
        html += ".status.inactive{background:#dc3545;color:white}";
        html += ".status.manual{background:#ffc107;color:black}";
        html += ".status.auto{background:#17a2b8;color:white}";
        html += "button{padding:10px 20px;margin:5px;border:none;border-radius:5px;cursor:pointer;font-size:14px}";
        html += ".btn-start{background:#28a745;color:white}";
        html += ".btn-stop{background:#dc3545;color:white}";
        html += ".btn-auto{background:#17a2b8;color:white}";
        html += ".btn-manual{background:#ffc107;color:black}";
        html += ".btn-settings{background:#6c757d;color:white}";
        html += "input{padding:8px;border:1px solid #ddd;border-radius:3px;width:100px}";
        html += "</style>";
        html += "<script>";
        html += "function updateStatus(){";
        html += "fetch('/status').then(r=>r.json()).then(d=>{";
        html += "document.getElementById('temp').innerText=d.temp.toFixed(2);";
        html += "document.getElementById('output').innerText=d.output.toFixed(2);";
        html += "document.getElementById('timer').innerText=d.timer;";
        html += "document.getElementById('tempState').className=d.tempEnabled?'status active':'status inactive';";
        html += "document.getElementById('tempState').innerText=d.tempEnabled?'ATTIVO':'FERMO';";
        html += "document.getElementById('timerState').className=d.timerRunning?'status active':'status inactive';";
        html += "document.getElementById('timerState').innerText=d.timerRunning?'IN CORSO':'FERMO';";
        html += "}).catch(e=>console.log(e));}";
        html += "setInterval(updateStatus,2000);";
        html += "</script>";
        html += "</head><body><div class='container'>";
        html += "<h2>🔥 Sleepy</h2>";
        
        // Sezione Temperatura
        html += "<div class='section'>";
        html += "<h3>🌡️ Controllo Temperatura</h3>";
        html += "<div class='control-row'><span class='label'>Stato:</span><span id='tempState' class='status ";
        html += (pidParams.enabled ? "active'>ATTIVO" : "inactive'>FERMO");
        html += "</span></div>";
        html += "<div class='control-row'><span class='label'>Temperatura:</span><span id='temp' class='value'>" + String(temp, 2) + "</span> °C</div>";
        html += "<div class='control-row'><span class='label'>Output PID:</span><span id='output' class='value'>" + String(pidOut, 2) + "</span> %</div>";
        html += "<form method='POST' action='/temp_setpoint' style='margin:15px 0'>";
        html += "<div class='control-row'><span class='label'>Setpoint:</span>";
        html += "<input type='number' name='setpoint' value='" + String(pidParams.setpoint, 1) + "' step='0.5' min='10' max='50'> °C ";
        html += "<button type='submit' class='btn-settings'>Imposta</button></div></form>";
        html += "<form method='POST' action='/temp_control'>";
        html += "<button name='action' value='start' class='btn-start'>▶ START</button>";
        html += "<button name='action' value='stop' class='btn-stop'>⏹ STOP</button>";
        html += "</form></div>";
        
        // Sezione Timer
        html += "<div class='section'>";
        html += "<h3>⏱️ Timer</h3>";
        html += "<div class='control-row'><span class='label'>Stato:</span><span id='timerState' class='status ";
        html += (timerRunning ? "active'>IN CORSO" : "inactive'>FERMO");
        html += "</span></div>";
        html += "<div class='control-row'><span class='label'>Tempo:</span><span id='timer' class='value'>";
        html += String(hh) + ":" + (mm < 10 ? "0" : "") + String(mm) + ":" + (ss < 10 ? "0" : "") + String(ss);
        html += "</span></div>";
        html += "<form method='POST' action='/timer_set' style='margin:15px 0'>";
        html += "<div class='control-row'><span class='label'>Imposta Timer:</span>";
        html += "<input type='number' name='hours' value='" + String(hh) + "' min='0' max='23' style='width:50px'> h ";
        html += "<input type='number' name='minutes' value='" + String(mm) + "' min='0' max='59' style='width:50px'> m ";
        html += "<input type='number' name='seconds' value='" + String(ss) + "' min='0' max='59' style='width:50px'> s ";
        html += "<button type='submit' class='btn-settings'>Imposta</button></div></form>";
        html += "<form method='POST' action='/timer_control'>";
        html += "<button name='action' value='start' class='btn-start'>▶ START</button>";
        html += "<button name='action' value='pause' class='btn-stop'>⏸ PAUSE</button>";
        html += "<button name='action' value='reset' class='btn-settings'>↺ RESET</button>";
        html += "</form></div>";
        
        // Link Settings e Diagnostica
        html += "<div style='text-align:center;margin-top:20px'>";
        html += "<a href='/settings'><button class='btn-settings'>⚙️ Impostazioni Avanzate</button></a> ";
        html += "<a href='/diagnostics'><button class='btn-settings'>📊 Diagnostica PID</button></a>";
        html += "</div>";
        
        html += "</div></body></html>";
        server.send(200, "text/html", html);
    });
    
    // Pagina Settings
    server.on("/settings", HTTP_GET, []() {
        String html = "<html><head><meta charset='UTF-8'><title>Impostazioni PID</title>";
        html += "<style>";
        html += "body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
        html += ".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}";
        html += "h2{color:#333;border-bottom:2px solid #007bff;padding-bottom:10px}";
        html += ".form-group{margin:15px 0}";
        html += "label{display:inline-block;width:200px;font-weight:bold;color:#555}";
        html += "input{padding:8px;border:1px solid #ddd;border-radius:3px;width:150px}";
        html += "button{padding:10px 20px;margin:10px 5px;border:none;border-radius:5px;cursor:pointer;font-size:14px}";
        html += ".btn-save{background:#28a745;color:white}";
        html += ".btn-back{background:#6c757d;color:white}";
        html += "</style>";
        html += "</head><body><div class='container'>";
        html += "<h2>⚙️ Impostazioni Avanzate PID</h2>";
        html += "<div style='margin:20px 0;padding:15px;background:#f9f9f9;border-radius:5px'>";
        html += "<h3>Modalità Controllo</h3>";
        html += "<p>Modalità attuale: <b>" + String(pidParams.manual_mode ? "MANUALE" : "AUTOMATICA") + "</b></p>";
        html += "<form method='POST' action='/temp_control'>";
        html += "<button name='action' value='auto' class='btn-auto'>AUTO (PID)</button>";
        html += "<button name='action' value='manual' class='btn-manual'>MANUALE</button>";
        html += "</form></div>";
        html += "<form method='POST' action='/save_settings'>";
        html += "<div class='form-group'><label>Kp (Guadagno principale):</label><input name='Kp' value='" + String(pidParams.Kp, 3) + "' step='0.1'></div>";
        html += "<div class='form-group'><label>Gp (Guadagno proporzionale):</label><input name='Gp' value='" + String(pidParams.Gp, 3) + "' step='0.1'></div>";
        html += "<div class='form-group'><label>Ti (Integrale, s):</label><input name='Ti' value='" + String(pidParams.Ti, 2) + "' step='1'></div>";
        html += "<div class='form-group'><label>Td (Derivativo, s):</label><input name='Td' value='" + String(pidParams.Td, 3) + "' step='0.1'></div>";
        html += "<div class='form-group'><label>Taw (Anti-windup, s):</label><input name='Taw' value='" + String(pidParams.Taw, 3) + "' step='0.01'></div>";
        html += "<div class='form-group'><label>Out Min (%):</label><input name='out_min' value='" + String(pidParams.out_min, 1) + "'></div>";
        html += "<div class='form-group'><label>Out Max (%):</label><input name='out_max' value='" + String(pidParams.out_max, 1) + "'></div>";
        html += "<div class='form-group'><label>Ref Min (%):</label><input name='ref_min' value='" + String(pidParams.ref_min, 1) + "'></div>";
        html += "<div class='form-group'><label>Ref Max (%):</label><input name='ref_max' value='" + String(pidParams.ref_max, 1) + "'></div>";
        html += "<div class='form-group'><label>Gradiente (°C/s):</label><input name='gradiente' value='" + String(pidParams.gradiente, 2) + "' step='0.1'></div>";
        html += "<div class='form-group'><label>Output Gradient (%/s):</label><input name='output_gradient' value='" + String(pidParams.output_gradient, 2) + "' step='0.1'></div>";
        html += "<div class='form-group'><label>Manual Output (%):</label><input name='manual_output' value='" + String(pidParams.manual_output, 1) + "'></div>";
        html += "<div class='form-group'><label>Compensazione Temp (°C):</label><input name='compensazione' value='" + String(pidParams.compensazione, 2) + "' step='0.1'></div>";
        html += "<div class='form-group'><label>Cycle Period (ms):</label><input name='cycle_period_ms' value='" + String(pidParams.cycle_period_ms) + "'></div>";
        html += "<div class='form-group'><label>Min Pulse (ms):</label><input name='min_pulse_ms' value='" + String(pidParams.min_pulse_ms) + "'></div>";
        html += "<button type='submit' class='btn-save'>💾 Salva</button>";
        html += "<a href='/'><button type='button' class='btn-back'>← Indietro</button></a>";
        html += "</form>";
        html += "<form method='POST' action='/reset_defaults' style='margin-top:20px;padding:15px;background:#fff3cd;border-radius:5px'>";
        html += "<p style='margin:10px 0'><b>⚠️ Reset ai valori di default:</b> Kp=1.0, Gp=40.0, Ti=0.0 (solo P)</p>";
        html += "<button type='submit' style='background:#dc3545;color:white'>🔄 Reset Defaults</button>";
        html += "</form>";
        html += "</div></body></html>";
        server.send(200, "text/html", html);
    });
    
    // Handler per salvare impostazioni
    server.on("/save_settings", HTTP_POST, []() {
        pidParams.Kp = server.arg("Kp").toFloat();
        pidParams.Gp = server.arg("Gp").toFloat();
        pidParams.Ti = server.arg("Ti").toFloat();
        pidParams.Td = server.arg("Td").toFloat();
        pidParams.Taw = server.arg("Taw").toFloat();
        pidParams.out_min = server.arg("out_min").toFloat();
        pidParams.out_max = server.arg("out_max").toFloat();
        pidParams.ref_min = server.arg("ref_min").toFloat();
        pidParams.ref_max = server.arg("ref_max").toFloat();
        pidParams.gradiente = server.arg("gradiente").toFloat();
        pidParams.output_gradient = server.arg("output_gradient").toFloat();
        pidParams.manual_output = server.arg("manual_output").toFloat();
        pidParams.compensazione = server.arg("compensazione").toFloat();
        pidParams.cycle_period_ms = server.arg("cycle_period_ms").toInt();
        pidParams.min_pulse_ms = server.arg("min_pulse_ms").toInt();
        savePIDParamsToEEPROM();
        tempController.setAdvancedPIDParams(pidParams);
        server.sendHeader("Location", "/settings", true);
        server.send(302, "text/plain", "Updated");
    });
    
    // Handler per setpoint temperatura
    server.on("/temp_setpoint", HTTP_POST, []() {
        if (server.hasArg("setpoint")) {
            pidParams.setpoint = server.arg("setpoint").toFloat();
            savePIDParamsToEEPROM();
        }
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "Updated");
    });
    
    // Handler per controlli temperatura
    server.on("/temp_control", HTTP_POST, []() {
        if (server.hasArg("action")) {
            String action = server.arg("action");
            if (action == "start") {
                pidParams.enabled = true;
                tempController.enable();
            } else if (action == "stop") {
                pidParams.enabled = false;
                tempController.disable();
            } else if (action == "auto") {
                pidParams.manual_mode = false;
            } else if (action == "manual") {
                pidParams.manual_mode = true;
            }
            savePIDParamsToEEPROM();
        }
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "Updated");
    });
    
    // Handler per reset parametri PID a default
    server.on("/reset_defaults", HTTP_POST, []() {
        pidParams = TemperatureController::getDefaultPIDParams();
        savePIDParamsToEEPROM();
        tempController.setAdvancedPIDParams(pidParams);
        server.sendHeader("Location", "/settings", true);
        server.send(302, "text/plain", "Reset to defaults");
    });
    
    // Handler per controlli timer
    server.on("/timer_control", HTTP_POST, []() {
        if (server.hasArg("action")) {
            String action = server.arg("action");
            if (action == "start") {
                timer_start();
            } else if (action == "pause") {
                timer_pause();
            } else if (action == "reset") {
                timer_reset();
            }
        }
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "Updated");
    });
    
    // Handler per impostare il timer
    server.on("/timer_set", HTTP_POST, []() {
        if (server.hasArg("hours") && server.hasArg("minutes") && server.hasArg("seconds")) {
            int h = server.arg("hours").toInt();
            int m = server.arg("minutes").toInt();
            int s = server.arg("seconds").toInt();
            timer_set(h, m, s);
        }
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "Updated");
    });
    
    // Pagina Diagnostica PID
    server.on("/diagnostics", HTTP_GET, []() {
        String html = "<html><head><meta charset='UTF-8'><title>Diagnostica PID</title>";
        html += "<style>";
        html += "body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0}";
        html += ".container{max-width:1000px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}";
        html += "h2{color:#333;border-bottom:2px solid #007bff;padding-bottom:10px}";
        html += "table{width:100%;border-collapse:collapse;margin:20px 0}";
        html += "th{background:#007bff;color:white;padding:10px;text-align:left;font-weight:bold}";
        html += "td{padding:8px;border-bottom:1px solid #ddd}";
        html += "tr:hover{background:#f5f5f5}";
        html += ".value{font-family:monospace;font-size:1.1em;color:#007bff}";
        html += "button{padding:10px 20px;margin:10px 5px;border:none;border-radius:5px;cursor:pointer;font-size:14px;background:#6c757d;color:white}";
        html += "</style>";
        html += "<script>";
        html += "function updateDiag(){";
        html += "fetch('/diag_data').then(r=>r.json()).then(d=>{";
        html += "document.getElementById('set').innerText=d.set.toFixed(2);";
        html += "document.getElementById('act').innerText=d.act.toFixed(2);";
        html += "document.getElementById('err').innerText=d.err.toFixed(3);";
        html += "document.getElementById('p').innerText=d.p.toFixed(3);";
        html += "document.getElementById('i').innerText=d.i.toFixed(3);";
        html += "document.getElementById('d').innerText=d.d.toFixed(3);";
        html += "document.getElementById('tot').innerText=d.tot.toFixed(3);";
        html += "document.getElementById('out').innerText=d.out.toFixed(2);";
        html += "document.getElementById('avg').innerText=d.avg.toFixed(2);";
        html += "document.getElementById('samples').innerText=d.samples;";
        html += "}).catch(e=>console.log(e));}";
        html += "setInterval(updateDiag,1000);";
        html += "</script>";
        html += "</head><body><div class='container'>";
        html += "<h2>📊 Diagnostica PID in Tempo Reale</h2>";
        html += "<table>";
        html += "<tr><th>Parametro</th><th>Valore</th><th>Unità</th></tr>";
        html += "<tr><td>Setpoint</td><td class='value' id='set'>-</td><td>°C</td></tr>";
        html += "<tr><td>Actual (Temperatura)</td><td class='value' id='act'>-</td><td>°C</td></tr>";
        html += "<tr><td>Errore</td><td class='value' id='err'>-</td><td>°C</td></tr>";
        html += "<tr><td>Componente Proporzionale</td><td class='value' id='p'>-</td><td>%</td></tr>";
        html += "<tr><td>Componente Integrativa</td><td class='value' id='i'>-</td><td>%</td></tr>";
        html += "<tr><td>Componente Derivativa</td><td class='value' id='d'>-</td><td>%</td></tr>";
        html += "<tr><td>Correzione Totale</td><td class='value' id='tot'>-</td><td>%</td></tr>";
        html += "<tr><td>Uscita Effettiva</td><td class='value' id='out'>-</td><td>%</td></tr>";
        html += "<tr><td>PWM Duty Average</td><td class='value' id='avg'>-</td><td>%</td></tr>";
        html += "<tr><td>PWM Duty Samples</td><td class='value' id='samples'>-</td><td>-</td></tr>";
        html += "</table>";
        html += "<div style='text-align:center'>";
        html += "<a href='/'><button>← Indietro</button></a>";
        html += "</div>";
        html += "</div></body></html>";
        server.send(200, "text/html", html);
    });
    
    // API per dati diagnostica PID
    server.on("/diag_data", HTTP_GET, []() {
        float temp = ntcReader.readTemperature() + pidParams.compensazione;
        PID_Handle* pidHandle = tempController.getPIDHandle();
        
        String json = "{";
        json += "\"set\":" + String(pidParams.setpoint, 2) + ",";
        json += "\"act\":" + String(temp, 2) + ",";
        json += "\"err\":" + String(pidHandle->state.error, 3) + ",";
        json += "\"p\":" + String(pidHandle->state.proportionalCorrection, 3) + ",";
        json += "\"i\":" + String(pidHandle->state.integralCorrection, 3) + ",";
        json += "\"d\":" + String(pidHandle->state.derivativeCorrection, 3) + ",";
        json += "\"tot\":" + String(pidHandle->state.totalCorrection, 3) + ",";
        json += "\"out\":" + String(pidHandle->state.out, 2) + ",";
        json += "\"avg\":" + String(tempController.getOutput(), 2) + ",";  // Usa output corrente
        json += "\"samples\":0";  // Non disponibile nel TemperatureController
        json += "}";
        server.send(200, "application/json", json);
    });
    
    // API Status per aggiornamento automatico
    server.on("/status", HTTP_GET, []() {
        float temp = ntcReader.readTemperature() + pidParams.compensazione;
        float pidOut = tempController.getOutput();
        int hh, mm, ss;
        menu_get_timer(hh, mm, ss);
        bool timerRunning = timer_is_running();
        
        String json = "{";
        json += "\"temp\":" + String(temp, 2) + ",";
        json += "\"output\":" + String(pidOut, 2) + ",";
        json += "\"tempEnabled\":" + String(pidParams.enabled ? "true" : "false") + ",";
        json += "\"manual\":" + String(pidParams.manual_mode ? "true" : "false") + ",";
        json += "\"timerRunning\":" + String(timerRunning ? "true" : "false") + ",";
        json += "\"timer\":\"" + String(hh) + ":" + (mm < 10 ? "0" : "") + String(mm) + ":" + (ss < 10 ? "0" : "") + String(ss) + "\"";
        json += "}";
        server.send(200, "application/json", json);
    });
    
    server.begin();
}

void handleWebServer() {
    server.handleClient();
}
