# Comando finger_cut - Eliminación de Huellas por MQTT

## Descripción
El comando `finger_cut` permite eliminar una huella dactilar específica enviando un comando MQTT desde el servidor al cliente.

## Formato del Comando
```
server:finger_cut:id
```

Donde `id` es el número de la huella a eliminar (debe ser un entero positivo).

## Ejemplos de Uso

### Desde el Monitor Serial del Servidor
```bash
# Eliminar la huella con ID 1
server:finger_cut:1

# Eliminar la huella con ID 5
server:finger_cut:5

# Eliminar la huella con ID 10
server:finger_cut:10
```

### Desde Python (usando el script de prueba)
```bash
# Ejecutar el script de prueba
python monitoresPy/test_finger_cut.py 1
python monitoresPy/test_finger_cut.py 5
```

## ¿Cómo Funciona?

1. **Servidor (BorkerMQTT)**: 
   - Recibe el comando `server:finger_cut:id`
   - Busca el módulo de huellas registrado
   - Envía el comando `delete_user:id` al cliente via MQTT

2. **Cliente (HuellaDactilar)**:
   - Recibe el comando `delete_user:id`
   - Elimina las 5 posiciones de huella del usuario (id*5 a id*5+4)
   - Elimina el nombre asociado del almacenamiento
   - Envía confirmación de vuelta al servidor

## Comandos Relacionados

| Comando | Descripción |
|---------|-------------|
| `server:finger_cut:id` | Eliminar huella específica (nuevo) |
| `server:delete:id` | Eliminar huella específica (original) |
| `server:enroll:nombre` | Registrar nueva huella |
| `server:list_fingerprints` | Listar todas las huellas registradas |
| `server:clear_all` | Eliminar todas las huellas |
| `server:scan_fingerprint` | Escanear huella |

## Respuestas del Sistema

El cliente enviará una respuesta JSON con el resultado:

```json
{
  "type": "publish",
  "topic": "devices/fingerprint_xxx/events",
  "payload": {
    "event": "delete_result",
    "module_id": "fingerprint_xxx",
    "timestamp": 1234567890,
    "data": {
      "message": "Usuario eliminado exitosamente",
      "user_id": 1,
      "success": true
    }
  }
}
```

## Logs en el Cliente

Cuando se ejecuta el comando, verás logs como:

```
✂️ Comando MQTT: Cortar/Eliminar huella ID 1
Eliminando slot 5: OK
Eliminando slot 6: OK  
Eliminando slot 7: OK
Eliminando slot 8: OK
Eliminando slot 9: OK
📨 Evento enviado: delete_result
```

## Notas Importantes

- El comando `finger_cut` es un alias amigable para `delete`
- Cada usuario ocupa 5 slots en el sensor (id*5 a id*5+4)
- Se elimina tanto la huella como el nombre asociado
- El servidor debe estar conectado al cliente via MQTT
- El cliente debe estar registrado como `fingerprint_scanner`

## Troubleshooting

### Error: "No hay módulos de huella registrados"
- Verifica que el cliente esté conectado y registrado
- Usa `modules` en el servidor para ver los módulos registrados

### Error: "ID de huella inválido"
- Asegúrate de usar un número entero positivo
- Ejemplo correcto: `server:finger_cut:1`

### Sin respuesta del cliente
- Verifica la conexión WiFi del cliente
- Revisa que el cliente esté en estado MQTT_STATE_CONNECTED