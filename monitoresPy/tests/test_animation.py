#!/usr/bin/env python3
"""
Test rápido de animación - Envía comando y observa timing
"""
import serial
import time
import subprocess

def test_animation():
    print("🧪 TEST DE ANIMACIÓN OPTIMIZADA")
    print("=" * 40)
    
    # Buscar puerto del ESP32-WROOM
    try:
        result = subprocess.run(['python3', 'detect_devices.py'], 
                              capture_output=True, text=True, timeout=10)
        
        if 'ESP32-WROOM' in result.stdout:
            # Extraer puerto (buscar patrón /dev/cu.)
            lines = result.stdout.split('\n')
            client_port = None
            for line in lines:
                if 'ESP32-WROOM' in line and '/dev/cu.' in line:
                    # Buscar el puerto en la línea anterior o siguiente
                    port_line = [l for l in lines if '/dev/cu.' in l and 'usbserial' in l]
                    if port_line:
                        client_port = port_line[0].strip().split()[0]
                        break
            
            if client_port:
                print(f"✅ Puerto CLIENT encontrado: {client_port}")
                
                # Conectar y enviar comando
                try:
                    ser = serial.Serial(client_port, 115200, timeout=1)
                    time.sleep(1)  # Esperar estabilización
                    
                    print("📤 Enviando comando de escaneo...")
                    start_time = time.time()
                    
                    # Enviar comando local para probar animación
                    ser.write(b'scan\n')
                    
                    print("⏱️ Midiendo timing de animación...")
                    animation_start = None
                    animation_end = None
                    
                    # Leer respuestas por 20 segundos
                    timeout = time.time() + 20
                    while time.time() < timeout:
                        if ser.in_waiting:
                            line = ser.readline().decode('utf-8', errors='ignore').strip()
                            if line:
                                current_time = time.time() - start_time
                                print(f"[{current_time:.1f}s] {line}")
                                
                                # Detectar inicio y fin de animación
                                if 'requestScan' in line and animation_start is None:
                                    animation_start = current_time
                                    print(f"🎬 ANIMACIÓN INICIADA a los {current_time:.1f}s")
                                
                                if ('state 2 -> 0' in line or 'ICON_PERMAQUIM' in line) and animation_end is None:
                                    animation_end = current_time  
                                    print(f"🏁 ANIMACIÓN TERMINADA a los {current_time:.1f}s")
                                    
                                    if animation_start:
                                        duration = animation_end - animation_start
                                        print(f"⏱️ DURACIÓN TOTAL: {duration:.1f} segundos")
                                        
                                        if duration < 3.0:
                                            print("🚀 ¡ANIMACIÓN OPTIMIZADA! Duración buena")
                                        elif duration < 5.0:
                                            print("⚡ Animación mejorada, pero puede optimizarse más")  
                                        else:
                                            print("🐌 Animación todavía lenta")
                                    break
                        time.sleep(0.1)
                    
                    ser.close()
                    
                except Exception as e:
                    print(f"❌ Error de conexión serial: {e}")
            else:
                print("❌ No se pudo identificar puerto CLIENT")
        else:
            print("❌ ESP32-WROOM no detectado")
            
    except Exception as e:
        print(f"❌ Error en detección: {e}")

if __name__ == "__main__":
    test_animation()
