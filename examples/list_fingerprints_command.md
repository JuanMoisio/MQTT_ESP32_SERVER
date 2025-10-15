# Comando list_fingerprints - Listar Huellas Registradas

## Descripción
El comando `list_fingerprints` permite obtener una lista completa de todas las huellas dactilares registradas en el sensor, incluyendo sus IDs y nombres asociados.

## Formato del Comando
```
server:list_fingerprints
```

Este comando no requiere parámetros adicionales.

## Ejemplos de Uso

### Desde el Monitor Serial del Servidor
```bash
# Listar todas las huellas registradas
server:list_fingerprints

# Ver ayuda completa
help
```

### Desde Python (usando el script de prueba)
```bash
# Ejecutar el script de prueba
python monitoresPy/test_list_fingerprints.py
```

## ¿Cómo Funciona?

1. **Servidor (BorkerMQTT)**: 
   - Recibe el comando `server:list_fingerprints`
   - Busca el módulo de huellas registrado
   - Envía el comando `list_all_fingerprints` al cliente via MQTT

2. **Cliente (HuellaDactilar)**:
   - Recibe el comando `list_all_fingerprints`
   - Escanea todos los slots del sensor (IDs 1-40, cada uno con 5 posiciones)
   - Verifica qué slots contienen templates válidos
   - Obtiene los nombres asociados desde el almacenamiento NVS
   - Envía la lista completa de vuelta al servidor

## Respuesta del Sistema

El cliente enviará una respuesta JSON detallada:

```json
{
  "type": "publish",
  "topic": "devices/fingerprint_xxx/events",
  "payload": {
    "event": "fingerprint_list",
    "module_id": "fingerprint_xxx",
    "timestamp": 1234567890,
    "data": {
      "message": "Lista de huellas registradas",
      "registered_ids": [1, 3, 5, 8],
      "registered_names": ["Juan", "María", "Pedro", "Sin nombre"],
      "total_count": 4,
      "success": true
    }
  }
}
```

## Logs en el Cliente

Cuando se ejecuta el comando, verás logs como:

```
📋 Comando MQTT: Listar todas las huellas registradas
👤 Usuario ID 1: Juan
👤 Usuario ID 3: María
👤 Usuario ID 5: Pedro
👤 Usuario ID 8: Sin nombre
📊 Total de usuarios registrados: 4
📨 Evento enviado: fingerprint_list
```

## Información Técnica

### Estructura de Almacenamiento
- Cada usuario ocupa 5 slots consecutivos en el sensor
- ID 1 → slots 5-9
- ID 2 → slots 10-14
- ID 3 → slots 15-19
- etc.

### Rango de IDs
- El sistema escanea IDs del 1 al 40 (máximo 40 usuarios)
- Esto corresponde a slots 5-204 en el sensor
- Los slots 0-4 están reservados para uso del sistema

### Nombres de Usuario
- Los nombres se almacenan en NVS (Non-Volatile Storage)
- Si no hay nombre asociado, se muestra "Sin nombre"
- Los nombres se eliminan automáticamente cuando se borra un ID

## Comandos Relacionados

| Comando | Descripción |
|---------|-------------|
| `server:list_fingerprints` | Listar huellas registradas (nuevo) |
| `server:enroll:nombre` | Registrar nueva huella |
| `server:finger_cut:id` | Eliminar huella específica |
| `server:clear_all` | Eliminar todas las huellas |
| `server:scan_fingerprint` | Escanear/verificar huella |

## Casos de Uso

### Administración del Sistema
```bash
# Ver qué huellas están registradas
server:list_fingerprints

# Eliminar huellas no utilizadas
server:finger_cut:5

# Verificar después de la limpieza
server:list_fingerprints
```

### Debugging
- Verificar si el enrolamiento fue exitoso
- Encontrar IDs disponibles para nuevos usuarios
- Auditar el sistema de huellas
- Verificar integridad de la base de datos

## Troubleshooting

### Error: "No hay módulos de huella registrados"
- Verifica que el cliente esté conectado y registrado
- Usa `modules` en el servidor para ver los módulos activos

### Respuesta vacía o incompleta
- Verifica la conexión del sensor R305
- Asegúrate de que el sensor esté funcionando correctamente
- Revisa que haya huellas realmente registradas

### Timeout en el escaneo
- El escaneo puede tomar varios segundos (escanea 200 slots)
- Aumenta el timeout si es necesario
- El proceso es normal y se ejecuta automáticamente

### Nombres faltantes
- Los nombres son opcionales y se almacenan por separado
- "Sin nombre" es normal para huellas registradas sin nombre asignado
- Usa `server:enroll:nombre` para registrar con nombre desde el inicio