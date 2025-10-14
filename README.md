# 🔐 Sistema Integrado de Huella Dactilar ESP32

> **Sistema completo de reconocimiento biométrico con ESP32-C3 como broker TCP y ESP32-WROOM como cliente de huellas, incluyendo herramientas de monitoreo automático con Python.**

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange)](https://platformio.org/)
[![ESP32](https://img.shields.io/badge/ESP32-C3%20%7C%20WROOM-blue)](https://www.espressif.com/)
[![Python](https://img.shields.io/badge/Python-3.7+-green)](https://python.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## 🏗️ **Arquitectura del Sistema**

```
┌─────────────────────┐    TCP/IP     ┌─────────────────────┐
│   ESP32-C3 SuperMini│◄─────────────►│  ESP32-WROOM-32    │
│   (Broker Server)   │   WiFi AP     │  (Client Huella)   │
│                     │               │                     │
│  • Red: DEPOSITO_   │               │  • Sensor R305     │
│  • IP: 192.168.4.1  │               │  • Display OLED    │
│  • Puerto: 1883     │               │  • Auto-conexión   │
└─────────────────────┘               └─────────────────────┘
           ▲                                     ▲
           │                                     │
           ▼                                     ▼
┌─────────────────────────────────────────────────────────────┐
│              🐍 Python Monitor Scripts                      │
│                                                             │
│  • Auto-detección USB                                      │
│  • Monitoreo dual en tiempo real                          │
│  • Upload automático con PlatformIO                       │
│  • Comandos remotos vía TCP                               │
└─────────────────────────────────────────────────────────────┘
```

## ⚡ **Inicio Rápido**

### 1️⃣ **Preparación del Hardware**
```bash
# Conecta los dispositivos:
ESP32-C3 SuperMini ──USB──► Puerto A del PC
ESP32-WROOM + R305  ──USB──► Puerto B del PC
```

### 2️⃣ **Instalación y Deploy Automático**
```bash
# Clona el repositorio
git clone [tu-repo-url]
cd PlatformIO/Projects

# Ejecuta el sistema completo
cd monitoresPy
python3 launcher.py
```

### 3️⃣ **Uso del Sistema**
```bash
# Una vez iniciado el monitor, usa estos comandos:
server:scan_fingerprint    # 🔍 Escaneo remoto de huella
server:status_fingerprint  # 📊 Estado del sensor
server:info_fingerprint    # ℹ️ Info del dispositivo
client:scan                # 👆 Escaneo directo
reset:both                 # 🔄 Resetear ambos ESP32
```

## 🚀 **Características Principales**

### 🤖 **Auto-Detección Inteligente**
- ✅ Detección automática de puertos ESP32 por descriptores USB
- ✅ Identificación automática entre ESP32-C3 y ESP32-WROOM  
- ✅ Reconexión automática en caso de desconexión
- ✅ Limpieza automática de puertos ocupados

### 🌐 **Protocolo TCP Personalizado**
- ✅ Servidor TCP custom en ESP32-C3 (no MQTT estándar)
- ✅ Comunicación JSON sobre TCP para máximo rendimiento
- ✅ Red WiFi AP automática `DEPOSITO_BROKER`
- ✅ Heartbeat y registro automático de módulos

### 🔐 **Sistema de Huellas Avanzado**
- ✅ Sensor R305 con 1000+ templates de huella
- ✅ Display OLED SH1106 con interfaz gráfica
- ✅ Procesamiento automático de coincidencias
- ✅ Comandos remotos via broker TCP

### 🛠️ **Herramientas de Desarrollo**
- ✅ Upload automático con detección de dispositivo
- ✅ Monitor dual con salida organizada por colores
- ✅ Scripts de prueba y debug independientes
- ✅ Reset físico remoto de dispositivos

## 📁 **Estructura del Proyecto**

```
PlatformIO/Projects/
├── 📄 README.md                    # Esta documentación
├── 🖥️ BorkerMQTT/                   # ESP32-C3 Broker Server
│   ├── src/main.cpp               # Servidor TCP personalizado
│   ├── platformio.ini             # Config PlatformIO (sin puerto fijo)
│   └── 📄 README.md               # Docs específicas del broker
├── 📱 HuellaDactilar/              # ESP32-WROOM Client
│   ├── src/
│   │   ├── main.cpp               # Cliente principal
│   │   └── MqttBrokerClient.cpp   # Protocolo TCP custom
│   ├── platformio.ini             # Config PlatformIO (sin puerto fijo)
│   └── 📄 README.md               # Docs específicas del cliente
├── 🐍 monitoresPy/                 # Scripts Python de monitoreo
│   ├── launcher.py                # 🚀 Lanzador principal
│   ├── single_terminal_monitor.py # 📺 Monitor dual automático
│   ├── auto_upload.py             # ⬆️ Upload automático  
│   ├── quick_upload.py            # ⚡ Upload rápido
│   ├── detect_devices.py          # 🔍 Detección de dispositivos
│   └── 📄 README.md               # Docs de herramientas Python
└── 📄 TECHNICAL.md                # Documentación técnica avanzada
```

## 🔧 **Instalación de Dependencias**

### **Python Requirements**
```bash
# Instalar dependencias Python
pip3 install pyserial platformio

# Verificar instalación
python3 -c "import serial; print('PySerial OK')"
pio --version
```

### **Hardware Requirements**
- **ESP32-C3 SuperMini** (Broker Server)
- **ESP32-WROOM-32** (Cliente Huella)  
- **Sensor R305** (Huella dactilar)
- **Display OLED SH1106** 128x64
- **Cables USB** para ambos ESP32

### **PlatformIO Libraries** (Auto-instaladas)
```ini
# Para ESP32-C3 (BorkerMQTT)
ArduinoJson @ ^6.21.4
WiFi (built-in)

# Para ESP32-WROOM (HuellaDactilar)  
ArduinoJson @ ^6.21.4
Adafruit Fingerprint Sensor Library @ ^2.1.0
U8g2 @ ^2.34.22
```

## 📊 **Comandos Disponibles**

| Comando | Dispositivo | Descripción |
|---------|-------------|-------------|
| `server:scan_fingerprint` | 🖥️ ESP32-C3 → 📱 ESP32-WROOM | Escaneo remoto de huella |
| `server:status_fingerprint` | 🖥️ ESP32-C3 → 📱 ESP32-WROOM | Estado del sensor R305 |
| `server:info_fingerprint` | 🖥️ ESP32-C3 → 📱 ESP32-WROOM | Información del dispositivo |
| `client:scan` | 📱 ESP32-WROOM | Escaneo directo local |
| `client:info` | 📱 ESP32-WROOM | Info local del sensor |
| `reset:server` | 🖥️ ESP32-C3 | Reset del broker |
| `reset:client` | 📱 ESP32-WROOM | Reset del cliente |
| `reset:both` | 🔄 Ambos | Reset completo del sistema |

## 🏃‍♂️ **Flujo de Trabajo Típico**

### **Desarrollo Diario**
```bash
# 1. Conectar hardware y abrir monitor automático
cd monitoresPy && python3 launcher.py

# 2. Desarrollar código en ambos proyectos
# 3. Upload automático detectará cambios
python3 auto_upload.py

# 4. Probar funcionalidad
server:scan_fingerprint
```

### **Debugging**
```bash
# Monitor con debug detallado  
python3 single_terminal_monitor.py

# Upload específico por proyecto
python3 quick_upload.py

# Test de comunicación directa
python3 direct_server_test.py
```

### **Producción**
```bash
# Sistema listo para producción (sin debug)
python3 launcher.py
```

## 🐛 **Troubleshooting**

### **Problemas Comunes**

#### 📡 **"No se detectan dispositivos ESP32"**
```bash
# Verificar puertos
python3 detect_devices.py

# Revisar permisos USB (macOS/Linux)
sudo chmod 666 /dev/tty.usbserial-*
```

#### 🌐 **"Cliente no conecta al broker"**
- ✅ Verificar que ESP32-C3 esté creando red `DEPOSITO_BROKER`
- ✅ Reset del servidor: `reset:server`
- ✅ Verificar IP 192.168.4.1:1883

#### 👆 **"Sensor de huella no responde"**
- ✅ Verificar conexiones R305 al ESP32-WROOM
- ✅ Probar comando local: `client:scan`
- ✅ Reset del cliente: `reset:client`

#### 🔌 **"Error de puerto ocupado"**
```bash
# El sistema auto-limpia puertos, pero si persiste:
sudo killall -9 python3
python3 launcher.py
```

## 🤝 **Contribución**

1. Fork del repositorio
2. Crear branch: `git checkout -b feature/nueva-funcionalidad`
3. Commit: `git commit -am 'Agregar nueva funcionalidad'`
4. Push: `git push origin feature/nueva-funcionalidad`
5. Pull Request

## 📜 **Licencia**

Este proyecto está bajo la Licencia MIT - ver el archivo [LICENSE](LICENSE) para detalles.

## 👥 **Autor**

**Juan Moisio** - [GitHub](https://github.com/JuanMoisio)

## 🙏 **Agradecimientos**

- **Espressif** por el increíble ecosistema ESP32
- **PlatformIO** por la herramienta de desarrollo
- **ArduinoJson** por la biblioteca JSON eficiente
- **Adafruit** por las librerías de sensores

---

⭐ **¡Si este proyecto te ayuda, dale una estrella!** ⭐