import socket
import time

HOST = "0.0.0.0"
PORT = 5000

PING_INTERVAL = 2.0      # segundos entre PING
PONG_TIMEOUT = 5.0       # máximo tiempo esperando PONG


def manejar_cliente(client_socket, client_address):

    print(f"[+] Cliente conectado: {client_address}")

    client_socket.settimeout(PONG_TIMEOUT)

    try:
        while True:

            # Enviar PING
            client_socket.sendall(b"PING\n")
            print(">> PING")

            # Esperar respuesta
            data = client_socket.recv(1024)

            if not data:
                print("[-] Cliente cerró la conexión")
                break

            mensaje = data.decode(errors="ignore").strip()

            print(f"<< {mensaje}")

            if mensaje == "PONG":
                print("[OK] PLC responde")
            else:
                print(f"[!] Respuesta inesperada: {mensaje}")

            time.sleep(PING_INTERVAL)

    except socket.timeout:
        print("[!] Timeout: el PLC no respondió al PING")

    except (ConnectionResetError, BrokenPipeError):
        print("[!] Conexión perdida")

    except Exception as e:
        print(f"[!] Error: {e}")

    finally:
        client_socket.close()
        print("[-] Cliente desconectado")


def main():

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Permite reiniciar el servidor sin esperar a que se libere el puerto
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    server.bind((HOST, PORT))
    server.listen(1)

    print(f"Servidor TCP escuchando en puerto {PORT}")

    while True:

        print("\nEsperando PLC...")

        client_socket, client_address = server.accept()

        manejar_cliente(client_socket, client_address)


if __name__ == "__main__":
    main()