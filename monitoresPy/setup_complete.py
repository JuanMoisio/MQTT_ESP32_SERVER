#!/usr/bin/env python3
"""
ESP32 Fingerprint System - Setup Completo
Usa las utilidades centralizadas para detectar dispositivos y subir firmware.
"""

import subprocess
import time
import sys
import os

# Imports con fallback para permitir ejecución directa o como paquete
try:
	from .utils import Colors
	from .uploader import detect_esp32_devices, upload_to_device
	from .config import PROJECTS
except Exception:
	# fallback when running the script directly
	from utils import Colors
	from uploader import detect_esp32_devices, upload_to_device
	from config import PROJECTS


def main(dry_run=False):
	print(f"{Colors.BOLD}{Colors.MAGENTA}")
	print("=" * 70)
	print("    🚀 SISTEMA DE HUELLAS DACTILARES ESP32 - SETUP COMPLETO")
	print("=" * 70)
	print(f"{Colors.END}")

	print("Verificando dispositivos conectados...")
	devices = detect_esp32_devices()
	if not devices or (not devices.get('BROKER') or not devices.get('FINGERPRINT')):
		print(f"{Colors.RED}No se detectaron los dispositivos necesarios{Colors.END}")
		return 1

	if dry_run:
		print(f"{Colors.YELLOW}MODO DRY-RUN: no se realizará upload ni se abrirán monitores{Colors.END}")
		for role, info in devices.items():
			print(f"  {role}: port={info.get('port')}")
		# show what would be uploaded
		if devices.get('BROKER') and PROJECTS.get('BROKER'):
			print(f"  -> subir {PROJECTS['BROKER']} a {devices['BROKER'].get('port')}")
		if devices.get('FINGERPRINT') and PROJECTS.get('FINGERPRINT'):
			print(f"  -> subir {PROJECTS['FINGERPRINT']} a {devices['FINGERPRINT'].get('port')}")
		return 0

	success = True
	if not dry_run and devices['BROKER'].get('port') and PROJECTS.get('BROKER'):
		ok = upload_to_device(PROJECTS['BROKER'], devices['BROKER']['port'], 'ESP32-C3 (BROKER)')
		success = success and ok
		time.sleep(3)

	if not dry_run and devices['FINGERPRINT'].get('port') and PROJECTS.get('FINGERPRINT'):
		ok = upload_to_device(PROJECTS['FINGERPRINT'], devices['FINGERPRINT']['port'], 'ESP32-WROOM (CLIENT)')
		success = success and ok
		time.sleep(3)

	if not success:
		print(f"{Colors.RED}Alguno de los uploads falló{Colors.END}")
		return 1

	# Abrir monitores serie (usar monitor_cli canonical)
	if not dry_run:
		monitor_script = os.path.join(os.path.dirname(__file__), 'monitor_cli.py')
		try:
			subprocess.run(['python3', monitor_script])
		except Exception:
			# fallback: try to run as module
			try:
				subprocess.run(['python3', '-m', 'monitoresPy.monitor_cli'])
			except Exception:
				print(f"{Colors.YELLOW}No se pudo abrir el monitor automaticamente{Colors.END}")
	else:
		print(f"{Colors.YELLOW}Dry-run: no se abren monitores{Colors.END}")

	print(f"\n{Colors.GREEN}✅ CONFIGURACIÓN COMPLETADA!{Colors.END}")
	return 0


if __name__ == '__main__':
	try:
		import argparse
		p = argparse.ArgumentParser(prog='setup_complete')
		p.add_argument('--dry-run', action='store_true', help='Detectar dispositivos y mostrar acciones, sin subir ni abrir monitores')
		args = p.parse_args()
		code = main(dry_run=args.dry_run)
		sys.exit(code)
	except KeyboardInterrupt:
		print(f"\n{Colors.YELLOW}Configuración interrumpida por el usuario{Colors.END}")
		sys.exit(1)


  
