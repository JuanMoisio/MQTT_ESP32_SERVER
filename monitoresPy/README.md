# 🐍 Monitor Scripts - Herramientas Python de Auto-Detección

> **Suite completa de herramientas Python para automatizar el desarrollo, monitoreo y control del sistema ESP32 de huellas dactilares con detección automática de dispositivos USB.**

[![Python](https://img.shields.io/badge/Python-3.7+-green)](https://python.org/)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange)](https://platformio.org/)
[![PySerial](https://img.shields.io/badge/PySerial-Auto--Detection-blue)](https://pyserial.readthedocs.io/)

## 🚀 **Descripción General**

Este conjunto de scripts Python proporciona:
- 🔍 **Auto-detección inteligente** de dispositivos ESP32 por descriptores USB
- 📺 **Monitoreo dual automático** ESP32-C3 (SERVER) + ESP32-WROOM (CLIENT)
- ⬆️ **Sistema de auto-upload** con PlatformIO integrado
- 🎮 **Comandos remotos** via TCP al sistema de huellas
- 🔄 **Auto-reconexión** y limpieza de puertos ocupados
- 🚨 **Reset físico** de dispositivos via DTR/RTS

### **¿Por qué herramientas Python automatizadas?**
- ✅ **Plug & Play** - Conectar USB y ejecutar, sin configuración manual
- ✅ **Identificación inteligente** - Distingue automáticamente ESP32-C3 de ESP32-WROOM
- ✅ **Desarrollo ágil** - Upload y monitor automático durante codificación
- ✅ **Interfaz profesional** - Colores, timestamps y organización clara
- ✅ **Productividad máxima** - Una sola herramienta para todo el workflow

## 📁 **Estructura de Archivos**

```
monitoresPy/
├── 📄 README.md                    # Esta documentación
├── 🚀 launcher.py                  # Lanzador principal (TODO-EN-UNO)
├── 📺 single_terminal_monitor.py   # Monitor dual automático
├── ⬆️ auto_upload.py               # Sistema de auto-upload
├── ⚡ quick_upload.py              # Upload rápido por proyecto
├── 🔍 detect_devices.py           # Detección de dispositivos
├── 🧪 simple_scan_test.py         # Test básico de escaneo
├── 🔧 direct_server_test.py       # Test directo servidor TCP
└── 📜 requirements.txt            # Dependencias Python
```

## ⚡ **Inicio Rápido - TODO EN UNO**

### **🎯 Método Recomendado (Una Sola Línea)**
```bash
# Conecta ambos ESP32 por USB y ejecuta:
cd monitoresPy && python3 launcher.py

# ✨ El sistema hace TODO automáticamente:
# ✅ Detecta dispositivos ESP32
# ✅ Identifica cuál es SERVER (C3) y CLIENT (WROOM)  
# ✅ Upload automático del código más reciente
# ✅ Inicia monitor dual con colores
# ✅ Interfaz de comandos integrada
```

### **Salida Típica del Launcher**
```bash
🚀 LAUNCHER ESP32 SISTEMA INTEGRADO DE HUELLAS
═══════════════════════════════════════════════

🔍 Detectando dispositivos ESP32...
✅ ESP32-C3 SuperMini detectado en: /dev/cu.usbmodem14201  
✅ ESP32-WROOM detectado en: /dev/cu.usbserial-0001

📤 Uploading código actualizado...
[SERVER] Compilando BorkerMQTT...
[CLIENT] Compilando HuellaDactilar...  
✅ Upload completo en ambos dispositivos

📺 Iniciando monitor dual...
[11:30:00] Dispositivos conectados y listos
[11:30:01] [SERVER] 🌐 Broker TCP iniciado en 192.168.4.1:1883
[11:30:05] [CLIENT] 📡 Cliente conectado al broker

💬 Sistema listo. Comandos disponibles:
server:scan_fingerprint  - Escaneo remoto de huella
client:scan              - Escaneo local directo  
reset:both              - Reset ambos dispositivos
help                    - Lista completa de comandos

> _
```

## 🛠️ **Herramientas Individuales**

### **📺 Monitor Dual Automático**
```bash
python3 single_terminal_monitor.py

# Características:
✅ Auto-detecta ESP32-C3 como SERVER y ESP32-WROOM como CLIENT
✅ Monitor dual con colores diferenciados por dispositivo
✅ Timestamps automáticos en todos los mensajes
✅ Comandos integrados (server:, client:, reset:)
✅ Auto-reconexión en caso de desconexión USB
✅ Limpieza automática de puertos ocupados
```

#### **Colores por Dispositivo**
```python
# Sistema de colores organizacional:
🟦 AZUL    - [SERVER] ESP32-C3 (Broker TCP)
🟢 VERDE   - [CLIENT] ESP32-WROOM (Huella)  
🟡 AMARILLO - [ENVIADO] Comandos salientes
🔴 ROJO     - [ERROR] Mensajes de error
⚪ BLANCO   - [INFO] Información general
```

### **⬆️ Sistema de Auto-Upload**
```bash
python3 auto_upload.py

# Proceso automático:
1️⃣ Detecta dispositivos ESP32 conectados
2️⃣ Identifica tipo por descriptores USB
3️⃣ Mapea proyecto correspondiente:
   • ESP32-C3 → BorkerMQTT/
   • ESP32-WROOM → HuellaDactilar/
4️⃣ Ejecuta PlatformIO upload en paralelo
5️⃣ Verifica éxito de compilación y upload
```

#### **Upload Inteligente**
```bash
# El sistema determina automáticamente:
Port: /dev/cu.usbmodem14201 → Project: BorkerMQTT (ESP32-C3)
Port: /dev/cu.usbserial-0001 → Project: HuellaDactilar (WROOM)

# Sin configuración manual de puertos
# Sin modificar platformio.ini
# Sin conflictos de dispositivos múltiples
```

### **🔍 Detector de Dispositivos**
```bash
python3 detect_devices.py

# Salida ejemplo:
🔍 ESP32 DEVICE DETECTION
═══════════════════════════

📱 Dispositivos ESP32 encontrados:
┌─────────────────────────────────────────────┐
│ Puerto: /dev/cu.usbmodem14201              │
│ Descripción: USB Serial                    │  
│ Tipo detectado: ESP32-C3 SuperMini         │
│ Proyecto: BorkerMQTT                       │
└─────────────────────────────────────────────┘

┌─────────────────────────────────────────────┐
│ Puerto: /dev/cu.usbserial-0001             │
│ Descripción: USB-Serial Controller         │
│ Tipo detectado: ESP32-WROOM-32             │  
│ Proyecto: HuellaDactilar                   │
└─────────────────────────────────────────────┘

✅ Total: 2 dispositivos ESP32 detectados
```

### **⚡ Upload Rápido por Proyecto**
```bash
python3 quick_upload.py

# Menú interactivo:
🚀 QUICK UPLOAD - Selecciona proyecto:
[1] BorkerMQTT (ESP32-C3 Server)
[2] HuellaDactilar (ESP32-WROOM Client)  
[3] Ambos proyectos
[q] Salir

Selección: 1

📤 Uploading BorkerMQTT...
✅ Upload exitoso en 15.3 segundos
```

## 🎮 **Sistema de Comandos Integrado**

### **Comandos Principales**
| Comando | Descripción | Ejemplo de Uso |
|---------|-------------|----------------|
| `server:scan_fingerprint` | Escaneo remoto de huella | Control desde PC/monitor |
| `server:status_fingerprint` | Estado del sensor R305 | Diagnóstico remoto |
| `server:info_fingerprint` | Info del dispositivo | Especificaciones técnicas |
| `client:scan` | Escaneo local directo | Test del sensor local |
| `client:info` | Info del cliente | Estado del ESP32-WROOM |
| `reset:server` | Reset ESP32-C3 | Reiniciar solo broker |
| `reset:client` | Reset ESP32-WROOM | Reiniciar solo cliente |
| `reset:both` | Reset completo | Reiniciar todo el sistema |
| `help` | Lista de comandos | Ayuda interactiva |

### **Flujo de Comando Típico**
```bash
# 1. Ejecutar comando
> server:scan_fingerprint

# 2. Confirmación de envío  
[ENVIADO a SERVER] server:scan_fingerprint

# 3. Procesamiento en broker
[11:31:34] [SERVER] 🔍 Iniciando escaneo de huella...
[11:31:34] [SERVER] 📨 Comando enviado a fingerprint_4b224f7c630

# 4. Ejecución en cliente
[11:31:34] [CLIENT] 🔍 Comando MQTT: Escanear huella
[11:31:34] [CLIENT] [scanreq] requestScan timeoutMs=15000

# 5. Resultado del escaneo
[11:31:41] [CLIENT] Match OK: user=11 name='JUAN' score=154

# 6. Sistema listo para próximo comando
> _
```

## 🔧 **Configuración y Personalización**

### **Configuración de Puertos (Auto-Detectada)**
```python
# detect_devices.py - Lógica de detección
ESP32_DESCRIPTORS = {
    'ESP32-C3': [
        'USB Serial',           # USB CDC nativo ESP32-C3
        'usbmodem',            # Patrón macOS para CDC
        'USB JTAG'             # Algunas variantes C3
    ],
    'ESP32-WROOM': [
        'USB-Serial Controller', # CH340, CP2102, FTDI
        'usbserial',            # Patrón macOS para UART
        'Silicon Labs',         # CP210x específico
        'FTDI'                 # Chips FTDI
    ]
}
```

### **Configuración de Proyectos**
```python
# Mapeo automático proyecto↔dispositivo
PROJECT_MAPPING = {
    'ESP32-C3': {
        'project_dir': '../BorkerMQTT',
        'description': 'Broker TCP Server',
        'role': 'SERVER'
    },
    'ESP32-WROOM': {
        'project_dir': '../HuellaDactilar', 
        'description': 'Cliente Huella Dactilar',
        'role': 'CLIENT'  
    }
}
```

### **Personalización de Colores**
```python
# single_terminal_monitor.py - Colores customizables
class Colors:
    RED = '\033[91m'      # Errores
    GREEN = '\033[92m'    # Cliente/éxito
    YELLOW = '\033[93m'   # Advertencias/enviados
    BLUE = '\033[94m'     # Servidor
    CYAN = '\033[96m'     # Info general
    WHITE = '\033[97m'    # Normal
    BOLD = '\033[1m'      # Énfasis
    END = '\033[0m'       # Reset
```

## 🚀 **Instalación de Dependencias**

### **Instalación Automática**
```bash
# Método 1: pip desde requirements.txt
cd monitoresPy
pip3 install -r requirements.txt

# Método 2: Instalación manual
pip3 install pyserial platformio

# Verificación
python3 -c "import serial; print('✅ PySerial OK')"
pio --version  # ✅ PlatformIO OK
```

### **requirements.txt**
```txt
pyserial>=3.5
platformio>=6.1.0
```

### **Compatibilidad de Sistema**
- ✅ **macOS** (Testeado en macOS 14+)
- ✅ **Linux** (Ubuntu 20.04+, Raspberry Pi OS)  
- ✅ **Windows 10/11** (con drivers USB correctos)
- ✅ **Python 3.7+** (Recomendado 3.9+)

## 🔍 **Auto-Detección Avanzada**

### **Algoritmo de Detección USB**
```python
def detect_esp32_devices():
    """
    Detecta dispositivos ESP32 por:
    1. Descriptores USB únicos
    2. Patrones de nombre de puerto
    3. VID/PID cuando disponible
    4. Comportamiento del dispositivo
    """
    
    # Escaneo inicial
    all_ports = serial.tools.list_ports.comports()
    
    # Filtrado por descriptores ESP32
    esp32_ports = []
    for port in all_ports:
        if any(desc in port.description for desc in ESP32_DESCRIPTORS):
            esp32_ports.append(port)
    
    # Clasificación por tipo
    for port in esp32_ports:
        device_type = classify_esp32_type(port)
        devices[device_type] = port.device
    
    return devices
```

### **Identificación Inteligente**
```python
# Ejemplos de detección real:
Port: /dev/cu.usbmodem14201
Description: "USB Serial"
→ Clasificado: ESP32-C3 (USB CDC nativo)

Port: /dev/cu.usbserial-0001  
Description: "USB-Serial Controller"
→ Clasificado: ESP32-WROOM (UART bridge)

Port: /dev/cu.SLAB_USBtoUART
Description: "Silicon Labs CP210x"  
→ Clasificado: ESP32-WROOM (CP2102 chip)
```

### **Manejo de Casos Especiales**
```python
# Múltiples dispositivos del mismo tipo
def handle_multiple_devices():
    if len(esp32_c3_devices) > 1:
        print("⚠️ Múltiples ESP32-C3 detectados")
        print("🎯 Usando el primero encontrado")
        return esp32_c3_devices[0]
    
    if len(esp32_wroom_devices) > 1:
        print("⚠️ Múltiples ESP32-WROOM detectados") 
        print("📋 Selecciona manualmente:")
        # Mostrar menú de selección...
```

## 🔄 **Auto-Reconexión y Recovery**

### **Detección de Desconexión**
```python
def monitor_connection_health():
    """
    Monitorea salud de conexiones USB:
    - Detecta desconexión física
    - Detecta puertos ocupados por otros procesos  
    - Auto-reconexión cuando dispositivo vuelve
    - Limpieza de procesos zombi
    """
    
    while True:
        try:
            # Test de conectividad
            if not connection.is_open:
                print(f"🔌 Dispositivo {device} desconectado")
                attempt_reconnection(device)
                
        except serial.SerialException:
            handle_port_occupied(device)
```

### **Limpieza de Puertos Ocupados**  
```python
def cleanup_occupied_ports():
    """
    Limpia puertos ocupados por:
    - Procesos PlatformIO colgados
    - Monitores serie previos
    - Conexiones TCP abiertas
    - Procesos Python zombi
    """
    
    # Método 1: Señal SIGTERM suave
    try:
        for proc in get_processes_using_port(port):
            proc.terminate()
            proc.wait(timeout=3)
    except:
        # Método 2: Kill forzado
        subprocess.run(['killall', '-9', 'python3'], 
                      capture_output=True)
```

## 🧪 **Scripts de Testing y Debug**

### **🧪 simple_scan_test.py**
```bash
python3 simple_scan_test.py

# Test básico de conexión TCP:
🧪 TEST SIMPLE DE ESCANEO
═══════════════════════════

🔍 Buscando ESP32-C3 Server...
✅ Servidor encontrado en: /dev/cu.usbmodem14201

📡 Conectando a TCP 192.168.4.1:1883...
✅ Conexión TCP establecida

📤 Enviando comando: server:scan_fingerprint  
⏳ Esperando respuesta...
✅ Comando procesado correctamente

🎯 Test completado exitosamente
```

### **🔧 direct_server_test.py**
```bash
python3 direct_server_test.py

# Test directo del protocolo TCP:
🔧 TEST DIRECTO SERVIDOR TCP
══════════════════════════════

📡 Conectando directamente a 192.168.4.1:1883...
✅ Socket TCP abierto

📤 Enviando JSON: {"type":"command","command":"scan_fingerprint"}
📥 Respuesta recibida: {"type":"ack","status":"processing"}

🔍 Análisis del protocolo:
✓ JSON válido
✓ Respuesta correcta
✓ Latencia: 45ms

🎯 Protocolo TCP funcionando correctamente
```

## 📊 **Monitoreo de Performance**

### **Métricas del Sistema**
```python
# Datos recopilados automáticamente:
performance_metrics = {
    'upload_time': 15.3,           # segundos
    'detection_time': 0.8,         # segundos  
    'tcp_latency': 45,             # millisegundos
    'reconnection_time': 3.2,      # segundos
    'success_rate': 99.1,          # porcentaje
    'devices_managed': 2,          # cantidad
    'uptime': 3600,               # segundos
    'commands_processed': 47       # cantidad
}
```

### **Log de Performance**
```bash
# Salida en monitor:
[STATS] Upload time: 15.3s
[STATS] Detection time: 0.8s  
[STATS] TCP latency: 45ms
[STATS] Success rate: 99.1%
[STATS] Uptime: 1h 0m 0s
```

## 🐛 **Troubleshooting Avanzado**

### **Problemas Comunes**

#### **🔴 "No se detectan dispositivos ESP32"**
```bash
# Diagnóstico paso a paso:
python3 detect_devices.py

# Si no detecta nada:
1. Verificar cables USB (data, no solo carga)
2. Revisar drivers USB en Windows
3. Probar diferentes puertos USB
4. Verificar permisos en Linux/macOS:
   sudo chmod 666 /dev/ttyUSB* /dev/ttyACM*
```

#### **🔴 "Puerto ocupado por otro proceso"**
```bash
# Solución automática:
python3 launcher.py
# El script limpia automáticamente

# Solución manual:
sudo lsof -t /dev/cu.usbmodem14201 | xargs kill -9
killall -9 python3
```

#### **🔴 "Upload falla constantemente"**
```bash
# Debug paso a paso:
1. Verificar PlatformIO: pio --version
2. Test manual: cd BorkerMQTT && pio run --target upload  
3. Verificar memoria disponible: df -h
4. Revisar permisos: ls -la /dev/cu.*
```

#### **🔴 "Monitor se desconecta aleatoriamente"**
```bash
# Causas comunes:
- Cable USB defectuoso → Cambiar cable
- Puerto USB con poca potencia → Usar hub con alimentación
- Interferencia electromagnética → Alejar de WiFi/Bluetooth
- Proceso competidor → Usar cleanup automático
```

### **Debug Avanzado**
```python
# Activar debug verbose en cualquier script:
DEBUG_VERBOSE = True

# Salida detallada:
[DEBUG] Scanning USB ports...
[DEBUG] Found port: /dev/cu.usbmodem14201
[DEBUG] Port description: USB Serial  
[DEBUG] Classified as: ESP32-C3
[DEBUG] Mapping to project: BorkerMQTT
[DEBUG] Starting upload process...
[DEBUG] PlatformIO command: pio run --target upload
[DEBUG] Upload successful in 15.3 seconds
```

## 🚀 **Workflow de Desarrollo Recomendado**

### **Desarrollo Diario**
```bash
# 1. Conectar hardware
# Conectar ESP32-C3 y ESP32-WROOM por USB

# 2. Lanzar entorno completo  
cd monitoresPy && python3 launcher.py

# 3. Desarrollar código
# Editar archivos en BorkerMQTT/ y HuellaDactilar/

# 4. Test rápido
server:scan_fingerprint

# 5. Upload cambios (si necesario)
python3 auto_upload.py
```

### **Debugging Específico**
```bash  
# Para debug del broker TCP:
python3 direct_server_test.py

# Para debug de detección USB:
python3 detect_devices.py

# Para debug del protocolo completo:
python3 single_terminal_monitor.py
# Activar debug mode en ambos ESP32
```

### **Testing de Integración**
```bash
# Test completo del sistema:
python3 simple_scan_test.py

# Verificar todos los comandos:
server:scan_fingerprint
server:status_fingerprint  
server:info_fingerprint
client:scan
reset:both
```

## 🎯 **Próximas Funcionalidades**

### **En Desarrollo**
- 📊 **Dashboard web** para monitoreo remoto
- 🤖 **CI/CD automático** con GitHub Actions  
- 📱 **App móvil** para control remoto
- 🔐 **Logs de seguridad** con persistencia
- 🌐 **Soporte multi-broker** para múltiples ESP32-C3

### **Roadmap**
```
v1.0 ✅ Sistema básico funcional
v1.1 ✅ Auto-detección y upload automático  
v1.2 ✅ Monitor dual con comandos
v1.3 🚧 Dashboard web en desarrollo
v1.4 📋 App móvil planificada
v2.0 💡 Sistema multi-site con múltiples brokers
```

---

📖 **Documentación relacionada:**
- [README Principal](../README.md) - Visión general del sistema
- [Broker ESP32-C3](../BorkerMQTT/README.md) - Documentación del servidor
- [Cliente ESP32-WROOM](../HuellaDactilar/README.md) - Documentación del cliente  
- [Documentación Técnica](../TECHNICAL.md) - Detalles avanzados del protocolo

---

⭐ **¡Estas herramientas Python hacen que el desarrollo ESP32 sea plug-and-play!** ⭐