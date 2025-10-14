# Ejemplos del Sistema de Depósito

## 🚀 Instalación Rápida en Raspberry Pi

```bash
# Hacer ejecutable y correr script de instalación
chmod +x install_raspberry.sh
./install_raspberry.sh
```

Este script automáticamente:
- ✅ Conecta la Raspberry Pi al Access Point del ESP32-C3
- ✅ Instala las dependencias Python necesarias  
- ✅ Crea directorio del proyecto con scripts de prueba
- ✅ Verifica la conectividad

## 📁 Archivos Incluidos

### `sensor_module_example.cpp`
Ejemplo de módulo ESP32 que se conecta al broker y simula un sensor de temperatura.

**Hardware sugerido:**
- ESP32 DevKit
- Sensor DHT22 (opcional, se simula)
- LED indicador

### `raspberry_client.py` 
Cliente Python completo para Raspberry Pi con interfaz interactiva.

**Características:**
- 🔗 Conexión automática al broker ESP32-C3
- 📊 Monitoreo en tiempo real de módulos
- 💻 Interfaz de comandos interactiva
- 📋 Listado de módulos registrados
- 🔧 Envío de comandos a módulos

### `install_raspberry.sh`
Script de instalación automática para Raspberry Pi.

## 🔧 Configuración Manual

### ESP32-C3 (Broker)
1. Compilar y subir el código principal
2. El ESP32-C3 creará el AP "DEPOSITO_BROKER"
3. IP fija: `192.168.4.1`

### Módulos ESP32 
1. Actualizar credenciales WiFi en el código:
```cpp
const char* ssid = "DEPOSITO_BROKER";
const char* password = "deposito123";
const char* brokerIP = "192.168.4.1";
```

2. Compilar y subir a cada ESP32 módulo

### Raspberry Pi

**Opción 1 - NetworkManager:**
```bash
nmcli dev wifi connect "DEPOSITO_BROKER" password "deposito123"
```

**Opción 2 - wpa_supplicant:**
```bash
sudo nano /etc/wpa_supplicant/wpa_supplicant.conf
```

Agregar:
```
network={
    ssid="DEPOSITO_BROKER"
    psk="deposito123"
    priority=10
}
```

## 🧪 Pruebas

### Verificar Conectividad
```bash
ping 192.168.4.1
```

### Probar Puerto MQTT
```bash
telnet 192.168.4.1 1883
```

### Ejecutar Cliente Python
```bash
python3 raspberry_client.py
```

## 📡 Red del Sistema

```
ESP32-C3 Broker (Access Point)
├── SSID: DEPOSITO_BROKER  
├── Password: deposito123
├── IP: 192.168.4.1
├── Puerto MQTT: 1883
└── Clientes:
    ├── Raspberry Pi (192.168.4.2)
    ├── Módulo Control Acceso (192.168.4.3)  
    ├── Módulo Motor 1 (192.168.4.4)
    └── Módulo Sensores (192.168.4.5)
```

## 🔍 Troubleshooting

### ESP32-C3 no crea el Access Point
- Verificar alimentación (USB o 3.3V estable)
- Revisar configuración en `include/config.h`
- Comprobar código con monitor serie

### Raspberry Pi no se conecta al AP
- Verificar que el AP esté visible: `nmcli dev wifi`
- Comprobar password: "deposito123" (mínimo 8 caracteres)
- Reiniciar servicios WiFi: `sudo systemctl restart wpa_supplicant`

### Módulos ESP32 no se registran
- Verificar conexión WiFi del módulo
- Comprobar IP del broker (192.168.4.1)
- Revisar formato JSON de mensajes
- Verificar que el tipo de módulo esté permitido

### Sin comunicación MQTT  
- Probar conexión TCP: `telnet 192.168.4.1 1883`
- Verificar firewall (normalmente no hay en ESP32)
- Comprobar que el broker esté iniciado

## 📚 Próximos Pasos

1. **Personalizar módulos** - Adaptar el código de ejemplo a tu hardware específico
2. **Interfaz web** - Crear dashboard web servido desde el ESP32-C3
3. **Persistencia** - Guardar configuración en SPIFFS/LittleFS
4. **Seguridad** - Implementar autenticación MQTT
5. **Monitoreo** - Logs y métricas del sistema