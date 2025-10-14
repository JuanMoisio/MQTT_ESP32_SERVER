# 🔐 ESP32 Fingerprint System - Monitores

Sistema de monitoreo y gestión para el proyecto de huellas dactilares con ESP32.

## 📁 Estructura de Archivos

```
monitoresPy/
├── main.py              # 🎯 Script principal (START HERE!)
├── setup_complete.py    # 🚀 Configuración automática completa
├── simple_monitor.py    # 📺 Monitores serie simples (Terminal)
├── dual_monitor.py      # 🔧 Monitor avanzado Python
└── README.md           # 📖 Esta documentación
```

## 🚀 Uso Rápido

### Opción 1: Script Principal (Recomendado)
```bash
cd /Users/jmoisio/Documents/PlatformIO/Projects/monitoresPy
python3 main.py
```

### Opción 2: Setup Completo Directo
```bash
python3 /Users/jmoisio/Documents/PlatformIO/Projects/monitoresPy/setup_complete.py
```

## 🎯 Descripción de Scripts

### `main.py` - Script Principal
- **Función**: Menú interactivo con todas las opciones
- **Uso**: Punto de entrada principal del sistema
- **Opciones**: 
  - Setup completo automatizado
  - Solo monitores
  - Upload individual de código
  - Monitor avanzado

### `setup_complete.py` - Configuración Automática
- **Función**: Automatiza todo el proceso de configuración
- **Pasos**:
  1. Detecta puertos automáticamente
  2. Sube código al ESP32-C3 (Broker MQTT)
  3. Sube código al ESP32-WROOM (Cliente Fingerprint)
  4. Abre monitores serie duales

### `simple_monitor.py` - Monitores Simples
- **Función**: Abre dos terminales con `screen`
- **Ventaja**: Simple y confiable
- **Uso**: Cuando solo quieres monitorear ambos dispositivos

### `dual_monitor.py` - Monitor Avanzado
- **Función**: Monitor Python con colores y comandos interactivos
- **Características**:
  - Identificación automática de dispositivos
  - Colores por dispositivo
  - Envío de comandos interactivo
  - Timestamps en mensajes

## 🔧 Hardware Requerido

- **ESP32-C3 SuperMini**: Broker MQTT (Puerto USB CDC)
- **ESP32 WROOM**: Cliente Fingerprint (Puerto USB Serial)
- **Sensor R305**: Lector de huellas dactilares
- **Display SH1106**: OLED 128x64

## 📋 Configuración de Red

### ESP32-C3 (Broker)
- **SSID**: `DEPOSITO_BROKER`
- **Password**: `deposito123`
- **IP**: `192.168.4.1`
- **Puerto MQTT**: `1883`

### ESP32-WROOM (Cliente)
Se conecta automáticamente a la red del broker tras la configuración inicial.

## 🎮 Comandos Disponibles

Una vez conectado el sistema, comandos en el cliente:

```bash
scan        # Escanear huella dactilar
help        # Ver todos los comandos
status      # Estado del sistema
enroll 1    # Enrollar nueva huella (ID 1)
delete 1    # Eliminar huella (ID 1)
```

## 🚨 Troubleshooting

### Problema: Puertos no detectados
**Solución**: Verificar conexiones USB y que ambos ESP32 estén conectados

### Problema: Upload falla
**Solución**: Verificar que el puerto correcto esté siendo usado en `platformio.ini`

### Problema: Monitor no conecta
**Solución**: Cerrar otros programas que usen puerto serie (Arduino IDE, etc.)

### Problema: Cliente no encuentra red
**Solución**: Verificar que ESP32-C3 esté funcionando y creando AP

## 📞 Uso del Sistema

1. **Ejecutar**: `python3 main.py`
2. **Elegir opción**: `1` (Setup completo)
3. **Esperar**: Configuración automática
4. **Conectar**: ESP32-WROOM a red "DEPOSITO_BROKER"
5. **Usar**: Comandos de fingerprint en terminal del cliente

---

*Creado para el proyecto ESP32 Fingerprint System*