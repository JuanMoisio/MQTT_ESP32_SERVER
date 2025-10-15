#include "web_interface.h"

const char* getAdminHTML() {
  return R"HTML(
<!DOCTYPE html>
<html>
<head>
<title>Admin Panel</title>
<style>
body{font-family:Arial;margin:20px;background:#f0f0f0}
.container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:8px}
.form-group{margin-bottom:15px}
.form-group label{display:block;margin-bottom:5px;font-weight:bold}
.form-group input,.form-group select{width:100%;padding:8px;border:1px solid #ddd;border-radius:4px;box-sizing:border-box}
.btn{background:#007bff;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;margin-right:10px}
.btn-secondary{background:#6c757d;color:white;padding:8px 16px;border:none;border-radius:4px;cursor:pointer}
.hidden{display:none}
table{width:100%;border-collapse:collapse;margin-top:15px}
th,td{padding:8px;border:1px solid #ddd;text-align:left}
th{background:#f2f2f2}
#message{margin:10px 0}
.success{background:#d4edda;color:#155724;padding:10px;border-radius:4px}
.error{background:#f8d7da;color:#721c24;padding:10px;border-radius:4px}
</style>
</head>
<body>
<div class="container">
  <div id="loginSection">
    <h2>Admin Login</h2>
    <div class="form-group">
      <label>Usuario</label>
      <input type="text" id="username" value="admin">
    </div>
    <div class="form-group">
      <label>Password</label>
      <input type="password" id="password" value="deposito123">
    </div>
    <button class="btn" onclick="login()">Entrar</button>
    <div id="message"></div>
  </div>
  
  <div id="adminSection" class="hidden">
    <h2>Control de Dispositivos</h2>
    
    <h3>Estado del Sistema</h3>
    <p>Estado: Online | IP: <span id="brokerIP">192.168.4.1</span></p>
    
    <h3>Agregar Dispositivo</h3>
    <div class="form-group">
      <label>MAC Address</label>
      <div style="display:flex;gap:8px">
        <input type="text" id="newMac" placeholder="AA:BB:CC:DD:EE:FF" style="flex:1">
        <button type="button" class="btn-secondary" onclick="requestMAC()" id="requestMacBtn">Consultar</button>
      </div>
    </div>
    <div class="form-group">
      <label>Tipo de Dispositivo</label>
      <select id="newType">
        <option value="fingerprint">Lector Huellas</option>
        <option value="camera">Camara</option>
        <option value="sensor">Sensor</option>
      </select>
    </div>
    <div class="form-group">
      <label>Descripcion</label>
      <input type="text" id="newDesc" placeholder="Descripcion del dispositivo">
    </div>
    <button class="btn" onclick="addDevice()">Agregar Dispositivo</button>
    
    <div id="deviceList"></div>
  </div>
</div>

<script>
function showMessage(msg,isError){
  document.getElementById('message').innerHTML='<div class="'+(isError?'error':'success')+'">'+msg+'</div>';
}
function login(){
  var u=document.getElementById('username').value;
  var p=document.getElementById('password').value;
  var x=new XMLHttpRequest();
  x.open('POST','/api/login');
  x.setRequestHeader('Content-Type','application/x-www-form-urlencoded');
  x.onreadystatechange=function(){
    if(x.readyState==4){
      var d=JSON.parse(x.responseText);
      if(d.success){
        document.getElementById('loginSection').style.display='none';
        document.getElementById('adminSection').style.display='block';
        loadDevices();
      }else{
        showMessage('Error: '+d.message,true);
      }
    }
  };
  x.send('username='+encodeURIComponent(u)+'&password='+encodeURIComponent(p));
}
function loadDevices(){
  var x=new XMLHttpRequest();
  x.open('GET','/api/devices');
  x.onreadystatechange=function(){
    if(x.readyState==4){
      var d=JSON.parse(x.responseText);
      displayDevices(d.devices);
    }
  };
  x.send();
}
function displayDevices(devices){
  var h='<h3>Dispositivos Autorizados</h3>';
  if(devices.length==0){
    h+='<p>Sin dispositivos registrados</p>';
  }else{
    h+='<table><tr><th>MAC</th><th>Tipo</th><th>Estado</th><th>API Key</th><th>Accion</th></tr>';
    for(var i=0;i<devices.length;i++){
      var dev=devices[i];
      var st=dev.isActive?'Activo':'Inactivo';
      var key=dev.apiKey.substring(0,12)+'...';
      h+='<tr><td>'+dev.macAddress+'</td><td>'+dev.deviceType+'</td><td>'+st+'</td><td>'+key+'</td><td><button onclick="removeDevice(\''+dev.macAddress+'\')">Eliminar</button></td></tr>';
    }
    h+='</table>';
  }
  document.getElementById('deviceList').innerHTML=h;
}
function addDevice(){
  var mac=document.getElementById('newMac').value;
  var type=document.getElementById('newType').value;
  var desc=document.getElementById('newDesc').value;
  if(!mac||!type||!desc){alert('Complete todos los campos');return;}
  var x=new XMLHttpRequest();
  x.open('POST','/api/add-device');
  x.setRequestHeader('Content-Type','application/x-www-form-urlencoded');
  x.onreadystatechange=function(){
    if(x.readyState==4){
      var d=JSON.parse(x.responseText);
      if(d.success){
        alert('Dispositivo agregado! API Key: '+d.apiKey);
        document.getElementById('newMac').value='';
        document.getElementById('newDesc').value='';
        loadDevices();
      }else{
        alert('Error: '+d.message);
      }
    }
  };
  x.send('macAddress='+encodeURIComponent(mac)+'&deviceType='+encodeURIComponent(type)+'&description='+encodeURIComponent(desc));
}
function requestMAC(){
  var btn=document.getElementById('requestMacBtn');
  btn.textContent='Consultando...';
  btn.disabled=true;
  var x=new XMLHttpRequest();
  x.open('POST','/api/request-mac');
  x.setRequestHeader('Content-Type','application/x-www-form-urlencoded');
  x.onreadystatechange=function(){
    if(x.readyState==4){
      var d=JSON.parse(x.responseText);
      if(d.success){
        setTimeout(function(){
          var x2=new XMLHttpRequest();
          x2.open('GET','/api/get-mac');
          x2.onreadystatechange=function(){
            if(x2.readyState==4){
              var md=JSON.parse(x2.responseText);
              if(md.success){
                document.getElementById('newMac').value=md.macAddress;
                btn.textContent='OK!';
                btn.style.background='#28a745';
                setTimeout(function(){
                  btn.textContent='Consultar';
                  btn.style.background='#6c757d';
                  btn.disabled=false;
                },2000);
              }else{
                alert('No se pudo obtener MAC');
                btn.textContent='Consultar';
                btn.disabled=false;
              }
            }
          };
          x2.send();
        },2000);
      }
    }
  };
  x.send();
}
function removeDevice(mac){
  if(confirm('Eliminar dispositivo '+mac+'?')){
    var x=new XMLHttpRequest();
    x.open('POST','/api/remove-device');
    x.setRequestHeader('Content-Type','application/x-www-form-urlencoded');
    x.onreadystatechange=function(){
      if(x.readyState==4){
        var d=JSON.parse(x.responseText);
        if(d.success) loadDevices();
        else alert('Error eliminando dispositivo');
      }
    };
    x.send('macAddress='+encodeURIComponent(mac));
  }
}
var x=new XMLHttpRequest();
x.open('GET','/api/system-info');
x.onreadystatechange=function(){
  if(x.readyState==4){
    var d=JSON.parse(x.responseText);
    document.getElementById('brokerIP').textContent=d.ap_ip||'192.168.4.1';
  }
};
x.send();
</script>
</body>
</html>
)HTML";
}