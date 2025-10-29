#include "web_interface.h"

const char* getAdminHTML() {
  return R"HTML(
<!DOCTYPE html>
<html>
<head>
<title>Broker MQTT - Dashboard</title>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { font-family: 'Arial', sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; }
.container { max-width: 1200px; margin: 0 auto; padding: 20px; }
.login-card { background: white; padding: 40px; border-radius: 12px; box-shadow: 0 10px 30px rgba(0,0,0,0.3); max-width: 400px; margin: 100px auto; }
.login-card h2 { text-align: center; margin-bottom: 30px; color: #333; }
.admin-panel { background: white; border-radius: 12px; box-shadow: 0 10px 30px rgba(0,0,0,0.3); overflow: hidden; }
.header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 20px; text-align: center; }
.header h1 { margin: 0; font-size: 24px; }
.header .status { margin-top: 10px; font-size: 14px; }
.tabs { display: flex; background: #f8f9fa; border-bottom: 1px solid #ddd; }
.tab { padding: 15px 25px; cursor: pointer; border: none; background: transparent; transition: all 0.3s; font-weight: 500; }
.tab:hover { background: rgba(102,126,234,0.1); }
.tab.active { background: white; border-bottom: 3px solid #667eea; color: #667eea; }
.tab-content { padding: 25px; min-height: 500px; }
.hidden { display: none; }
.device-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(300px, 1fr)); gap: 20px; margin-top: 20px; }
.device-card { background: white; border: 1px solid #e1e5e9; border-radius: 8px; padding: 20px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); transition: transform 0.2s; }
.device-card:hover { transform: translateY(-2px); }
.device-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px; }
.device-id { font-weight: bold; font-size: 16px; color: #333; }
.status-led { width: 12px; height: 12px; border-radius: 50%; margin-left: 0; margin-right: 12px; }
.status-online { background: #28a745; box-shadow: 0 0 10px rgba(40,167,69,0.5); }
.status-offline { background: #dc3545; box-shadow: 0 0 10px rgba(220,53,69,0.5); }
.device-details { font-size: 14px; color: #666; }
.device-type { background: #e9ecef; padding: 4px 8px; border-radius: 4px; font-size: 12px; margin-top: 10px; display: inline-block; }
.form-group { margin-bottom: 20px; }
.form-group label { display: block; margin-bottom: 8px; font-weight: 600; color: #333; }
.form-group input, .form-group select, .form-group textarea { width: 100%; padding: 12px; border: 2px solid #e1e5e9; border-radius: 6px; font-size: 14px; transition: border-color 0.3s; }
.form-group input:focus, .form-group select:focus, .form-group textarea:focus { outline: none; border-color: #667eea; }
.scan-controls { margin-bottom: 20px; display: flex; gap: 10px; align-items: center; }
.scan-status { padding: 15px; border-radius: 6px; margin-bottom: 20px; font-weight: 500; }
.scan-status.scanning { background: #fff3cd; border: 1px solid #ffeaa7; color: #856404; }
.scan-status.completed { background: #d4edda; border: 1px solid #c3e6cb; color: #155724; }
.scan-status.error { background: #f8d7da; border: 1px solid #f5c6cb; color: #721c24; }
.scan-results { margin-top: 20px; }
.scan-device-card { background: white; border: 1px solid #e1e5e9; border-radius: 8px; padding: 20px; margin-bottom: 15px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
.scan-device-card.registered { border-left: 4px solid #28a745; background: #f8fff9; }
.scan-device-card.unregistered { border-left: 4px solid #ffc107; background: #fffdf5; }
.scan-device-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px; }
.scan-device-info { flex: 1; }
.scan-device-mac { font-family: 'Courier New', monospace; font-weight: bold; font-size: 16px; color: #333; }
.scan-device-type { background: #e9ecef; padding: 4px 8px; border-radius: 4px; font-size: 12px; margin-top: 5px; display: inline-block; }
.scan-device-status { font-size: 14px; margin-top: 5px; }
.scan-device-actions { display: flex; gap: 10px; align-items: center; }
.registered-badge { background: #28a745; color: white; padding: 4px 8px; border-radius: 4px; font-size: 12px; font-weight: bold; }
.unregistered-badge { background: #ffc107; color: #333; padding: 4px 8px; border-radius: 4px; font-size: 12px; font-weight: bold; }
.btn { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 12px 24px; border: none; border-radius: 6px; cursor: pointer; font-weight: 600; transition: all 0.3s; }
.btn:hover { transform: translateY(-1px); box-shadow: 0 5px 15px rgba(0,0,0,0.3); }
.btn-secondary { background: #6c757d; }
.btn-danger { background: #dc3545; }
.btn-small { padding: 8px 16px; font-size: 12px; }
.message { margin: 15px 0; padding: 15px; border-radius: 6px; }
.success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
.error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
.stats-row { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin-bottom: 30px; }
.stat-card { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 20px; border-radius: 8px; text-align: center; }
.stat-number { font-size: 28px; font-weight: bold; margin-bottom: 5px; }
.stat-label { font-size: 14px; opacity: 0.9; }
.mac-input-group { display: flex; gap: 10px; }
.mac-input-group input { flex: 1; }
.refresh-indicator { display: inline-block; margin-left: 10px; opacity: 0; transition: opacity 0.3s; }
.refresh-indicator.active { opacity: 1; }
@keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
.spinning { animation: spin 1s linear infinite; }

/* NUEVO: Estilos para el menú de acciones del módulo */
.module-menu {
  position: fixed;
  right: 20px;
  top: 80px;
  width: 340px;
  max-width: calc(100% - 40px);
  background: white;
  border-radius: 8px;
  box-shadow: 0 10px 30px rgba(0,0,0,0.25);
  z-index: 9999;
  display: none;
  padding: 16px;
}
.module-menu.show { display: block; }
.module-menu .header { display:flex; justify-content:space-between; align-items:center; margin-bottom:8px; }
.module-menu .title { font-weight:700; font-size:16px; color:#333; }
.module-menu .close-btn { background:#dc3545; color:white; border:none; padding:6px 10px; border-radius:6px; cursor:pointer; }
</style>
</head>
<body>
<div class="container">
  <!-- Login Section -->
  <div id="loginSection">
    <div class="login-card">
      <h2>🔐 Dashboard Login</h2>
      <div class="form-group">
        <label>Usuario</label>
        <input type="text" id="username" value="admin">
      </div>
      <div class="form-group">
        <label>Password</label>
        <input type="password" id="password" value="deposito123">
      </div>
      <button class="btn" onclick="login()" style="width:100%" id="loginBtn">Entrar al Dashboard</button>
      <div id="message"></div>
      <!-- debug UI removed -->
    </div>
  </div>
  
  <!-- Admin Panel -->
  <div id="adminSection" class="hidden">
    <div class="admin-panel">
      <div class="header">
        <h1>🚀 Broker MQTT Dashboard</h1>
        <div class="status">
          Estado: <strong>Online</strong> | IP: <span id="brokerIP">192.168.4.1</span>
          <span class="refresh-indicator" id="refreshIndicator">🔄</span>
        </div>
      </div>
      
      <div class="tabs">
        <button class="tab active" onclick="showTab('dashboard')">📊 Dashboard</button>
        <button class="tab" onclick="showTab('scan')">🔍 Scan Devices</button>
        <button class="tab" onclick="showTab('register')">➕ Registro Manual</button>
        <button class="tab" onclick="showTab('modules')">📱 Módulos</button>
      </div>
      
      <!-- Dashboard Tab -->
      <div id="dashboardTab" class="tab-content">
        <div class="stats-row">
          <div class="stat-card">
            <div class="stat-number" id="totalDevices">0</div>
            <div class="stat-label">Dispositivos Registrados</div>
          </div>
          <div class="stat-card">
            <div class="stat-number" id="connectedDevices">0</div>
            <div class="stat-label">Dispositivos Conectados</div>
          </div>
          <div class="stat-card">
            <div class="stat-number" id="depositarioStatus">❌</div>
            <div class="stat-label">Depositario</div>
          </div>
          <div class="stat-card">
            <div class="stat-number" id="placaMotoresStatus">❌</div>
            <div class="stat-label">Placa Motores</div>
          </div>
        </div>
        
        <h3>🔗 Dispositivos Conectados</h3>
        <div id="deviceGrid" class="device-grid">
          <!-- Dispositivos se cargan aquí -->
        </div>
      </div>
      
      <!-- Scan Devices Tab -->
      <div id="scanTab" class="tab-content hidden">
        <h3>🔍 Escanear Dispositivos Disponibles</h3>
        <p>Busca automáticamente todos los dispositivos conectados al broker y muestra cuáles están y cuáles no están registrados.</p>
        
        <div class="scan-controls">
          <button class="btn" onclick="scanAllDevices()" id="scanBtn">🔍 Escanear Dispositivos</button>
          <button class="btn btn-secondary" onclick="clearScanResults()" id="clearBtn">🗑️ Limpiar Resultados</button>
          <button class="btn btn-secondary" onclick="debugScan()" id="debugBtn">🐛 Debug Info</button>
        </div>
        
        <div id="scanStatus" class="scan-status hidden"></div>
        
        <div id="scanResults" class="scan-results">
          <!-- Los resultados del scan aparecerán aquí -->
        </div>
      </div>
      
      <!-- Register Tab -->
      <div id="registerTab" class="tab-content hidden">
        <h3>➕ Registro Manual de Dispositivos</h3>
        <div class="form-group">
          <label>MAC Address</label>
          <div class="mac-input-group">
            <input type="text" id="newMac" placeholder="AA:BB:CC:DD:EE:FF">
            <button type="button" class="btn btn-secondary" onclick="requestMAC()" id="requestMacBtn">Consultar</button>
          </div>
        </div>
        <div class="form-group">
          <label>Tipo de Dispositivo</label>
          <select id="newType">
            <option value="fingerprint">🔒 Lector de Huellas</option>
            <option value="rfid">📱 Lector RFID</option>
            <option value="camera">📹 Cámara</option>
            <option value="sensor">📡 Sensor</option>
            <option value="actuator">⚙️ Actuador</option>
          </select>
        </div>
        <div class="form-group">
          <label>Descripción</label>
          <textarea id="newDesc" placeholder="Descripción del dispositivo (ej: Scanner entrada principal)" rows="3"></textarea>
        </div>
        <button class="btn" onclick="addDevice()">Agregar Dispositivo</button>
        <div id="registerMessage"></div>
      </div>
      
      <!-- Modules Tab -->
      <div id="modulesTab" class="tab-content hidden">
        <h3>📱 Módulos Registrados</h3>
        <div id="modulesList">
          <!-- Módulos se cargan aquí -->
        </div>
      </div>

      <!-- Contenedor del menú de acciones por módulo (invisible hasta activarse) -->
      <div id="moduleMenu" class="module-menu" role="dialog" aria-hidden="true">
        <div class="header">
          <div class="title" id="moduleMenuTitle">Acciones módulo</div>
          <button class="close-btn" onclick="closeModuleMenu()">Cerrar</button>
        </div>
        <div id="moduleMenuBody">
          <!-- Opciones específicas del módulo se renderizan aquí -->
          <p style="color:#666;margin:8px 0">Seleccione una acción...</p>
        </div>
      </div>
    </div>
  </div>
</div>

<script>
let refreshInterval;
let currentTab = 'dashboard';

function showMessage(msg, isError, target = 'message') {
  const el = document.getElementById(target);
  el.innerHTML = '<div class="' + (isError ? 'error' : 'success') + '">' + msg + '</div>';
  setTimeout(() => el.innerHTML = '', 5000);
}

function showTab(tabName) {
  // Ocultar todas las pestañas
  document.querySelectorAll('.tab-content').forEach(tab => tab.classList.add('hidden'));
  document.querySelectorAll('.tab').forEach(tab => tab.classList.remove('active'));
  
  // Mostrar pestaña seleccionada
  document.getElementById(tabName + 'Tab').classList.remove('hidden');

  // Asignar .active al botón cuyo onclick contiene el tabName (robusto ante event undefined)
  const tabs = document.querySelectorAll('.tab');
  for (const t of tabs) {
    const onclickAttr = t.getAttribute('onclick') || '';
    if (onclickAttr.indexOf("'" + tabName + "'") !== -1 || onclickAttr.indexOf('"' + tabName + '"') !== -1) {
      t.classList.add('active');
      break;
    }
  }

  currentTab = tabName;
  
  // Cargar datos específicos de cada pestaña
  if (tabName === 'dashboard') {
    loadDashboard();
  } else if (tabName === 'modules') {
    loadModules();
  } else if (tabName === 'scan') {
    // La pestaña de scan se carga manualmente al hacer clic en "Escanear"
    console.log('Pestaña de scan activada');
  }
}

function login() {
  const u = document.getElementById('username').value;
  const p = document.getElementById('password').value;
  const loginBtn = document.getElementById('loginBtn');
  
  loginBtn.disabled = true;
  loginBtn.textContent = 'Conectando...';
  showMessage('Conectando al servidor...', false);
  
  const x = new XMLHttpRequest();
  x.timeout = 10000;
  x.open('POST', '/api/login');
  x.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
  
  x.onreadystatechange = function() {
    if (x.readyState == 4) {
      loginBtn.disabled = false;
      loginBtn.textContent = 'Entrar al Dashboard';
      
      if (x.status == 200) {
        try {
          const d = JSON.parse(x.responseText);
          if (d.success) {
            showMessage('Login exitoso! Cargando dashboard...', false);
            
            // Cambiar pantallas
            setTimeout(() => {
              document.getElementById('loginSection').style.display = 'none';
              document.getElementById('adminSection').style.display = 'block';
              // Cargar dashboard simplificado
              loadDashboardSimple();
            }, 500);
            
          } else {
            showMessage('Error: ' + d.message, true);
          }
          } catch (e) {
            showMessage('Error de comunicación', true);
          }
      } else {
        // Error HTTP en login
        showMessage('Error de conexión (HTTP ' + x.status + ')', true);
      }
    }
  };
  
  x.ontimeout = function() {
    loginBtn.disabled = false;
    loginBtn.textContent = 'Entrar al Dashboard';
    showMessage('Tiempo de espera agotado', true);
  };

  x.onerror = function() {
    loginBtn.disabled = false;
    loginBtn.textContent = 'Entrar al Dashboard';
    showMessage('Error de red', true);
  };
  
  x.send('username=' + encodeURIComponent(u) + '&password=' + encodeURIComponent(p));
}

// Asegurar que loadDashboardStats exista antes de usarla desde loadDashboard
function loadDashboardStats() {
  return new Promise((resolve, reject) => {
    console.log('Cargando estadísticas...');
    const x = new XMLHttpRequest();
    x.timeout = 5000; // 5 segundos timeout
    x.open('GET', '/api/stats');

    x.onreadystatechange = function() {
      if (x.readyState == 4) {
        if (x.status == 200) {
          try {
            const d = JSON.parse(x.responseText);
            console.log('Stats recibidas:', d);
            // Actualizar solo lo que corresponda aquí. connectedDevices se calcula desde /api/devices.
            document.getElementById('totalDevices').textContent = d.registered_devices || 0;
            document.getElementById('depositarioStatus').textContent = d.depositario_status ? '✅' : '❌';
            document.getElementById('placaMotoresStatus').textContent = d.placa_motores_status ? '✅' : '❌';
            resolve();
          } catch (e) {
            console.error('Error parsing stats:', e);
            reject(e);
          }
        } else {
          console.error('Stats HTTP error:', x.status);
          reject(new Error('HTTP ' + x.status));
        }
      }
    };

    x.ontimeout = function() {
      console.error('Stats timeout');
      reject(new Error('Timeout'));
    };

    x.onerror = function() {
      console.error('Stats network error');
      reject(new Error('Network error'));
    };

    x.send();
  });
}

function loadDashboard() {
  console.log('Cargando dashboard...');
  showRefreshIndicator(true);
  
  // Cargar cada función con timeout individualizado
  const loadPromises = [
    loadDashboardStats().catch(e => {
      console.log('Error loadDashboardStats:', e);
      // Mostrar valores por defecto en caso de error
      document.getElementById('totalDevices').textContent = '0';
      document.getElementById('connectedDevices').textContent = '0';
      document.getElementById('depositarioStatus').textContent = '❌';
      document.getElementById('placaMotoresStatus').textContent = '❌';
    }),
    loadDevices().catch(e => {
      console.log('Error loadDevices:', e);
      document.getElementById('deviceGrid').innerHTML = '<div style="text-align:center;padding:40px;color:#666">Error cargando dispositivos</div>';
    }),
    loadModules().catch(e => {
      console.log('Error loadModules:', e);
      document.getElementById('modulesList').innerHTML = '<div style="text-align:center;padding:40px;color:#666">Error cargando módulos</div>';
    }),
    loadSystemInfo().catch(e => {
      console.log('Error loadSystemInfo:', e);
    })
  ];
  
  // Esperar que todas las promesas terminen (exitosas o con error)
  Promise.allSettled(loadPromises).then(() => {
    console.log('Dashboard cargado completamente');
    showRefreshIndicator(false);
  }).catch(e => {
    console.error('Error crítico en loadDashboard:', e);
    showRefreshIndicator(false);
  });
}

function loadDashboardSimple() {
  // Carga simplificada del dashboard
  showRefreshIndicator(true);
  
  // Establecer valores por defecto primero
  document.getElementById('totalDevices').textContent = '0';
  document.getElementById('connectedDevices').textContent = '0';  
 document.getElementById('depositarioStatus').textContent = '❌';
  document.getElementById('placaMotoresStatus').textContent = '❌';
  document.getElementById('deviceGrid').innerHTML = '<div style="text-align:center;padding:40px;color:#666">Cargando dispositivos...</div>';
  document.getElementById('modulesList').innerHTML = '<div style="text-align:center;padding:40px;color:#666">Cargando módulos...</div>';
  
  // Cargar stats y dispositivos
  loadStatsOnly().then(() => {
    return loadDevicesOnly();
  }).then(() => {
    showRefreshIndicator(false);
    startAutoRefresh();
  }).catch(e => {
    showRefreshIndicator(false);
  });
}

function loadStatsOnly() {
  return new Promise((resolve, reject) => {
    const x = new XMLHttpRequest();
    x.timeout = 3000;
    x.open('GET', '/api/stats');
    x.onreadystatechange = function() {
      if (x.readyState == 4) {
        if (x.status == 200) {
          try {
            const d = JSON.parse(x.responseText);
            // Sólo actualizar totalDevices aquí; connectedDevices lo calcula loadDevices/loadDevicesOnly
            document.getElementById('totalDevices').textContent = d.registered_devices || 0;
            resolve();
          } catch (e) {
            resolve();
          }
        } else {
          resolve();
        }
      }
    };
    x.ontimeout = () => { resolve(); };
    x.onerror = () => { resolve(); };
    x.send();
  });
}

function loadDevicesOnly() {
  return new Promise((resolve, reject) => {
    const x = new XMLHttpRequest();
    x.timeout = 3000;
    x.open('GET', '/api/devices');
    x.onreadystatechange = function() {
      if (x.readyState == 4) {
        if (x.status == 200) {
          try {
            const d = JSON.parse(x.responseText);
            const devices = d.devices || [];
            displayDevicesDashboard(devices);

            // NUEVO: calcular connectedDevices localmente (misma lógica que loadDevices)
            try {
              const connectedCount = devices.filter(dev => {
                return (dev.currentIP && dev.currentIP !== '') ||
                       (!!dev.isConnected) ||
                       isLastSeenRecent(dev.lastSeen, 5);
              }).length;
              document.getElementById('connectedDevices').textContent = connectedCount;
            } catch (e) {
              console.warn('No se pudo calcular connectedDevices desde devices (loadDevicesOnly):', e);
            }

            resolve();
          } catch (e) {
            document.getElementById('deviceGrid').innerHTML = '<div style="color:red;">Error cargando dispositivos</div>';
            resolve();
          }
        } else {
          document.getElementById('deviceGrid').innerHTML = '<div style="color:red;">Error HTTP: ' + x.status + '</div>';
          resolve();
        }
      }
    };
    x.ontimeout = () => { 
      document.getElementById('deviceGrid').innerHTML = '<div style="color:red;">Timeout cargando dispositivos</div>';
      resolve(); 
    };
    x.onerror = () => { 
      document.getElementById('deviceGrid').innerHTML = '<div style="color:red;">Error de red</div>';
      resolve(); 
    };
    x.send();
  });
}

function loadDevices() {
  return new Promise((resolve, reject) => {
    console.log('Cargando dispositivos...');
    const x = new XMLHttpRequest();
    x.timeout = 5000; // 5 segundos timeout
    x.open('GET', '/api/devices');
    
    x.onreadystatechange = function() {
      if (x.readyState == 4) {
        if (x.status == 200) {
          try {
            const d = JSON.parse(x.responseText);
            console.log('Dispositivos recibidos:', d.devices?.length || 0);
            const devices = d.devices || [];
            displayDevicesDashboard(devices);

            // NUEVO: calcular connectedDevices localmente a partir de la lista recibida
            try {
              const connectedCount = devices.filter(dev => {
                // Considerar conectado si hay IP, o flag isConnected, o lastSeen reciente
                return (dev.currentIP && dev.currentIP !== '') ||
                       (!!dev.isConnected) ||
                       isLastSeenRecent(dev.lastSeen, 5);
              }).length;
              document.getElementById('connectedDevices').textContent = connectedCount;
            } catch (e) {
              console.warn('No se pudo calcular connectedDevices desde devices:', e);
            }

            resolve();
          } catch (e) {
            console.error('Error parsing devices:', e);
            reject(e);
          }
        } else {
          console.error('Devices HTTP error:', x.status);
          reject(new Error('HTTP ' + x.status));
        }
      }
    };
    
    x.ontimeout = function() {
      console.error('Devices timeout');
      reject(new Error('Timeout'));
    };
    
    x.onerror = function() {
      console.error('Devices network error');
      reject(new Error('Network error'));
    };
    
    x.send();
  });
}

function displayDevicesDashboard(devices) {
  // Mostrar únicamente dispositivos conectados en el dashboard
  const connectedDevices = (devices || []).filter(device => {
    const isOnline = (device.currentIP && device.currentIP !== '') ||
                     (!!device.isConnected) ||
                     isLastSeenRecent(device.lastSeen, 5);
    return isOnline;
  });

  let html = '';
  if (connectedDevices.length === 0) {
    html = '<div style="text-align:center;padding:40px;color:#666">No hay dispositivos conectados</div>';
  } else {
    connectedDevices.forEach(device => {
      const isOnline = true; // ya filtrado
      const statusClass = isOnline ? 'status-online' : 'status-offline';
      const statusText = isOnline ? 'Online' : 'Offline';
      
      html += `
        <div class="device-card">
          <div class="device-header">
            <div class="device-id">${device.macAddress}</div>
            <div class="status-led ${statusClass}" title="${statusText}"></div>
          </div>
          <div class="device-details">
            <div><strong>Tipo:</strong> ${getDeviceTypeIcon(device.deviceType)} ${getDeviceTypeLabel(device.deviceType)}</div>
            <div><strong>Estado:</strong> ${statusText}</div>
            <div><strong>Descripción:</strong> ${device.description || 'Sin descripción'}</div>
            <div><strong>IP:</strong> ${device.currentIP || 'N/A'}</div>
            <div><strong>Última conexión:</strong> ${formatLastSeen(device.lastSeen)}</div>
          </div>
          <div style="margin-top:15px">
            <!-- Botón eliminar removido del Dashboard (solo visual) -->
          </div>
        </div>
      `;
    });
  }
  document.getElementById('deviceGrid').innerHTML = html;
}

function getDeviceTypeIcon(type) {
  const icons = {
    'fingerprint': '🔒',
    'rfid': '📱',
    'camera': '📹',
    'sensor': '📡',
    'actuator': '⚙️'
  };
  return icons[type] || '📱';
}

function getDeviceTypeLabel(type) {
  const labels = {
    'fingerprint': 'Lector de Huellas',
    'rfid': 'Lector RFID',
    'camera': 'Cámara',
    'sensor': 'Sensor',
    'actuator': 'Actuador'
  };
  return labels[type] || type;
}

function formatLastSeen(timestamp) {
  if (!timestamp || timestamp === 0) return 'Nunca';
  const currentTime = Date.now();
  const diff = Math.abs(timestamp - (currentTime % 4294967295));
  const minutes = Math.floor(diff / 60000);
  if (minutes < 1) return 'Ahora';
  if (minutes < 60) return `Hace ${minutes} min`;
  const hours = Math.floor(minutes / 60);
  if (hours < 24) return `Hace ${hours} h`;
  return `Hace ${Math.floor(hours / 24)} días`;
}

// Helper: devuelve minutos transcurridos desde timestamp (timestamp = millis() del ESP)
function getMinutesSince(timestamp) {
  if (!timestamp || timestamp === 0) return Number.MAX_SAFE_INTEGER;
  const now = Date.now();
  // El ESP envía millis() (uint32), aproximamos con modulo para compararlo con Date.now()
  const approxNow = now % 4294967295;
  const diff = Math.abs(approxNow - timestamp);
  return Math.floor(diff / 60000);
}

// Nuevo helper: considera reciente si < 5 minutos (ajustable)
function isLastSeenRecent(timestamp, minutesThreshold = 5) {
  return getMinutesSince(timestamp) < minutesThreshold;
}

// Mejorar formatModuleTime: calcular recencia correctamente (usa minutos desde timestamp)
function formatModuleTime(timestamp) {
  if (!timestamp || timestamp === 0) return 'Nunca';
  const minutes = getMinutesSince(timestamp);
  if (minutes < 1) return 'Ahora';
  if (minutes < 60) return `Hace ${minutes} min`;
  const hours = Math.floor(minutes / 60);
  if (hours < 24) return `Hace ${hours} h`;
  return `Hace ${Math.floor(hours / 24)} días`;
}

function loadModules() {
  return new Promise((resolve, reject) => {
    console.log('Cargando módulos...');
    const x = new XMLHttpRequest();
    x.timeout = 5000; // 5 segundos timeout
    x.open('GET', '/api/modules');

    x.onreadystatechange = function() {
      if (x.readyState == 4) {
        if (x.status == 200) {
          try {
            const d = JSON.parse(x.responseText);
            console.log('Módulos recibidos:', d.modules?.length || 0);
            const modules = d.modules || [];

            if (modules.length === 0) {
              console.log('No hay módulos desde /api/modules, intentando fallback desde /api/devices');
              const x2 = new XMLHttpRequest();
              x2.timeout = 4000;
              x2.open('GET', '/api/devices');
              x2.onreadystatechange = function() {
                if (x2.readyState == 4) {
                  try {
                    if (x2.status == 200) {
                      const dd = JSON.parse(x2.responseText);
                      const devices = dd.devices || [];
                      const pseudoModules = [];
                      // Incluir TODOS los dispositivos registrados (devices) como módulos,
                      // marcando isActive si tienen hints de conexión.
                      devices.forEach(dev => {
                        const hasConnectionHints = (dev.currentIP && dev.currentIP !== '') || (dev.clientIndex && dev.clientIndex >= 0) || !!dev.isConnected;
                        const moduleId = (dev.deviceType || 'module') + '_' + (dev.macAddress ? dev.macAddress.replace(/:/g,'') : Math.random().toString(36).substring(2,8));
                        const lastHb = dev.lastSeen || 0;
                        const active = hasConnectionHints || isLastSeenRecent(dev.lastSeen, 5);
                        pseudoModules.push({
                          moduleId: moduleId,
                          moduleType: dev.deviceType || 'unknown',
                          capabilities: dev.capabilities || '',
                          macAddress: dev.macAddress || '',
                          isActive: active,
                          lastHeartbeat: lastHb,
                          // Venimos de la lista "devices" => son módulos registrados
                          isRegistered: true,
                          // Mostrar nombre legible: preferir description o name si están
                          displayName: dev.description || dev.name || moduleId
                        });
                      });
                      console.log('Fallback pseudo-modules (todos devices):', pseudoModules.length);
                      displayModules(pseudoModules);
                    } else {
                      console.warn('/api/devices HTTP error in fallback:', x2.status);
                      displayModules([]); // mostrar vacío
                    }
                  } catch (e) {
                    console.error('Error parsing devices in fallback:', e);
                    displayModules([]); // mostrar vacío
                  }
                  resolve();
                }
              };
              x2.ontimeout = () => { console.warn('Fallback /api/devices timeout'); displayModules([]); resolve(); };
              x2.onerror = () => { console.warn('Fallback /api/devices network error'); displayModules([]); resolve(); };
              x2.send();
            } else {
              // Si /api/modules devolvió módulos, intentar enriquecer nombres consultando /api/devices
              // para usar las descripciones que pusiste al registrar
              try {
                const xDev = new XMLHttpRequest();
                xDev.timeout = 4000;
                xDev.open('GET', '/api/devices');
                xDev.onreadystatechange = function() {
                  if (xDev.readyState == 4) {
                    if (xDev.status == 200) {
                      try {
                        const dd = JSON.parse(xDev.responseText);
                        const devicesList = dd.devices || [];
                        // construir mapa mac (sin : y con mayúsculas) -> description/name
                        const nameMap = {};
                        devicesList.forEach(dv => {
                          const mac = (dv.macAddress || dv.mac || dv.mac_address || '').toString().toUpperCase();
                          if (mac) {
                            nameMap[ mac.replace(/:/g,'') ] = (dv.description || dv.name || dv.label || dv.registeredName || '').toString();
                          }
                        });
                        // enriquecer módulos
                        modules.forEach(m => {
                          // posibles campos de mac en el módulo
                          const macCandidates = [
                            m.macAddress, m.mac, m.deviceMac, m.mac_address, m.address
                          ];
                          let macPlain = '';
                          for (const c of macCandidates) {
                            if (c) { macPlain = c.toString().toUpperCase().replace(/:/g,''); break; }
                          }
                          // nombre preferido del módulo (varios aliases)
                          const currentName = m.displayName || m.name || m.description || m.label || m.friendlyName || m.deviceName || m.registeredName || '';
                          if (!currentName || currentName === '' || currentName.match(/^.+_[0-9A-F]{12}$/i)) {
                            // si hay nombre en devices, usarlo
                            const mapped = macPlain ? nameMap[macPlain] : null;
                            if (mapped && mapped.length > 0) {
                              m.displayName = mapped;
                            } else {
                              // fallback: conservar lo que tenga o moduleId
                              m.displayName = currentName || m.moduleId;
                            }
                          } else {
                            m.displayName = currentName;
                          }
                        });
                        displayModules(modules);
                      } catch (e) {
                        console.warn('No se pudo parsear /api/devices en enriquecimiento de módulos:', e);
                        displayModules(modules);
                      }
                    } else {
                      // no hay devices; mostrar módulos tal cual
                      displayModules(modules);
                    }
                    resolve();
                  }
                };
                xDev.ontimeout = function() { displayModules(modules); resolve(); };
                xDev.onerror = function() { displayModules(modules); resolve(); };
                xDev.send();
              } catch (e) {
                console.warn('Error enriqueciendo módulos con /api/devices:', e);
                displayModules(modules);
                resolve();
              }
            }

          } catch (e) {
            console.error('Error parsing modules:', e);
            reject(e);
          }
        } else {
          console.error('Modules HTTP error:', x.status);
          reject(new Error('HTTP ' + x.status));
        }
      }
    };

    x.ontimeout = function() {
      console.error('Modules timeout');
      reject(new Error('Timeout'));
    };

    x.onerror = function() {
      console.error('Modules network error');
      reject(new Error('Network error'));
    };

    x.send();
  });
}

function displayModules(modules) {
  let html = '<div class="device-grid">';
  if (modules.length === 0) {
    html += '<div style="text-align:center;padding:40px;color:#666">No hay módulos activos</div>';
  } else {
    modules.forEach(module => {
      const isActive = !!module.isActive;
      const statusClass = isActive ? 'status-online' : 'status-offline';
      const statusText = isActive ? 'Activo' : 'Inactivo';

      // elegir nombre a mostrar: displayName -> varios aliases -> moduleId
      const shownName = module.displayName || module.registeredName || module.label || module.friendlyName || module.deviceName || module.name || module.description || module.moduleId;

      html += `
        <div class="device-card" data-module-id="${module.moduleId}" data-mac="${module.macAddress || ''}" data-type="${module.moduleType || ''}">
          <div class="device-header">
            <div class="device-id">${shownName}</div>
            <div style="display:flex;align-items:center;gap:8px">
              <div class="status-led ${statusClass}" title="${statusText}"></div>
            </div>
          </div>
          <div class="device-details">
            <div><strong>Tipo:</strong> ${module.moduleType}</div>
            <div><strong>Estado:</strong> ${statusText}</div>
            <div><strong>Capacidades:</strong> ${module.capabilities}</div>
            <div><strong>MAC:</strong> ${module.macAddress}</div>
            <div><strong>Último heartbeat:</strong> ${formatModuleTime(module.lastHeartbeat)}</div>
          </div>
          <div style="margin-top:15px; display:flex; gap:8px;">
            <button class="btn btn-danger btn-small" onclick="deleteModule('${module.moduleId}')">Eliminar</button>
            <button class="btn btn-small" onclick="openModuleMenu('${module.moduleId}')">Acciones</button>
          </div>
        </div>
      `;
    });
  }
  html += '</div>';
  document.getElementById('modulesList').innerHTML = html;
}

function deleteModule(moduleId) {
  if (!moduleId) {
    showMessage('ID de módulo inválida', true);
    return;
  }
  if (!confirm('¿Eliminar módulo y persistir (EEPROM)? ' + moduleId + '?')) return;

  showMessage('Eliminando módulo ' + moduleId + ' ...', false);

  // Optimistic UI: quitar tarjeta visualmente inmediatamente
  try {
    const cards = Array.from(document.querySelectorAll('#modulesList .device-card'));
    cards.forEach(c => {
      const idEl = c.querySelector('.device-id');
      if (idEl && idEl.textContent === moduleId) {
        c.remove();
      }
    });
  } catch (e) {
    // no romper si DOM cambia
  }

  const body = 'module_id=' + encodeURIComponent(moduleId);

  // Intentar deregistrar (si el endpoint existe). No abortar si devuelve 404/otro no-ok:
  fetch('/api/modules/deregister', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: body
  })
  .then(r => {
    if (r.ok) {
      return r.json().catch(() => ({ success: false }));
    } else {
      // No existe el endpoint o devuelve error: continuar al borrado persistente
      console.warn('Deregister HTTP', r.status, '-> continuando con delete_device');
      return { success: false, httpStatus: r.status };
    }
  })
  .then(d => {
    // Siempre intentar delete_device (erase) ya sea que deregister haya dicho OK o no
    return fetch('/api/modules/delete_device', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: body + '&erase=1'
    });
  })
  .then(r2 => {
    if (!r2) throw new Error('No response from delete attempt');
    if (!r2.ok) {
      console.warn('Delete_device HTTP', r2.status);
      return r2.json().catch(() => ({ success: false, httpStatus: r2.status }));
    }
    return r2.json().catch(() => ({ success: false }));
  })
  .then(d2 => {
    if (d2 && d2.success) {
      showMessage('Módulo eliminado y persistido en EEPROM', false);
    } else {
      showMessage('Módulo eliminado parcialmente. Revise servidor. ' + (d2 && (d2.error || d2.message || '')), true);
    }
    // Recargar para estado consistente
    loadModules().catch(()=>{});
    if (currentTab === 'dashboard') loadDashboard();
  })
  .catch(err => {
    console.error('deleteModule error', err);
    showMessage('Error eliminando módulo: ' + err.message, true);
    loadModules();
  });
}

function scanAllDevices() {
  const scanBtn = document.getElementById('scanBtn');
  const scanStatus = document.getElementById('scanStatus');
  const scanResults = document.getElementById('scanResults');
  
  scanBtn.disabled = true;
  scanBtn.textContent = '🔄 Escaneando...';
  scanStatus.className = 'scan-status scanning';
  scanStatus.textContent = '🔍 Escaneando dispositivos conectados...';
  scanStatus.classList.remove('hidden');
  scanResults.innerHTML = '';
  
  fetch('/api/scan-devices', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    }
  })
  .then(response => response.json())
  .then(data => {
    if (data.success) {
      setTimeout(() => {
        fetchScanResults();
      }, 3000);
    } else {
      throw new Error(data.message || 'Error al iniciar scan');
    }
  })
  .catch(error => {
    console.error('Error:', error);
    scanStatus.className = 'scan-status error';
    scanStatus.textContent = '❌ Error al escanear dispositivos: ' + error.message;
    scanBtn.disabled = false;
    scanBtn.textContent = '🔍 Escanear Dispositivos';
  });
}

function fetchScanResults() {
  const scanBtn = document.getElementById('scanBtn');
  const scanStatus = document.getElementById('scanStatus');
  const scanResults = document.getElementById('scanResults');
  
  fetch('/api/scan-results')
  .then(response => response.json())
  .then(data => {
    console.log('fetchScanResults - Datos recibidos:', data);
    if (data.success) {
      const devices = data.devices || [];
      displayScanResults(devices);
      scanStatus.className = 'scan-status completed';
      scanStatus.textContent = `✅ Scan completado. Encontrados ${devices.length} dispositivos.`;
    } else {
      throw new Error(data.message || 'Error al obtener resultados');
    }
  })
  .catch(error => {
    console.error('Error:', error);
    scanStatus.className = 'scan-status error';
    scanStatus.textContent = '❌ Error al obtener resultados: ' + error.message;
  })
  .finally(() => {
    scanBtn.disabled = false;
    scanBtn.textContent = '🔍 Escanear Dispositivos';
  });
}

function displayScanResults(devices) {
  const scanResults = document.getElementById('scanResults');
  if (!devices || devices.length === 0) {
    scanResults.innerHTML = '<p>No se encontraron dispositivos conectados.</p>';
    return;
  }
  
  let html = '';
  devices.forEach(device => {
    const isRegistered = device.isRegistered;
    const statusClass = isRegistered ? 'registered' : 'unregistered';
    const statusBadge = isRegistered ? 
      '<span class="registered-badge">✅ REGISTRADO</span>' : 
      '<span class="unregistered-badge">⚠️ NO REGISTRADO</span>';
    
    const deviceTypeNames = {
      'fingerprint': '🔒 Lector de Huellas',
      'rfid': '📱 Lector RFID',
      'camera': '📹 Cámara',
      'sensor': '📡 Sensor',
      'actuator': '⚙️ Actuador'
    };
    
    const actions = isRegistered ? 
      `<button class="btn btn-secondary" onclick="viewDeviceDetails('${device.macAddress}')" title="Ver detalles">👁️ Ver</button>` :
      `<button class="btn" onclick="quickRegisterDevice('${device.macAddress}', '${device.deviceType}', '${device.moduleId}')" title="Registrar dispositivo">➕ Registrar</button>`;
    
    html += `
      <div class="scan-device-card ${statusClass}">
        <div class="scan-device-header">
          <div class="scan-device-info">
            <div class="scan-device-mac">${device.macAddress}</div>
            <div class="scan-device-type">${deviceTypeNames[device.deviceType] || device.deviceType}</div>
            <div class="scan-device-status">ID: ${device.moduleId}</div>
          </div>
          <div class="scan-device-actions">
            ${statusBadge}
            ${actions}
          </div>
        </div>
      </div>
    `;
  });
  scanResults.innerHTML = html;
}

function quickRegisterDevice(macAddress, deviceType, moduleId) {
  const description = prompt(`Ingrese una descripción para el dispositivo:\nMAC: ${macAddress}\nTipo: ${deviceType}`, `Dispositivo ${deviceType} - ${moduleId}`);
  if (description === null) return;
  fetch('/api/add-device', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded',
    },
    body: `macAddress=${encodeURIComponent(macAddress)}&deviceType=${encodeURIComponent(deviceType)}&description=${encodeURIComponent(description)}`
  })
  .then(response => response.json())
  .then(data => {
    if (data.success) {
      alert(`✅ Dispositivo registrado exitosamente!\nAPI Key: ${data.apiKey}`);
      fetchScanResults();
    } else {
      alert(`❌ Error: ${data.message || 'No se pudo registrar el dispositivo'}`);
    }
  })
  .catch(error => {
    console.error('Error:', error);
    alert('❌ Error de conexión al registrar dispositivo');
  });
}

function viewDeviceDetails(macAddress) {
  showTab('dashboard');
  setTimeout(() => {
    const deviceCards = document.querySelectorAll('.device-card');
    deviceCards.forEach(card => {
      const macText = card.querySelector('.device-id').textContent;
      if (macText.includes(macAddress)) {
        card.style.border = '2px solid #007bff';
        card.scrollIntoView({ behavior: 'smooth', block: 'center' });
        setTimeout(() => {
          card.style.border = '';
        }, 3000);
      }
    });
  }, 500);
}

function clearScanResults() {
  const scanResults = document.getElementById('scanResults');
  const scanStatus = document.getElementById('scanStatus');
  scanResults.innerHTML = '';
  scanStatus.classList.add('hidden');
}

function debugScan() {
  const scanStatus = document.getElementById('scanStatus');
  const scanResults = document.getElementById('scanResults');
  scanStatus.className = 'scan-status scanning';
  scanStatus.textContent = '🐛 Obteniendo información de debug...';
  scanStatus.classList.remove('hidden');
  fetch('/api/scan-devices', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' }
  })
  .then(response => response.json())
  .then(data => {
    console.log('Debug - Respuesta scan-devices:', data);
    return fetch('/api/scan-results');
  })
  .then(response => response.json())
  .then(data => {
    console.log('Debug - Respuesta scan-results:', data);
    scanStatus.className = 'scan-status completed';
    scanStatus.textContent = `🐛 Debug completado. Check consola del navegador. Dispositivos: ${data.total_devices || 0}`;
    scanResults.innerHTML = `
      <div style="background: #f8f9fa; padding: 15px; border-radius: 6px; font-family: monospace;">
        <h4>🐛 Información de Debug:</h4>
        <p><strong>Total dispositivos:</strong> ${data.total_devices || 0}</p>
        <p><strong>Estado API:</strong> ${data.success ? '✅ OK' : '❌ Error'}</p>
        <p><strong>Datos completos:</strong></p>
        <pre>${JSON.stringify(data, null, 2)}</pre>
      </div>
    `;
  })
  .catch(error => {
    console.error('Debug Error:', error);
    scanStatus.className = 'scan-status error';
    scanStatus.textContent = '❌ Error en debug: ' + error.message;
  });
}

function addDevice() {
  const mac = document.getElementById('newMac').value;
  const type = document.getElementById('newType').value;
  const desc = document.getElementById('newDesc').value;
  if (!mac || !type || !desc) {
    showMessage('Complete todos los campos', true, 'registerMessage');
    return;
  }
  const x = new XMLHttpRequest();
  x.open('POST', '/api/add-device');
  x.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
  x.onreadystatechange = function() {
    if (x.readyState == 4) {
      try {
        const d = JSON.parse(x.responseText);
        if (d.success) {
          showMessage('Dispositivo agregado! API Key: ' + d.apiKey, false, 'registerMessage');
          document.getElementById('newMac').value = '';
          document.getElementById('newDesc').value = '';
          if (currentTab === 'dashboard') loadDashboard();
        } else {
          showMessage('Error: ' + d.message, true, 'registerMessage');
        }
      } catch (e) {
        showMessage('Error de conexión', true, 'registerMessage');
      }
    }
  };
  x.send('macAddress=' + encodeURIComponent(mac) + '&deviceType=' + encodeURIComponent(type) + '&description=' + encodeURIComponent(desc));
}

function requestMAC() {
  const btn = document.getElementById('requestMacBtn');
  btn.textContent = 'Consultando...';
  btn.disabled = true;
  const x = new XMLHttpRequest();
  x.open('POST', '/api/request-mac');
  x.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
  x.onreadystatechange = function() {
    if (x.readyState == 4) {
      try {
        const d = JSON.parse(x.responseText);
        if (d.success) {
          setTimeout(function() {
            const x2 = new XMLHttpRequest();
            x2.open('GET', '/api/get-mac');
            x2.onreadystatechange = function() {
              if (x2.readyState == 4) {
                try {
                  const md = JSON.parse(x2.responseText);
                  if (md.success) {
                    document.getElementById('newMac').value = md.macAddress;
                    if (md.deviceType) {
                      const typeSelect = document.getElementById('newType');
                      const validTypes = ['fingerprint', 'rfid', 'camera', 'sensor', 'actuator'];
                      if (validTypes.includes(md.deviceType)) {
                        typeSelect.value = md.deviceType;
                        const deviceTypeNames = {
                          'fingerprint': '🔒 Lector de Huellas',
                          'rfid': '📱 Lector RFID',
                          'camera': '📹 Cámara',
                          'sensor': '📡 Sensor',
                          'actuator': '⚙️ Actuador'
                        };
                        setTimeout(() => {
                          showMessage(`Dispositivo detectado: ${deviceTypeNames[md.deviceType] || md.deviceType}`, false, 'registerMessage');
                        }, 500);
                      } else {
                        console.log('Tipo de dispositivo no reconocido:', md.deviceType);
                      }
                    }
                    btn.textContent = 'OK!';
                    btn.style.background = '#28a745';
                    setTimeout(function() {
                      btn.textContent = 'Consultar';
                      btn.style.background = '#6c757d';
                      btn.disabled = false;
                    }, 2000);
                  } else {
                    showMessage('No se pudo obtener MAC', true, 'registerMessage');
                    btn.textContent = 'Consultar';
                    btn.disabled = false;
                  }
                } catch (e) {
                  btn.textContent = 'Consultar';
                  btn.disabled = false;
                }
              }
            };
            x2.send();
          }, 2000);
        }
      } catch (e) {
        btn.textContent = 'Consultar';
        btn.disabled = false;
      }
    }
  };
  x.send();
}

function removeDevice(mac) {
  if (confirm('¿Eliminar dispositivo (vista) ' + mac + '?')) {
    const x = new XMLHttpRequest();
    x.open('POST', '/api/dashboard/delete_device'); // visual endpoint
    x.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    x.onreadystatechange = function() {
      if (x.readyState == 4) {
        try {
          const d = JSON.parse(x.responseText);
          if (d.success) {
            loadDashboard();
            showMessage('Dispositivo eliminado (vista)', false, 'message');
          } else {
            showMessage('Error eliminando dispositivo (vista)', true, 'message');
          }
        } catch (e) {
          showMessage('Error de conexión', true, 'message');
        }
      }
    };
    x.send('macAddress=' + encodeURIComponent(mac));
  }
}

function loadSystemInfo() {
  return new Promise((resolve) => {
    console.log('Cargando información del sistema...');
    const x = new XMLHttpRequest();
    x.timeout = 3000; // 3 segundos timeout
    x.open('GET', '/api/system-info');
    
    x.onreadystatechange = function() {
      if (x.readyState == 4) {
        if (x.status == 200) {
          try {
            const d = JSON.parse(x.responseText);
            console.log('System info recibida:', d);
            document.getElementById('brokerIP').textContent = d.ap_ip || '192.168.4.1';
          } catch (e) {
            console.error('Error parsing system info:', e);
          }
        } else {
          console.error('System info HTTP error:', x.status);
        }
        resolve();
      }
    };
    
    x.ontimeout = function() {
      console.error('System info timeout');
      resolve(); // No rechazamos para no bloquear el dashboard
    };
    
    x.onerror = function() {
      console.error('System info network error');
      resolve(); // No rechazamos para no bloquear el dashboard
    };
    
    x.send();
  });
}

function showRefreshIndicator(show) {
  const indicator = document.getElementById('refreshIndicator');
  if (show) {
    indicator.classList.add('active', 'spinning');
  } else {
    indicator.classList.remove('active', 'spinning');
  }
}

function startAutoRefresh() {
  if (refreshInterval) clearInterval(refreshInterval);
  refreshInterval = setInterval(() => {
    if (currentTab === 'dashboard') {
      loadDashboard();
    }
  }, 10000); // Actualizar cada 10 segundos
}

window.onload = function() {
  loadSystemInfo();
};

/* NUEVAS FUNCIONES JS: abrir/cerrar menú y render placeholder de opciones */
function openModuleMenu(moduleId) {
  const menu = document.getElementById('moduleMenu');
  const titleEl = document.getElementById('moduleMenuTitle');
  const bodyEl = document.getElementById('moduleMenuBody');

  // Intentar obtener info del módulo en la UI (si existe)
  const card = document.querySelector(`[data-module-id="${moduleId}"]`);
  let name = moduleId;
  let mac = '';
  if (card) {
    const idEl = card.querySelector('.device-id');
    name = idEl ? idEl.textContent : moduleId;
    mac = card.getAttribute('data-mac') || '';
  }

  titleEl.textContent = `Acciones — ${name}`;

  // Mostrar estado de carga y luego pedir capacidades al servidor
  bodyEl.innerHTML = '<div style="color:#666;margin:8px 0">Cargando funcionalidades...</div>';

  // Endpoint esperado: GET /api/modules/{moduleId}/capabilities
  fetch('/api/modules/' + encodeURIComponent(moduleId) + '/capabilities', { method: 'GET' })
    .then(r => r.json().catch(()=>({success:false})))
    .then(data => {
      // Estructura esperada: { success: true, capabilities: [...] }
      if (data && data.success && Array.isArray(data.capabilities) && data.capabilities.length > 0) {
        let html = `<div style="margin-top:6px">
                      <div style="margin-bottom:8px;color:#333"><strong>ID:</strong> ${moduleId}</div>
                      <div style="margin-bottom:8px;color:#333"><strong>MAC:</strong> ${mac || 'N/A'}</div>
                      <div style="margin:8px 0"><strong>Funciones disponibles:</strong></div>
                      <div style="display:flex;flex-direction:column;gap:8px">`;
        data.capabilities.forEach(cap => {
          const capTitle = cap.title || cap.name;
          const capDesc = cap.description ? `<div style="font-size:12px;color:#666;margin-top:4px">${cap.description}</div>` : '';
          html += `
            <div style="border:1px solid #eee;padding:8px;border-radius:6px">
              <div style="display:flex;justify-content:space-between;align-items:center">
                <div>
                  <div style="font-weight:700">${capTitle}</div>
                  ${capDesc}
                </div>
                <div style="display:flex;flex-direction:column;gap:6px">
                  <button class="btn btn-small" onclick="executeCapability('${moduleId}','${encodeURIComponent(cap.name)}')">Ejecutar</button>
                </div>
              </div>
            </div>
          `;
        });
        html += `</div>
                 <div style="margin-top:10px">
                   <button class="btn btn-danger" onclick="deleteModule('${moduleId}')">Eliminar (persistente)</button>
                 </div>
                </div>`;
        bodyEl.innerHTML = html;
      } else {
        // Fallback local: intentar inferir capacidades por tipo de módulo
        const card = document.querySelector(`[data-module-id="${moduleId}"]`);
        const modType = card ? (card.getAttribute('data-type') || '').toLowerCase() : '';
        // Mapa de capacidades por tipo (añadir más tipos si hace falta)
        const typeCapabilities = {
          'fingerprint': [
            { name: 'enroll', title: 'Registrar Huella', description: 'Registrar una nueva huella', params: [] },
            { name: 'scan',   title: 'Escanear Huella',  description: 'Forzar escaneo ahora', params: [] },
            { name: 'delete', title: 'Eliminar Huella', description: 'Eliminar huella por ID', params: [{ name: 'id', type: 'number', required: true }] }
          ],
          'rfid': [
            { name: 'scan', title: 'Escanear RFID', description: 'Forzar escaneo RFID', params: [] },
            { name: 'delete', title: 'Eliminar tag', description: 'Eliminar tag registrado', params: [{ name: 'mac', type: 'string', required: true }] }
          ]
        };

        const caps = typeCapabilities[modType] || null;

        if (caps) {
          let html = `<div style="margin-top:6px">
                        <div style="margin-bottom:8px;color:#333"><strong>ID:</strong> ${moduleId}</div>
                        <div style="margin-bottom:8px;color:#333"><strong>MAC:</strong> ${mac || 'N/A'}</div>
                        <div style="margin:8px 0"><strong>Funciones disponibles (por tipo: ${modType}):</strong></div>
                        <div style="display:flex;flex-direction:column;gap:8px">`;
          caps.forEach(cap => {
            const capTitle = cap.title || cap.name;
            const capDesc = cap.description ? `<div style="font-size:12px;color:#666;margin-top:4px">${cap.description}</div>` : '';
            html += `
              <div style="border:1px solid #eee;padding:8px;border-radius:6px">
                <div style="display:flex;justify-content:space-between;align-items:center">
                  <div>
                    <div style="font-weight:700">${capTitle}</div>
                    ${capDesc}
                  </div>
                  <div style="display:flex;flex-direction:column;gap:6px">
                    <button class="btn btn-small" onclick="executeCapability('${moduleId}','${encodeURIComponent(cap.name)}')">Ejecutar</button>
                  </div>
                </div>
              </div>
            `;
          });
          html += `</div>
                   <div style="margin-top:10px">
                     <button class="btn btn-danger" onclick="deleteModule('${moduleId}')">Eliminar (persistente)</button>
                   </div>
                  </div>`;
          bodyEl.innerHTML = html;
        } else {
          // Ningún fallback conocido: mostrar mensaje original
          bodyEl.innerHTML = `
            <div style="margin-top:6px">
              <div style="margin-bottom:8px;color:#333"><strong>ID:</strong> ${moduleId}</div>
              <div style="margin-bottom:8px;color:#333"><strong>MAC:</strong> ${mac || 'N/A'}</div>
              <p style="color:#666;margin:8px 0">No se encontraron funcionalidades en el servidor para este módulo.</p>
              <button class="btn btn-small" onclick="actionRefreshModule('${moduleId}')">Refrescar estado</button>
            </div>
          `;
        }
      }
    })
    .catch(err => {
      // En caso de error de comunicación, intentar también el fallback por tipo
      console.error('Error obteniendo capacidades:', err);
      const card = document.querySelector(`[data-module-id="${moduleId}"]`);
      const modType = card ? (card.getAttribute('data-type') || '').toLowerCase() : '';
      const typeCapabilities = {
        'fingerprint': [
          { name: 'enroll', title: 'Registrar Huella', description: 'Registrar una nueva huella', params: [] },
          { name: 'scan',   title: 'Escanear Huella',  description: 'Forzar escaneo ahora', params: [] },
          { name: 'delete', title: 'Eliminar Huella', description: 'Eliminar huella por ID', params: [{ name: 'id', type: 'number', required: true }] }
        ],
        'rfid': [
          { name: 'scan', title: 'Escanear RFID', description: 'Forzar escaneo RFID', params: [] },
          { name: 'delete', title: 'Eliminar tag', description: 'Eliminar tag registrado', params: [{ name: 'mac', type: 'string', required: true }] }
        ]
      };
      const caps = typeCapabilities[modType] || null;
      if (caps) {
        let html = `<div style="margin-top:6px">
                      <div style="margin-bottom:8px;color:#333"><strong>ID:</strong> ${moduleId}</div>
                      <div style="margin-bottom:8px;color:#333"><strong>MAC:</strong> ${mac || 'N/A'}</div>
                      <div style="margin:8px 0"><strong>Funciones disponibles (por tipo: ${modType}):</strong></div>
                      <div style="display:flex;flex-direction:column;gap:8px">`;
        caps.forEach(cap => {
          const capTitle = cap.title || cap.name;
          const capDesc = cap.description ? `<div style="font-size:12px;color:#666;margin-top:4px">${cap.description}</div>` : '';
          html += `
            <div style="border:1px solid #eee;padding:8px;border-radius:6px">
              <div style="display:flex;justify-content:space-between;align-items:center">
                <div>
                  <div style="font-weight:700">${capTitle}</div>
                  ${capDesc}
                </div>
                <div style="display:flex;flex-direction:column;gap:6px">
                  <button class="btn btn-small" onclick="executeCapability('${moduleId}','${encodeURIComponent(cap.name)}')">Ejecutar</button>
                </div>
              </div>
            </div>
          `;
        });
        html += `</div>
                 <div style="margin-top:10px">
                   <button class="btn btn-danger" onclick="deleteModule('${moduleId}')">Eliminar (persistente)</button>
                 </div>
                </div>`;
        bodyEl.innerHTML = html;
      } else {
        bodyEl.innerHTML = `
          <div style="margin-top:6px">
            <div style="margin-bottom:8px;color:#333"><strong>ID:</strong> ${moduleId}</div>
            <div style="margin-bottom:8px;color:#333"><strong>MAC:</strong> ${mac || 'N/A'}</div>
            <p style="color:#b21;margin:8px 0">No se pudo cargar las funcionalidades (error de comunicación).</p>
            <button class="btn btn-small" onclick="actionRefreshModule('${moduleId}')">Reintentar</button>
          </div>
        `;
      }
    });
  menu.classList.add('show');
  menu.setAttribute('aria-hidden', 'false');
}

/* Nueva función: ejecutar comando en módulo vía servidor */
function sendModuleCommand(moduleId, commandName, params = {}) {
  showMessage('Enviando comando ' + commandName + ' a ' + moduleId + ' ...', false);
  return fetch('/api/modules/action', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ moduleId, action: commandName, params })
  }).then(r => r.json().catch(()=>({ success:false })))
    .then(res => {
      if (res && res.success) {
        showMessage('Comando ejecutado correctamente', false);
      } else {
        showMessage('Error ejecutando comando: ' + (res.message || 'sin detalle'), true);
      }
      // refrescar estado de módulos por si cambió
      loadModules().catch(()=>{});
      return res;
    })
    .catch(err => {
      console.error('sendModuleCommand error', err);
      showMessage('Error de red al ejecutar comando', true);
      throw err;
    });
}

/* Helper para llamadas desde UI (decodifica el nombre si fue codificado) */
function executeCapability(moduleId, encodedCapName) {
  const capName = decodeURIComponent(encodedCapName);
  if (!confirm('Ejecutar "' + capName + '" en ' + moduleId + '?')) return;
  // Para capacidades que requieren parámetros añadir UI adicional; por ahora se envían sin params
  sendModuleCommand(moduleId, capName, {}).catch(()=>{});
}

// ...existing code...
</script>
</body>
</html>
)HTML";
}