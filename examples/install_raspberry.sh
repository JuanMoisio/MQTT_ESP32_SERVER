#!/bin/bash
#
# Script de instalación para Raspberry Pi - Sistema Depósito
# Configura automáticamente la conexión al ESP32-C3 Access Point
#

echo "🏭 Configuración Raspberry Pi - Sistema Depósito"
echo "================================================="

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuración del ESP32-C3 Access Point
ESP32_SSID="DEPOSITO_BROKER"
ESP32_PASSWORD="deposito123"
ESP32_IP="192.168.4.1"

echo -e "${BLUE}📡 Configurando conexión a ESP32-C3 Access Point...${NC}"

# Verificar si NetworkManager está disponible
if command -v nmcli &> /dev/null; then
    echo -e "${GREEN}✓ NetworkManager encontrado${NC}"
    
    # Conectar al Access Point del ESP32-C3
    echo -e "${YELLOW}🔗 Conectando a $ESP32_SSID...${NC}"
    
    if nmcli dev wifi connect "$ESP32_SSID" password "$ESP32_PASSWORD"; then
        echo -e "${GREEN}✅ Conectado exitosamente al ESP32-C3!${NC}"
        
        # Verificar conectividad
        echo -e "${YELLOW}🧪 Verificando conectividad...${NC}"
        if ping -c 3 "$ESP32_IP" &> /dev/null; then
            echo -e "${GREEN}✅ Ping exitoso a $ESP32_IP${NC}"
        else
            echo -e "${RED}❌ No se puede hacer ping a $ESP32_IP${NC}"
        fi
    else
        echo -e "${RED}❌ Error conectando a $ESP32_SSID${NC}"
        echo -e "${YELLOW}💡 Intentando configuración manual...${NC}"
        
        # Configuración manual con wpa_supplicant
        WPA_CONFIG="/etc/wpa_supplicant/wpa_supplicant.conf"
        
        echo -e "${YELLOW}📝 Agregando configuración a $WPA_CONFIG${NC}"
        
        sudo tee -a "$WPA_CONFIG" > /dev/null <<EOF

# ESP32-C3 Depósito Broker
network={
    ssid="$ESP32_SSID"
    psk="$ESP32_PASSWORD"
    priority=10
}
EOF
        
        echo -e "${GREEN}✓ Configuración agregada${NC}"
        echo -e "${YELLOW}🔄 Reiniciando servicios WiFi...${NC}"
        
        sudo systemctl restart wpa_supplicant
        sudo systemctl restart dhcpcd
        
        sleep 5
        
        if ping -c 3 "$ESP32_IP" &> /dev/null; then
            echo -e "${GREEN}✅ Conexión establecida después del reinicio${NC}"
        else
            echo -e "${RED}❌ Aún no hay conexión. Revisa la configuración manualmente.${NC}"
        fi
    fi
    
else
    echo -e "${YELLOW}⚠️ NetworkManager no encontrado, usando wpa_supplicant...${NC}"
    
    # Configuración directa con wpa_supplicant
    WPA_CONFIG="/etc/wpa_supplicant/wpa_supplicant.conf"
    
    if [ ! -f "$WPA_CONFIG" ]; then
        echo -e "${RED}❌ $WPA_CONFIG no existe${NC}"
        echo -e "${YELLOW}💡 Creando archivo de configuración básico...${NC}"
        
        sudo tee "$WPA_CONFIG" > /dev/null <<EOF
country=US
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

network={
    ssid="$ESP32_SSID"
    psk="$ESP32_PASSWORD"
    priority=10
}
EOF
    else
        echo -e "${YELLOW}📝 Agregando configuración a archivo existente...${NC}"
        
        sudo tee -a "$WPA_CONFIG" > /dev/null <<EOF

# ESP32-C3 Depósito Broker  
network={
    ssid="$ESP32_SSID"
    psk="$ESP32_PASSWORD"
    priority=10
}
EOF
    fi
    
    echo -e "${GREEN}✓ Configuración WiFi agregada${NC}"
fi

# Instalar dependencias Python
echo -e "${BLUE}🐍 Instalando dependencias Python...${NC}"

if command -v pip3 &> /dev/null; then
    echo -e "${YELLOW}📦 Instalando paho-mqtt...${NC}"
    pip3 install paho-mqtt
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✅ paho-mqtt instalado${NC}"
    else
        echo -e "${RED}❌ Error instalando paho-mqtt${NC}"
    fi
else
    echo -e "${RED}❌ pip3 no encontrado${NC}"
    echo -e "${YELLOW}💡 Instalar con: sudo apt update && sudo apt install python3-pip${NC}"
fi

# Crear directorio para el proyecto
PROJECT_DIR="/home/pi/deposito"
echo -e "${BLUE}📁 Creando directorio del proyecto...${NC}"

if [ ! -d "$PROJECT_DIR" ]; then
    mkdir -p "$PROJECT_DIR"
    echo -e "${GREEN}✓ Directorio creado: $PROJECT_DIR${NC}"
else
    echo -e "${YELLOW}⚠️ Directorio ya existe: $PROJECT_DIR${NC}"
fi

# Crear script de prueba
echo -e "${BLUE}📝 Creando script de prueba...${NC}"

cat > "$PROJECT_DIR/test_connection.py" << 'EOF'
#!/usr/bin/env python3
"""
Script de prueba de conexión al broker ESP32-C3
"""
import socket
import time

def test_connection():
    esp32_ip = "192.168.4.1"
    mqtt_port = 1883
    
    print(f"🧪 Probando conexión TCP a {esp32_ip}:{mqtt_port}")
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        
        result = sock.connect_ex((esp32_ip, mqtt_port))
        
        if result == 0:
            print("✅ Conexión TCP exitosa!")
            print("🎉 El broker MQTT está accesible")
            return True
        else:
            print("❌ No se puede conectar al broker")
            return False
            
    except Exception as e:
        print(f"❌ Error: {e}")
        return False
        
    finally:
        sock.close()

if __name__ == "__main__":
    test_connection()
EOF

chmod +x "$PROJECT_DIR/test_connection.py"

# Información final
echo ""
echo -e "${GREEN}🎉 INSTALACIÓN COMPLETADA${NC}"
echo "=============================="
echo -e "${BLUE}📋 Información de conexión:${NC}"
echo "   SSID: $ESP32_SSID"
echo "   Password: $ESP32_PASSWORD"  
echo "   IP Broker: $ESP32_IP"
echo "   Puerto MQTT: 1883"
echo ""
echo -e "${BLUE}🧪 Probar conexión:${NC}"
echo "   cd $PROJECT_DIR"
echo "   python3 test_connection.py"
echo ""
echo -e "${BLUE}📱 Comandos útiles:${NC}"
echo "   nmcli dev wifi                    # Ver redes WiFi disponibles"
echo "   nmcli con show                    # Ver conexiones"
echo "   ping 192.168.4.1                 # Ping al ESP32-C3"
echo "   iwconfig                          # Info interfaz WiFi"
echo ""
echo -e "${YELLOW}💡 Si hay problemas de conexión:${NC}"
echo "   1. Verificar que el ESP32-C3 esté encendido"
echo "   2. Verificar que el Access Point esté activo"
echo "   3. Revisar /var/log/syslog para errores"
echo "   4. Reiniciar con: sudo systemctl restart wpa_supplicant"

# Prueba automática
echo ""
echo -e "${BLUE}🧪 Ejecutando prueba de conexión...${NC}"
cd "$PROJECT_DIR" && python3 test_connection.py

echo ""
echo -e "${GREEN}✨ ¡Listo para usar el sistema de depósito!${NC}"